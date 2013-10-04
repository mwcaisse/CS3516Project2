#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "prog2.h"

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 0    /* change to 1 if you're doing extra credit */
                           /* and write a routine called B_output */
                           
/** The message window that A will use */
struct message_window a_window;

/** The message window that B will use */
struct receiver_window b_window;

int b_recv;


/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* Called from Application layer, sends the given message to node B*/
A_output(message)
  struct msg message;
{

	//check if our window is full
	if (list_full(a_window.unacked_packets)) {
		//we have too many outstanding packets. buffer the messages
		if (list_full(a_window.buffered_messages)) {
			printf("Droping a message, buffer full \n");
		}
		else {
			printf("Window is full, adding to message buffer \n");
			struct msg* message_ptr = (struct msg*) malloc(sizeof(struct msg));
			memcpy(message_ptr, &message, sizeof(struct msg));
			add_to_list(a_window.buffered_messages,message_ptr);
		}

	}
	
	else {
		//we hae less than 8 outstanding packets
		//send the message	
		printf("A window is not full, sending a message \n");
		send_message(&message);		
	}
}

/* need be completed only for extra credit (ignore for this project) */
B_output(message)  
  struct msg message;
{
  
}

/* Called from the Network Layer when node A receives a message,
	From Node B in this case 
	@param packet The packet received from the network
*/
A_input(packet)
  struct pkt packet;
{
	printf("A has received a pcket, lets check if it is valid and an ACK \n");
	//check if the packet is not corrupt
	if (check_packet(&packet)) {
		//it wasn't corrupt, check if it is an ACK
		if (packet.acknum == ACK_ID && list_size(a_window.unacked_packets) > 0) {	
			printf("A received an ACK from B \n");	
			//seg fault sometime after this.
			//the packet is an ack, and there are unacked packets.
			int ack_seq_num = packet.seqnum;
			
			int i=0;
			printf("We are going to go through the list of unacked packets \n");
			for (i=0;i<list_size(a_window.unacked_packets);i++) {
				struct pkt* elt = peek(a_window.unacked_packets); // peek at the first unacked packet
				printf("We found an unacked packet! %x\n", elt);
				if (elt->seqnum <= ack_seq_num) {
					printf("Packets seq num is less than ack seq num, removing from window \n");
					//packet has been acked!
					dequeue(a_window.unacked_packets); //remove it.
					free(elt); // free the element now that we are done with it
				}
				else {
					printf("Seq number is higher, no packets to ack. \n");
					//seq number is not less than packet.
					break;
				}
			}
			
			printf("Acknowledged %d packets \n", i);
			
			if (list_size(a_window.unacked_packets) == 0) {
				//no unacked packets left, stopping timer
				printf("Ack'd all packets, stoping timer \n");
				stoptimer(A_ID);
			}
			
			printf("Sending any messages in the buffer, that fit in window \n");
			while (!list_full(a_window.unacked_packets)&& list_size(a_window.buffered_messages) > 0) {
				//while we have free space in the window, and unbuffered messages
				struct msg* message = dequeue(a_window.buffered_messages);
				send_message(message); // send a message in the buffered message queue
				free(message); // free the message now that we are done with it
			}			
			//we have acked all unacked packets  before and uncluding the given acl
		}
	}
	else {
		//we received a corrupt packet, just ignore it
		printf("A received a corrupt packet \n");
	}
}

/* Interupt for A's timer */
A_timerinterrupt()
{
	printf("A Timer Interupt \n");
	//when the timer goes off we resend all unacked packets.
	if (list_size(a_window.unacked_packets) > 0) {
		//resend all unacked packets..
		printf("We have unacked packets, lets resend them \n");
		
		void** list_elt = list_get_all(a_window.unacked_packets);
		void** list_head = list_elt;
		int i;
		for (i=0;i<list_size(a_window.unacked_packets);i++) {
			printf("A is sending unacked packet to B \n");
			tolayer3(A_ID, *((struct pkt*) *list_head));
			list_head++;
		}
		
		free(list_elt); // free the array returned from list_get_all
		
		starttimer(A_ID, A_TIMEOUT);
	}
	else {
		printf("All packets were already ack'd lets do nothing. \n");
	}
}  

/* Initialize A  */
A_init()
{
  a_window.next_seq_num = 0;
  a_window.window_size = A_WINDOW_SIZE;
  a_window.buffered_messages = create_list(MESSAGE_BUFFER_SIZE);
  a_window.unacked_packets = create_list(A_WINDOW_SIZE);
}

/* Called from Network Layer when node B receives a packet,
	From node A in this case 
	@param packet The packet received from the network
*/
B_input(packet)
  struct pkt packet;
{
	printf("B recieved a message! \n");
	//check if the packet is valid
	if (check_packet(&packet)) {
		//printf("Checksums match! No corruption! \n");
		//printf("B next seq %d pkt seq %d \n", b_window.expected_seq_num,packet.seqnum);
		printf("Received packet %d expecting %d \n", packet.seqnum, b_window.expected_seq_num);
		if (b_window.expected_seq_num == packet.seqnum) {
			//we got the packet we were expecting, and it was uncorrupt
			
			if (b_window.last_ack != NULL) {
				free(b_window.last_ack);
			}
			struct pkt* ack_pkt = create_ack(packet.seqnum);
			b_window.last_ack = ack_pkt;
			printf("Sending ACK \n");
			tolayer3(B_ID, *ack_pkt);
			
			recv_window_inc_seq_num(&b_window);
			
			b_window.num_recv++;
			printf("B successfully received %d packets \n", b_window.num_recv);
		}
		else {
			//printf("Resending ack! \n");
			printf("Sending last ACK\n");
			tolayer3(B_ID, *(b_window.last_ack));
		}
		
	}
	else {
		printf("B recieved a corrupt packet \n");
		//lets resend last ack anyways.
		if (b_window.last_ack != NULL) {
			printf("Sending last ACK \n");
			tolayer3(B_ID, *(b_window.last_ack));
		}
	}
}

/* Interupt for B's timer */
B_timerinterrupt()
{
  
}

/* Initizlize B */
B_init()
{
	b_window.expected_seq_num = 0;
	b_window.last_ack = NULL;
	b_window.num_recv = 0;
}


/** MY FUNCTIONS */

void msg_window_inc_seq_num(struct message_window* window) {
	window->next_seq_num ++;
	//lets not use negative seq num, if it over flows, go back to 0
	if (window->next_seq_num < 0) {
		window->next_seq_num = 0;
	}
}

void recv_window_inc_seq_num(struct receiver_window* window) {
	window->expected_seq_num ++;
	//lets not use negative seq num, if it over flows, go back to 0
	if (window->expected_seq_num < 0) {
		window->expected_seq_num = 0;
	}
}

int generate_checksum(struct pkt* packet) {
	int checksum = 0;
	checksum += packet->seqnum;
	checksum += packet->acknum;
	int i=0;
	for (i=0;i<20;i++) {
		checksum += (int) packet->payload[i];
	}
	
	return checksum;
}

int check_packet(struct pkt* packet) {
	int res = 0;
	int checksum = generate_checksum(packet);
	return checksum == packet->checksum;
}


struct pkt* create_ack(int seq_num) {
	struct pkt* packet = (struct pkt*) malloc(sizeof(struct pkt));
	memset(packet, 0, sizeof(struct pkt)); //zero out the struct
	packet->seqnum = seq_num;
	packet->acknum = ACK_ID;
	packet->checksum = generate_checksum(packet);
	return packet;	
}

/** Adds the given value of the given list 
	@param list pointer to the list to add the value to
	@param value pointer to the value to add
	@return 1 if successful, 0 if list was full
*/
int add_to_list(struct list* list, void* value) {
	if (!list_full(list)) {
		*(list->tail) = value;
		list->tail++; // increase the tail
		list->curr_size++;
		if (list->tail >= (list->values + list->max_size)) {
			//wrap the list around
			list->tail = list->values;
		}
	}
}

void** list_get_all(struct list* list) {
	if (list->curr_size == 0) {
		return NULL; // no elements exist in the list
	}
	void** values = (void**) malloc ( sizeof(void*) * list->curr_size);
	void** values_tmp = values;
	void** head_tmp = list->head;
	int i=0;
	for (i=0;i<list->curr_size;i++) {
		*(values_tmp) = *(head_tmp);
		values_tmp++;
		head_tmp++;
		if (head_tmp>= (list->values + list->max_size)) {
			//wrap the head around
			head_tmp = list->values;
		}
	}
	
	return values;
}

/** Returns if the given list is full or not
	@param list Pointer to the list
	@return 1 if full, 0 otherwise
*/

int list_full(struct list* list) {
	return list->curr_size == list->max_size;
}

int list_size(struct list* list) {
	return list->curr_size;
}

/** Returns the first element in the list, but leaves it in the list
*/
void* peek(struct list* list) {
	return *(list->head);
}

/** Removes the first element in the list, and returns it
*/
void* dequeue(struct list* list) {
	void* val = NULL;
	if (list->curr_size > 0) {	//if the list is not empty
		val = *(list->head);
		list->head++;
		
		if (list->head >= (list->values + list->max_size)) {
			//wrap the head around
			list->head = list->values;
		}
		
		list->curr_size--; //decrease the current size
	}
	return val;
}
void send_message(struct msg* message) {
	
	struct pkt* packet = (struct pkt*) malloc (sizeof(struct pkt));
	packet->seqnum = a_window.next_seq_num; // the next seq num
	msg_window_inc_seq_num(&a_window);
	packet->acknum = 0; //no ack status
	strncpy(packet->payload, message->data, 20); //copy over the data		
	//we need to generate a checksum.
	packet->checksum = generate_checksum(packet);		

	add_to_list(a_window.unacked_packets, packet); //add the packet to unacked list
	//lets actualy send the packet
	tolayer3(A_ID, *packet);

	//only start the timer if there are no other unacked packets
	if (list_size(a_window.unacked_packets) == 1) {
		starttimer(A_ID, A_TIMEOUT);	
	}
}

/** Creates a list with the given size
	@param size THe size of the list
	@return pointer to the list
*/

struct list* create_list(int size) {
	struct list* list = (struct list*) malloc (sizeof(struct list));
	
	list->values = (void**) malloc( sizeof(void*) * size);
	
	list->max_size = size;
	list->curr_size = 0;
	list->head = list->values;
	list->tail = list->values;
	
	return list;
}

void free_list(struct list* list) {
	free(list->head);
	free(list);
}



/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
    and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
    interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

struct event {
   float evtime;           /* event time */
   int evtype;             /* event type code */
   int eventity;           /* entity where event occurs */
   struct pkt *pktptr;     /* ptr to packet (if any) assoc w/ this event */
   struct event *prev;
   struct event *next;
 };
struct event *evlist = NULL;   /* the event list */

/* possible events: */
#define  TIMER_INTERRUPT 0  
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2

#define  OFF             0
#define  ON              1
#define   A    0
#define   B    1



int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */ 
int nsimmax = 0;           /* number of msgs to generate, then stop */
float time = 0.000;
float lossprob;            /* probability that a packet is dropped  */
float corruptprob;         /* probability that one bit is packet is flipped */
float lambda;              /* arrival rate of messages from layer 5 */   
int   ntolayer3;           /* number sent into layer 3 */
int   nlost;               /* number lost in media */
int ncorrupt;              /* number corrupted by media*/

main()
{
   struct event *eventptr;
   struct msg  msg2give;
   struct pkt  pkt2give;
   
   int i,j;
   char c; 
  
   init();
   A_init();
   B_init();
   
   while (1) {
        eventptr = evlist;            /* get next event to simulate */
        if (eventptr==NULL)
           goto terminate;
        evlist = evlist->next;        /* remove this event from event list */
        if (evlist!=NULL)
           evlist->prev=NULL;
        if (TRACE>=2) {
           printf("\nEVENT time: %f,",eventptr->evtime);
           printf("  type: %d",eventptr->evtype);
           if (eventptr->evtype==0)
	       printf(", timerinterrupt  ");
             else if (eventptr->evtype==1)
               printf(", fromlayer5 ");
             else
	     printf(", fromlayer3 ");
           printf(" entity: %d\n",eventptr->eventity);
           }
        time = eventptr->evtime;        /* update time to next event time */
        if (nsim==nsimmax)
	  break;                        /* all done with simulation */
        if (eventptr->evtype == FROM_LAYER5 ) {
            generate_next_arrival();   /* set up future arrival */
            /* fill in msg to give with string of same letter */    
            j = nsim % 26; 
            for (i=0; i<20; i++)  
               msg2give.data[i] = 97 + j;
            if (TRACE>2) {
               printf("          MAINLOOP: data given to student: ");
                 for (i=0; i<20; i++) 
                  printf("%c", msg2give.data[i]);
               printf("\n");
	     }
            nsim++;
            if (eventptr->eventity == A) 
               A_output(msg2give);  
             else
               B_output(msg2give);  
            }
          else if (eventptr->evtype ==  FROM_LAYER3) {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i=0; i<20; i++)  
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
	    if (eventptr->eventity ==A)      /* deliver packet by calling */
   	       A_input(pkt2give);            /* appropriate entity */
            else
   	       B_input(pkt2give);
	    free(eventptr->pktptr);          /* free the memory for packet */
            }
          else if (eventptr->evtype ==  TIMER_INTERRUPT) {
            if (eventptr->eventity == A) 
	       A_timerinterrupt();
             else
	       B_timerinterrupt();
             }
          else  {
	     printf("INTERNAL PANIC: unknown event type \n");
             }
        free(eventptr);
        }

terminate:
   printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n",time,nsim);
}



init()                         /* initialize the simulator */
{
  int i;
  float sum, avg;
  float jimsrand();
  
  
   printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
   printf("Enter the number of messages to simulate: ");
   scanf("%d",&nsimmax);
   printf("Enter  packet loss probability [enter 0.0 for no loss]:");
   scanf("%f",&lossprob);
   printf("Enter packet corruption probability [0.0 for no corruption]:");
   scanf("%f",&corruptprob);
   printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
   scanf("%f",&lambda);
   printf("Enter TRACE:");
   scanf("%d",&TRACE);

   srand(9999);              /* init random number generator */
   sum = 0.0;                /* test random number generator for students */
   for (i=0; i<1000; i++)
      sum=sum+jimsrand();    /* jimsrand() should be uniform in [0,1] */
   avg = sum/1000.0;
   if (avg < 0.25 || avg > 0.75) {
    printf("It is likely that random number generation on your machine\n" ); 
    printf("is different from what this emulator expects.  Please take\n");
    printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
    exit(0);
    }

   ntolayer3 = 0;
   nlost = 0;
   ncorrupt = 0;

   time=0.0;                    /* initialize time to 0.0 */
   generate_next_arrival();     /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand() 
{
  double mmm = 2147483647;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
  float x;                   /* individual students may need to change mmm */ 
  x = rand()/mmm;            /* x should be uniform in [0,1] */
  return(x);
}  

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/
 
generate_next_arrival()
{
   double x,log(),ceil();
   struct event *evptr;
   float ttime;
   int tempint;

   if (TRACE>2)
       printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");
 
   x = lambda*jimsrand()*2;  /* x is uniform on [0,2*lambda] */
                             /* having mean of lambda        */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + x;
   evptr->evtype =  FROM_LAYER5;
   if (BIDIRECTIONAL && (jimsrand()>0.5) )
      evptr->eventity = B;
    else
      evptr->eventity = A;
   insertevent(evptr);
} 


insertevent(p)
   struct event *p;
{
   struct event *q,*qold;

   if (TRACE>2) {
      printf("            INSERTEVENT: time is %lf\n",time);
      printf("            INSERTEVENT: future time will be %lf\n",p->evtime); 
      }
   q = evlist;     /* q points to header of list in which p struct inserted */
   if (q==NULL) {   /* list is empty */
        evlist=p;
        p->next=NULL;
        p->prev=NULL;
        }
     else {
        for (qold = q; q !=NULL && p->evtime > q->evtime; q=q->next)
              qold=q; 
        if (q==NULL) {   /* end of list */
             qold->next = p;
             p->prev = qold;
             p->next = NULL;
             }
           else if (q==evlist) { /* front of list */
             p->next=evlist;
             p->prev=NULL;
             p->next->prev=p;
             evlist = p;
             }
           else {     /* middle of list */
             p->next=q;
             p->prev=q->prev;
             q->prev->next=p;
             q->prev=p;
             }
         }
}

printevlist()
{
  struct event *q;
  int i;
  printf("--------------\nEvent List Follows:\n");
  for(q = evlist; q!=NULL; q=q->next) {
    printf("Event time: %f, type: %d entity: %d\n",q->evtime,q->evtype,q->eventity);
    }
  printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
stoptimer(AorB)
int AorB;  /* A or B is trying to stop timer */
{
 struct event *q,*qold;

 if (TRACE>2)
    printf("          STOP TIMER: stopping timer at %f\n",time);
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
 for (q=evlist; q!=NULL ; q = q->next) 
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) { 
       /* remove this event */
       if (q->next==NULL && q->prev==NULL)
             evlist=NULL;         /* remove first and only event on list */
          else if (q->next==NULL) /* end of list - there is one in front */
             q->prev->next = NULL;
          else if (q==evlist) { /* front of list - there must be event after */
             q->next->prev=NULL;
             evlist = q->next;
             }
           else {     /* middle of list */
             q->next->prev = q->prev;
             q->prev->next =  q->next;
             }
       free(q);
       return;
     }
  printf("Warning: unable to cancel your timer. It wasn't running.\n");
}


starttimer(AorB,increment)
int AorB;  /* A or B is trying to stop timer */
float increment;
{

 struct event *q;
 struct event *evptr;

 if (TRACE>2)
    printf("          START TIMER: starting timer at %f\n",time);
 /* be nice: check to see if timer is already started, if so, then  warn */
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
   for (q=evlist; q!=NULL ; q = q->next)  
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) { 
      printf("Warning: attempt to start a timer that is already started\n");
      return;
      }
 
/* create future event for when timer goes off */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + increment;
   evptr->evtype =  TIMER_INTERRUPT;
   evptr->eventity = AorB;
   insertevent(evptr);
} 


/************************** TOLAYER3 ***************/
tolayer3(AorB,packet)
int AorB;  /* A or B is trying to stop timer */
struct pkt packet;
{
 struct pkt *mypktptr;
 struct event *evptr,*q;
 float lastime, x, jimsrand();
 int i;


 ntolayer3++;

 /* simulate losses: */
 if (jimsrand() < lossprob)  {
      nlost++;
      if (TRACE>0)    
	printf("          TOLAYER3: packet being lost\n");
      return;
    }  

/* make a copy of the packet student just gave me since he/she may decide */
/* to do something with the packet after we return back to him/her */ 
 mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
 mypktptr->seqnum = packet.seqnum;
 mypktptr->acknum = packet.acknum;
 mypktptr->checksum = packet.checksum;
 for (i=0; i<20; i++)
    mypktptr->payload[i] = packet.payload[i];
 if (TRACE>2)  {
   printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
	  mypktptr->acknum,  mypktptr->checksum);
    for (i=0; i<20; i++)
        printf("%c",mypktptr->payload[i]);
    printf("\n");
   }

/* create future event for arrival of packet at the other side */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtype =  FROM_LAYER3;   /* packet will pop out from layer3 */
  evptr->eventity = (AorB+1) % 2; /* event occurs at other entity */
  evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */
/* finally, compute the arrival time of packet at the other end.
   medium can not reorder, so make sure packet arrives between 1 and 10
   time units after the latest arrival time of packets
   currently in the medium on their way to the destination */
 lastime = time;
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
 for (q=evlist; q!=NULL ; q = q->next) 
    if ( (q->evtype==FROM_LAYER3  && q->eventity==evptr->eventity) ) 
      lastime = q->evtime;
 evptr->evtime =  lastime + 1 + 9*jimsrand();
 


 /* simulate corruption: */
 if (jimsrand() < corruptprob)  {
    ncorrupt++;
    if ( (x = jimsrand()) < .75)
       mypktptr->payload[0]='Z';   /* corrupt payload */
      else if (x < .875)
       mypktptr->seqnum = 999999;
      else
       mypktptr->acknum = 999999;
    if (TRACE>0)    
	printf("          TOLAYER3: packet being corrupted\n");
    }  

  if (TRACE>2)  
     printf("          TOLAYER3: scheduling arrival on other side\n");
  insertevent(evptr);
} 

tolayer5(AorB,datasent)
  int AorB;
  char datasent[20];
{
  int i;  
  if (TRACE>2) {
     printf("          TOLAYER5: data received: ");
     for (i=0; i<20; i++)  
        printf("%c",datasent[i]);
     printf("\n");
   }
  
}
