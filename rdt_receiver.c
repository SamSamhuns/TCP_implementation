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

tcp_packet *recvpkt;
tcp_packet *sndpkt;

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

    int window_size;
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


    nodepkt *head = NULL;
    head = malloc(sizeof(nodepkt));
    // if there is an error with malloc
    if (head == NULL ) {
        return 1;
    }

    head->buffer_pkt = make_packet(0);
    head->next_nodepkt = NULL;



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
            VLOG(INFO, "Dropping out of order Packet\n\n");
            //printf("DROP PACKAGE LOOP next seqno = %i and recvpkt->hdr.seqno = %i and sndpck hdr ACK NO %i \n\n", next_seqno, recvpkt->hdr.seqno,sndpkt->hdr.ackno );
        }
        // If the packet is received in order
        else
        {
            fseek(fp, recvpkt->hdr.seqno, SEEK_SET);
            fwrite(recvpkt->data, 1, recvpkt->hdr.data_size, fp);
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
    return 0;
}
