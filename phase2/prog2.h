#ifndef PROG2_H
#define PROG2_H

/** The size of the window that A will use, go back N, using 8 atm*/
#define A_WINDOW_SIZE (8)

/** The size of the message buffer */
#define MESSAGE_BUFFER_SIZE (50)

/** IDs for each node */
#define A_ID (0)
#define B_ID (1)

/** Constant for ACK */
#define ACK_ID (1)

#define A_TIMEOUT (50.0f)

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
	int next_seq_num;	//the next sequence number	
	int max_seq_num; // the highest possible seq number
	int window_size;	
	struct list* buffered_messages;
	struct list* unacked_packets;
};	


struct receiver_window {
	int expected_seq_num; // the seq num we are expecting.
	struct pkt* last_ack; // pointer to the last ack message sent.	
	int num_recv; // number of pkts succesfuly rcvd
};

/** List structure for message and unacked packets window
*/
struct list {

	void** values; // pointer to an array of pointers to values
	
	int curr_size; // the amount of elements currently in the list
	int max_size; // the maximum size of the list
	void** head; // pointer to the current head
	void** tail; // the pointer to the first free spot in the list.
	
};

/** Creates a list with the given size
	@param size THe size of the list
	@return pointer to the list
*/

struct list* create_list(int size);

/** Frees the memory allocated by the list, 
	does NOT free memory used by list elements
	@param list Poointer to the list to free
*/

/** Returns the first element in the list, but leaves it in the list
*/
void* peek(struct list* list);

/** Removes the first element in the list, and returns it
*/
void* dequeue(struct list* list);

void free_list(struct list* list);

/** Returns the size of the list
*/

int list_size(struct list* list);

/** Adds the given value of the given list 
	@param list pointer to the list to add the value to
	@param value pointer to the value to add
	@return 1 if successful, 0 if list was full
*/
int add_to_list(struct list* list, void* value);

/** Returns if the given list is full or not
	@param list Pointer to the list
	@return 1 if full, 0 otherwise
*/

int list_full(struct list* list);

/** Returns an array containing all of the elements in the list,
	@param list pointer to the list to return the elements from
*/
void** list_get_all(struct list* list);

/** Increments the seq number for the given message window, in this case alternated
	between 0 and 1
	@param window Pointer to the message window to increase the seq number in
*/

void msg_window_inc_seq_num(struct message_window* window);

/** Increments the seq number for the given receiver window, in this case alternated
	between 0 and 1
	@param window Pointer to the message window to increase the seq number in
*/

void recv_window_inc_seq_num(struct receiver_window* window);

/** Creates a checksum for the given packet
	@param packet Pointer to the packet to create the checksum for
	@return The integer checksum value
*/

int generate_checksum(struct pkt* packet);

/** Checks if the packet's checksums match
	@param packet Pointer to the packet to check
	@return 1 if they match, 0 otherwise
*/

int check_packet(struct pkt* packet);

/** Sends the given message to B */

void send_message(struct msg* message);

/** Sends the given packet to the application layer
*/

void to_application_layer(struct pkt* packet);


/** Creates an ack packet for the given sequence number */
struct pkt* create_ack(int seq_num);

/** Creates a nack packet for the given sequence number */

struct pkt* create_nack(int seq_num);

#endif
