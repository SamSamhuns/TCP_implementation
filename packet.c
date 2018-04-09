/*
 * Modified by Samridha sms1198 and Teona tr1348
 * Nabil Rahiman
 * NYU Abudhabi
 * email: nr83@nyu.edu
 */
#include <stdlib.h>
#include <stdio.h>
#include "packet.h"

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

void push ( nodepkt *head, tcp_packet *new_data_nodepkt ) 
{

    printf("tcp recv packet hdr seq no is %i\n", new_data_nodepkt->hdr.seqno  );
    if ( head->next_nodepkt == NULL )    {
        head->next_nodepkt = malloc(sizeof(nodepkt));
        head->next_nodepkt->data_nodepkt = new_data_nodepkt;
        head->next_nodepkt->next_nodepkt = NULL;
        printf("Inserted node packets hdr seq no is %i\n", head->next_nodepkt->data_nodepkt->hdr.seqno  );
        return;
    }
    if ( new_data_nodepkt->hdr.seqno == 0 )
    {
        return;
    }
    printf("Head's next packets seq no %i\n", head->next_nodepkt->data_nodepkt->hdr.seqno  );

    nodepkt *current = ( nodepkt* ) malloc(sizeof(nodepkt));
    current->data_nodepkt = new_data_nodepkt;
    current->next_nodepkt = head->next_nodepkt;
    printf("current packet outisde WHILE hdr seq no is %i\n", current->data_nodepkt->hdr.seqno  );

    nodepkt *new = ( nodepkt* ) malloc(sizeof(nodepkt));

    while ( current != NULL ) {
        printf("Outside IF STATEMNT Current node's packets hdr seq no is %i\n", current->data_nodepkt->hdr.seqno  );
        if ( current->next_nodepkt == NULL )
        {
            current->next_nodepkt = new;
            new->data_nodepkt = ( tcp_packet* ) new_data_nodepkt;
            printf("Final CURRENT location node packets hdr seq no is %i\n", current->next_nodepkt->data_nodepkt->hdr.seqno  );
            printf("Final INSERTED location node packets hdr seq no is %i\n", new->data_nodepkt->hdr.seqno  );
            new->next_nodepkt = NULL;
            return;
        }
        if ( (current->next_nodepkt->data_nodepkt->hdr.seqno) > new_data_nodepkt->hdr.seqno )
        {
            new->next_nodepkt = current->next_nodepkt;
            new->data_nodepkt = ( tcp_packet* ) new_data_nodepkt;
            printf("CURRENT location node packets hdr seq no is %i\n", current->next_nodepkt->data_nodepkt->hdr.seqno  );
            printf("INSERTED location node packets hdr seq no is %i\n", new->data_nodepkt->hdr.seqno  );
            current->next_nodepkt = new;
            return;
        }
        current = current->next_nodepkt;
    }
    printf("ERROR: Control should not reach here\n");
    return;
    /**********************************************************************************************************************************

    nodepkt *current; 
    nodepkt *temp; 
    current = malloc(sizeof(nodepkt));
    temp = malloc(sizeof(nodepkt));

    current->data_nodepkt = new_data_nodepkt;
    current->next_nodepkt = NULL;

    temp = ( nodepkt* )head;

    if ( head->next_nodepkt == NULL ) {
        current->next_nodepkt = head->next_nodepkt;
        head->next_nodepkt = current;
        printf("Printing head's next_nodepkt seq no %i\n", head->next_nodepkt->data_nodepkt->hdr.seqno );
        return;
    }

    printf("Sequence number of head next %i\n", head->next_nodepkt->data_nodepkt->hdr.seqno  );
    printf("000000   Printing current seq no %i\n", current->data_nodepkt->hdr.seqno );
    printf("000000   Printing temp's next node seq no%i\n", temp->next_nodepkt->data_nodepkt->hdr.seqno  );
    while ( current->data_nodepkt->hdr.seqno > temp->next_nodepkt->data_nodepkt->hdr.seqno ) {
        temp = temp->next_nodepkt;
        printf("1111111  Printing temp's next node seq no i %i\n", temp->next_nodepkt->data_nodepkt->hdr.seqno  );
        printf("1111111  Printing current seq no %i\n", current->data_nodepkt->hdr.seqno );
        if ( temp-> next_nodepkt == NULL ) {
            break;
        }
    }
    current->next_nodepkt = temp->next_nodepkt;
    temp->next_nodepkt = current;

    **********************************************************************************************************************************/


    /*Original Code
	nodepkt *current = head;
	while ( current->next_nodepkt != NULL ) {
		current = current->next_nodepkt;
	}
	current->next_nodepkt = malloc(sizeof(nodepkt));
	current->next_nodepkt->data_nodepkt = (tcp_packet*) data_nodepkt;
	current->next_nodepkt->next_nodepkt = NULL;*/
    
};



// Removes the first item from the list
// And returns the packet that was initially stored at the head of the linked list 
int pop (nodepkt ** head) {
    //tcp_packet *ret_val;
    nodepkt *following_nodepkt = NULL;

    if (*head == NULL) {
        return -1;
    }

    following_nodepkt= (*head)->next_nodepkt;
    //ret_val = (*head)-> data_nodepkt;
    free(*head);
    *head = following_nodepkt;

    //return ret_val;
    return 1;
}

// Bubble sort for the linked list
