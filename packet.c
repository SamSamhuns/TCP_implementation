/*
 * Modified by Samridha sms1198 and Teona tr1348
 * Nabil Rahiman
 * NYU Abudhabi
 * email: nr83@nyu.edu
 */
#include <stdlib.h>
#include"packet.h"

static tcp_packet zero_packet = {.hdr={0}};
/*
 * create tcp packet with header and space for data of size len
 */
tcp_packet* make_packet(int len)
{
    tcp_packet *pkt;
    pkt = malloc(TCP_HDR_SIZE + len);

    *pkt = zero_packet;
    pkt->hdr.data_size = len;
    return pkt;
}

int get_data_size(tcp_packet *pkt)
{
    return pkt->hdr.data_size;
}


// Adds a tcp_packet buffer to the end of the list
void push ( nodepkt * head, tcp_packet *buffer_pkt ) 
{
	nodepkt * current = head;
	while ( current->next_nodepkt != NULL ) {
		current = current->next_nodepkt;
	}

	current->next_nodepkt = malloc(sizeof(nodepkt));
	current->next_nodepkt->buffer_pkt = (tcp_packet*) buffer_pkt;
	current->next_nodepkt->next_nodepkt = NULL;
}; 

// Removes the first item from the list
// And returns the packet that was initially stored at the head of the linked list 
int pop (nodepkt ** head) {
    int ret_val = -1;
    nodepkt *following_nodepkt = NULL;

    if (*head == NULL) {
        return -1;
    }

    following_nodepkt= (*head)->next_nodepkt;
    ret_val = (*head)-> buffer_pkt;
    free(*head);
    *head = following_nodepkt;

    return ret_val;
}
