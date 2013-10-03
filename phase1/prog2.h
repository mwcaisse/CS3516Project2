#ifndef PROG2_H
#define PROG2_H

/** The size of the window that A will use, alt bit so 1 */
#define A_WINDOW_SIZE (1)

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg {
  char data[20];
  };


/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
   int seqnum;  // seq number (if ack/nak, this is the number)
   int acknum;  // 1 is ack, -1 is nak, 0 is niether
   int checksum;
   char payload[20];
    };

/** The message window that will be used by A, keeps track
	of the unack'd packets 
*/
struct message_window {
	int next_seq_num;	
	int num_outstanding;	
	int window_size;	
	struct msg outstanding_msg[A_WINDOW_SIZE];	
};	


void window_inc_seq_num(struct message_window* window);

#endif
