/*
 * Starter code modified by Samridha sms1198 and Teona tr1328
 * Nabil Rahiman
 * NYU Abudhabi
 * email: nr83@nyu.edu
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <assert.h>

#include "common.h"
#include "packet.h"
#define buffer_size 1000

tcp_packet *recvpkt;
tcp_packet *sndpkt;

tcp_packet *receiver_buffer[buffer_size]; // buffer array
void swap(tcp_packet* *xp, tcp_packet* *yp);
void bubbleSort(tcp_packet* arr[], int n);



int main(int argc, char **argv) {
    int sockfd; /* socket */
    int portno; /* port to listen on */
    int clientlen; /* byte size of client's address */
    struct sockaddr_in serveraddr; /* server's addr */
    struct sockaddr_in clientaddr; /* client addr */
    int optval; /* flag value for setsockopt */
    FILE *fp;
    char buffer[MSS_SIZE];
    struct timeval tp;


    int ipp = 0;
    while ( ipp < buffer_size ) {
        receiver_buffer[ipp] = NULL;
        ipp++;
    }
    int base_pointer = 0; // position of pointer to the current out of order packet
    int pkt_add_pointer = 0;

    int next_seqno = 0; // Variable that holds the value for the current required seqno

    /* 
     * check command line arguments 
     */
    if (argc != 3) {
        fprintf(stderr, "usage: %s <port> FILE_RECVD\n", argv[0]);
        exit(1);
    }
    portno = atoi(argv[1]);

    fp  = fopen(argv[2], "w");
    if (fp == NULL) {
        error(argv[2]);
    }

    /* 
     * socket: create the parent socket 
     */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* setsockopt: Handy debugging trick that lets 
     * us rerun the server immediately after we kill it; 
     * otherwise we have to wait about 20 secs. 
     * Eliminates "ERROR on binding: Address already in use" error. 
     */
    optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
            (const void *)&optval , sizeof(int));

    /*
     * build the server's Internet address
     */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)portno);

    /* 
     * bind: associate the parent socket with a port 
     */
    if (bind(sockfd, (struct sockaddr *) &serveraddr, 
                sizeof(serveraddr)) < 0) 
        error("ERROR on binding");

    /* 
     * main loop: wait for a datagram, then echo it
     */
    VLOG(DEBUG, "epoch time, bytes received, sequence number");

    clientlen = sizeof(clientaddr);
    while (1) {
        /*
         * recvfrom: receive a UDP datagram from a client
         */
        //VLOG(DEBUG, "waiting from server \n");
        if (recvfrom(sockfd, buffer, MSS_SIZE, 0,
                (struct sockaddr *) &clientaddr, (socklen_t *)&clientlen) < 0) {
            error("ERROR in recvfrom");
        }
        recvpkt = (tcp_packet *) buffer;
        assert(get_data_size(recvpkt) <= DATA_SIZE);

        // ***********************************************************************************************************************
        // NOTE TO NABIL
        // On rare occurences, the receiver does not exit because it never receives a recvpkt with a data_size of 0

        if ( recvpkt->hdr.data_size == 0 ) {
            VLOG(INFO, "End Of File has been reached");
            fclose(fp);
            break;
        }

        /* 
         * sendto: ACK back to the client 
         */
        
        gettimeofday(&tp, NULL);
        VLOG(DEBUG, "%lu, %d, %d", tp.tv_sec, recvpkt->hdr.data_size, recvpkt->hdr.seqno, recvpkt->hdr.ackno);

        // Checking if packets are received in order, if not, send ACK for the original package
        //printf("next seqno = %i and recvpkt->hdr.seqno = %i\n", next_seqno, recvpkt->hdr.seqno );
        if ( next_seqno != recvpkt->hdr.seqno )
        {
            sndpkt = make_packet(0);
            // Ask for the same old packet
            sndpkt->hdr.ackno = next_seqno;
            sndpkt->hdr.ctr_flags = ACK;
            if (sendto(sockfd, sndpkt, TCP_HDR_SIZE, 0, 
                    (struct sockaddr *) &clientaddr, clientlen) < 0) {
                error("ERROR in sendto");
            }


            receiver_buffer[pkt_add_pointer] = ( tcp_packet* ) recvpkt;

            int iq =0;
            while ( iq <buffer_size) {
                if ( receiver_buffer[iq] == NULL )
                {
                    break;
                }
                printf("hdr seqno values %i \n", receiver_buffer[iq]->hdr.seqno );
                iq++;
            }

            if ( receiver_buffer[pkt_add_pointer] != NULL )
            {
                printf("THIS IS FOR THE ASSINGMENT %i \n",receiver_buffer[pkt_add_pointer]->hdr.seqno );
            }
            pkt_add_pointer++;
            //printf("THIS IS FOR THE ASSINGMENT %i \n",receiver_buffer[pkt_add_pointer]->hdr.seqno );
            //bubbleSort( receiver_buffer, buffer_size);

    

            /*
            // This should only run once
            // Adds the first packet to the receiver buffer
            if ( pkt_add_pointer == 0 )
            {
                receiver_buffer[pkt_add_pointer] = (tcp_packet*)recvpkt;
                pkt_add_pointer ++;
            }

            int i = 0;
            int duplicate = 0;
            while ( 1 ) {
                if ( receiver_buffer[i]->hdr.seqno == recvpkt->hdr.seqno )
                {
                    duplicate = 1;
                }
                if ( receiver_buffer[i] == NULL ) {
                    break;
                }
                i++;
            }
            if ( duplicate == 0)
            {
                receiver_buffer[pkt_add_pointer] = (tcp_packet*)recvpkt;
                pkt_add_pointer++;
                bubbleSort( receiver_buffer, buffer_size);
            }*/

            VLOG(INFO, "Out of order Packet\n\n");
            //printf("DROP PACKAGE LOOP next seqno = %i and recvpkt->hdr.seqno = %i and sndpck hdr ACK NO %i \n\n", next_seqno, recvpkt->hdr.seqno,sndpkt->hdr.ackno );
        }
        // If the packet is received in order
        else
        {
            fseek(fp, recvpkt->hdr.seqno, SEEK_SET);
            fwrite(recvpkt->data, 1, recvpkt->hdr.data_size, fp);

            /*
            int i = base_pointer;
            while ( receiver_buffer[i] != NULL ) {


            }
            */




            sndpkt = make_packet(0);
            sndpkt->hdr.ackno = recvpkt->hdr.seqno + recvpkt->hdr.data_size;
            next_seqno = sndpkt->hdr.ackno; // Update next_seqno to the hdr.ackno

            //printf("SEND PACKAGE LOOP next seqno = %i and recvpkt->hdr.seqno = %i and sndpck hdr ACK NO %i \n\n", next_seqno, recvpkt->hdr.seqno,sndpkt->hdr.ackno );
            sndpkt->hdr.ctr_flags = ACK;
            if (sendto(sockfd, sndpkt, TCP_HDR_SIZE, 0, 
                    (struct sockaddr *) &clientaddr, clientlen) < 0) {
                error("ERROR in sendto");
            }
        }  
    }
    int ip =0;
    while ( ip <buffer_size) {
        if ( receiver_buffer[ip] == NULL )
        {
            break;
        }
        printf("hdr seqno values %i \n", receiver_buffer[ip]->hdr.seqno );
        ip++;

    }
    return 0;
}


void swap(tcp_packet* *xp, tcp_packet* *yp)
{
    tcp_packet *temp = *xp;
    *xp = *yp;
    *yp = temp;
}
 
// A function to implement bubble sort
void bubbleSort(tcp_packet* arr[], int n)
{
   int i, j;
   for (i = 0; i < n-1; i++)  
       // Last i elements are already in place 
       for (j = 0; j < n-i-1; j++) {
           if ( arr[j] == NULL || arr[j+1] == NULL) {
             continue;
           }

           if (arr[j]->hdr.seqno > arr[j+1]->hdr.seqno) {
              swap(&arr[j], &arr[j+1]);
           }
       }
}
