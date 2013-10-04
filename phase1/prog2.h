#ifndef PROG2_H
#define PROG2_H

/** The size of the window that A will use, alt bit so 1 */
#define A_WINDOW_SIZE (1)

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
	int next_seq_num;	
	int num_outstanding;	
	int window_size;	
	struct pkt* outstanding_packets[A_WINDOW_SIZE];	
};	

struct receiver_window {
	int expected_seq_num; // the seq num we are expecting.
	struct pkt* last_ack; // pointer to the last ack message sent.
	
	int num_recv; // number of pkts succesfuly rcvd
};

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

/** Sends the given packet to the application layer
*/

void to_application_layer(struct pkt* packet);


/** Creates an ack packet for the given sequence number */
struct pkt* create_ack(int seq_num);

/** Creates a nack packet for the given sequence number */

struct pkt* create_nack(int seq_num);

#endif
