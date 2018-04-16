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
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>

#include "packet.h"
#include "common.h"

#define STDIN_FD 0
#define RETRY 120 //milli second 
#define max_window_size 1000 // Setting the max_window_size to 1000 as a window_size CONST
int window_size = 1;
int dup_ack_seqno = 0; // To check if duplciate ackno has been sent from the receiver

int next_seqno=0;
int send_base=0;

int sockfd, serverlen;
struct sockaddr_in serveraddr;
struct itimerval timer; 
tcp_packet *sndpkt;
tcp_packet *recvpkt;
sigset_t sigmask; 

// Buffer to hold the send window of window_size i.e. array of tcp_packets  
tcp_packet *send_window[max_window_size];  

void resend_packets(int sig)
{
    if (sig == SIGALRM)
    {
        //*****************************************************************************************************************
        //The new packets are replaced in the send_window buffer later in the main code and no index pointer is required

        //Resend all packets range between 
        //sendBase and nextSeqNum
        VLOG(INFO, "Timout happend");
        int i =0;
        while ( i < window_size )
        {
            //printf("Are the packets always null\n");
            // If Packet is NULL, do not send packet
            if ( send_window[i] == NULL )
            {
                i++;
                continue;   
            }
            //printf(" Are we looping forever here with window packet seq no %i and data size %i\n", send_window[i]->hdr.seqno, send_window[i]->hdr.data_size  );
            sndpkt = (tcp_packet*) send_window[i];
            if(sendto(sockfd, sndpkt, TCP_HDR_SIZE + get_data_size(sndpkt), 0, 
                    ( const struct sockaddr *)&serveraddr, serverlen) < 0)
            {
                error("sendto");
            };  
            i++;
        }

    }
}

void start_timer()
{
    sigprocmask(SIG_UNBLOCK, &sigmask, NULL);
    setitimer(ITIMER_REAL, &timer, NULL);
}


void stop_timer()
{
    sigprocmask(SIG_BLOCK, &sigmask, NULL);
}

/*
 * init_timer: Initialize timeer
 * delay: delay in milli seconds
 * sig_handler: signal handler function for resending unacknoledge packets
 */
void init_timer(int delay, void (*sig_handler)(int)) 
{
    signal(SIGALRM, resend_packets);
    timer.it_interval.tv_sec = delay / 1000;    // sets an interval of the timer
    timer.it_interval.tv_usec = (delay % 1000) * 1000;  
    timer.it_value.tv_sec = delay / 1000;       // sets an initial value
    timer.it_value.tv_usec = (delay % 1000) * 1000;

    sigemptyset(&sigmask);
    sigaddset(&sigmask, SIGALRM);
}


int main (int argc, char **argv)
{
    int portno, len;
    int next_seqno;
    char *hostname;
    char buffer[DATA_SIZE]; // Send packet buffer
    char recv_buffer[DATA_SIZE]; // Receive packet buffer
    int dup_num = 0; // Variable for recording no of duplicate acks
    int file_pointer = 0; // Keeps track of pointer that reads from the file
    //int end_of_array = 0; // To denote the end of the sender buffer array i.e. it is a seqno
    FILE *fp;

    /* check command line arguments */
    if (argc != 4) {
        fprintf(stderr,"usage: %s <hostname> <port> <FILE>\n", argv[0]);
        exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);
    fp = fopen(argv[3], "r");
    if (fp == NULL) {
        error(argv[3]);
    }

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");


    /* initialize server server details */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serverlen = sizeof(serveraddr);

    /* covert host into network byte order */
    if (inet_aton(hostname, &serveraddr.sin_addr) == 0) {
        fprintf(stderr,"ERROR, invalid host %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(portno);

    assert(MSS_SIZE - TCP_HDR_SIZE > 0);

    //Stop and wait protocol

    init_timer(RETRY, resend_packets);
    next_seqno = 0;
    send_base = 0;
    int send_base_initial = 0; // base var to keep track of the base send for the first window_size packets
 
    // Send the first window_size packets
    int i = 0;
    while ( i < window_size)
    {
        len = fread(buffer, 1, DATA_SIZE, fp);
        if ( len <= 0 )
        {
            VLOG(INFO, "End of File has been reached");
            sndpkt = make_packet(0);
            sendto(sockfd, sndpkt, TCP_HDR_SIZE,  0,
                    (const struct sockaddr *)&serveraddr, serverlen);
            return 0;
        }
        send_base_initial = next_seqno;
        next_seqno = send_base_initial + len;
        //end_of_array = next_seqno;
        sndpkt = make_packet(len);
        memcpy(sndpkt->data, buffer, len);
        sndpkt->hdr.seqno = send_base_initial;

        // The packet is uploaded to the send_window bufferz
        send_window[i] = (tcp_packet *)sndpkt;
        i++;

        VLOG(DEBUG, "Sending packet %d to %s", 
        send_base_initial, inet_ntoa(serveraddr.sin_addr));
        /*
         * If the sendto is called for the first time, the system will
         * will assign a random port number so that server can send its
         * response to the src port.
         */
        if(sendto(sockfd, sndpkt, TCP_HDR_SIZE + get_data_size(sndpkt), 0, 
                    ( const struct sockaddr *)&serveraddr, serverlen) < 0)
        {
            error("sendto");
        }

    }

    // Main code for traversing the file and sending all the packets
    int eof_reach = 0; // End of file flag to check if the end of file has been reached in the sender's fread pointer
    //Wait for ACK
    do {
        start_timer();
        //ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
        //struct sockaddr *src_addr, socklen_t *addrlen);
        if(recvfrom(sockfd, recv_buffer, MSS_SIZE, 0,
                    (struct sockaddr *) &serveraddr, (socklen_t *)&serverlen) < 0)
        {
            error("recvfrom");
        }
        recvpkt = (tcp_packet *) recv_buffer;
        printf("%d \n", get_data_size(recvpkt));
        assert(get_data_size(recvpkt) <= DATA_SIZE);

        //Test for triple duplicate acks
        if ( dup_ack_seqno == recvpkt->hdr.ackno )
        {
            dup_num += 1;
        }
        dup_ack_seqno = recvpkt->hdr.ackno;
        // If three duplicate acknoledgements have been received 
        if ( dup_num == 3 )
        {
            dup_num = 0;
            //printf("3 duplicate acks forced timeout\n");
            // If three duplicate 
            int iter = 0;
            //printf("The window size is %d\n", window_size );
            while ( iter < window_size ) {
                //printf("send_window's hdr seqno is %i \n", send_window[iter]->hdr.seqno );
                if ( recvpkt->hdr.ackno == send_window[iter]->hdr.seqno )
                {
                    //printf("HELLO\n");
                    sndpkt = make_packet(send_window[iter]->hdr.data_size);
                    sndpkt = (tcp_packet*) send_window[iter];
                    if(sendto(sockfd, sndpkt, TCP_HDR_SIZE + get_data_size(sndpkt), 0, 
                        ( const struct sockaddr *)&serveraddr, serverlen) < 0)
                    {
                        error("sendto");
                    }
                    //printf("DOES 00111111111\n");
                    //end_of_array = send_base + ;
                    break;                    
                }
                //printf("DOES this run?\n");
                iter++;
            }
            //printf("Before the continue\n");
            //window_size = 1;
            continue;         
        }
        printf("recvpkt ackno = %i, send_base =  %i\n", recvpkt->hdr.ackno, send_base  );

        
        if ( recvpkt->hdr.ackno >= send_base ) {
            //printf("Inside this if statement\n");
            file_pointer = recvpkt->hdr.ackno;
            while ( send_base < recvpkt->hdr.ackno ) {

                //printf("SEGFAULT 0\n");
                fseek(fp, file_pointer, SEEK_SET);
                //printf("SEGFAULT 1\n");
                len = fread(buffer, 1, DATA_SIZE, fp);
                //printf("SEGFAULT 3\n");

                printf("%i\n", len );
                // If len = 0 and all of file has been read, set the eof_reach variable to 0
                if ( len <= 0 )
                {
                    eof_reach = 1;
                }
                // if eof has been reached and the final packet has been acknowledged 
                if ( eof_reach == 1 && recvpkt->hdr.ackno == next_seqno )
                {
                    VLOG(INFO, "End of File has been reached");
                    sndpkt = make_packet(0);
                    sendto(sockfd, sndpkt, TCP_HDR_SIZE,  0,
                        (const struct sockaddr *)&serveraddr, serverlen);
                    return 0;
                }

                //printf("Inside if statement rcvpck ackno = %i, send_base =  %i\n",recvpkt->hdr.ackno, send_base );
                send_base += len;
                sndpkt = make_packet(len);
                memcpy(sndpkt->data, buffer, len);
                sndpkt->hdr.seqno = next_seqno;
                //end_of_array += len; 
                file_pointer += len;

                //printf("After sndpack created statement rcvpck ackno = %i, send_base =  %i\n",recvpkt->hdr.ackno, send_base );
                // if eof has been reached then do not re-assign send_window packets with sndpcks
                if ( eof_reach != 1)
                {
                    window_size++;
                    send_window[(next_seqno/DATA_SIZE) % window_size] = (tcp_packet *)sndpkt;
                }

                VLOG(DEBUG, "Sending packet %d to %s\n", 
                send_base , inet_ntoa(serveraddr.sin_addr));
                
                // if eof has been reached set send_base to recvpkt's ackno
                if ( eof_reach == 1)
                {
                    send_base = recvpkt->hdr.ackno;
                }

                // This will ensure if eof has reached then do not send any more packets but wait for timeouts and resend packets
                // If eof has not been reached that then continue sending newly read packets
                if ( eof_reach != 1)
                {
                    if(sendto(sockfd, sndpkt, TCP_HDR_SIZE + get_data_size(sndpkt), 0, 
                                ( const struct sockaddr *)&serveraddr, serverlen) < 0)
                    {
                        error("sendto");
                    }
                    next_seqno += len;
                }
            }
        }
        stop_timer();
    } while(1);
    free(sndpkt);
    // Control should never reach this
    return 0;
}
