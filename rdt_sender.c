/*
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

#include"packet.h"
#include"common.h"

#define STDIN_FD    0
#define RETRY  120 //milli second 
#define window_size 10

int next_seqno=0;
int send_base=0;
//int window_size = 10;

int sockfd, serverlen;
struct sockaddr_in serveraddr;
struct itimerval timer; 
tcp_packet *sndpkt;
tcp_packet *recvpkt;
sigset_t sigmask; 

// Buffer to hold the send window of window_size    
tcp_packet *send_window[window_size];  

void resend_packets(int sig)
{
    if (sig == SIGALRM)
    {
        //Resend all packets range between 
        //sendBase and nextSeqNum
        VLOG(INFO, "Timout happend");
        int i =0;
        while ( i < window_size )
        {
            if ( send_window[i] == NULL )
            {
                continue;
            }
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
    char buffer[DATA_SIZE];
    FILE *fp;

    // Variable to make sure acks are in order
    int next_ackno = 0;

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


    // Assigning the array of tcp_packets in the buffer window to NULL
    int j = 0;                                                                              
    while ( j < window_size)                                                                
    {                                                                                                                                                    
        send_window[j] = NULL;                                 
        j++;                                                                                
    }     

    //*****************************************************************************************
    // TEST CODE TO CHECK IF THE BUFFER OF VARIABLE PACKETS HAS BEEN CREATED                  *
    /*                                                                                        *
    int ite = 0;                                                                              *
    while ( ite < window_size)                                                                *
    {                                                                                         *
        sndpkt = make_packet(0);                                                              *
        send_window[ite] = (tcp_packet*)sndpkt;//(tcp_packet*)                                *
        printf("Priting the ackno of creatd packets %i\n", send_window[ite]->hdr.ackno );     *
        ite++;                                                                                *
    }                                                                                         *
    *///                                                                                      *
    //*****************************************************************************************

    assert(MSS_SIZE - TCP_HDR_SIZE > 0);

    //Stop and wait protocol

    init_timer(RETRY, resend_packets);
    next_seqno = 0;
    int next_seqno_buffer = 0;

    /*
    while (1)
    {*/

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
                break;
            }
            next_seqno_buffer = next_seqno;
            next_seqno = next_seqno_buffer + len;
            sndpkt = make_packet(len);
            memcpy(sndpkt->data, buffer, len);
            sndpkt->hdr.seqno = next_seqno_buffer;

            send_window[i] = (tcp_packet *)sndpkt;
            i++;

            VLOG(DEBUG, "Sending packet %d to %s", 
            next_seqno_buffer, inet_ntoa(serveraddr.sin_addr));
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

        int loop_lim = 0;
        //Wait for ACK
        do {
            start_timer();
            //ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
            //struct sockaddr *src_addr, socklen_t *addrlen);

            if(recvfrom(sockfd, buffer, MSS_SIZE, 0,
                        (struct sockaddr *) &serveraddr, (socklen_t *)&serverlen) < 0)
            {
                error("recvfrom");
            }

            recvpkt = (tcp_packet *)buffer;
            printf("%d \n", get_data_size(recvpkt));
            assert(get_data_size(recvpkt) <= DATA_SIZE);


            if ( recvpkt->hdr.ackno >= next_ackno )
            {
                while ( next_ackno <= recvpkt->hdr.ackno )
                    {

                    len = fread(buffer, 1, DATA_SIZE, fp);
                    if ( len <= 0 )
                    {
                        VLOG(INFO, "End of File has been reached");
                        sndpkt = make_packet(0);
                        sendto(sockfd, sndpkt, TCP_HDR_SIZE,  0,
                                (const struct sockaddr *)&serveraddr, serverlen);
                        return 0;
                        break;
                    }

                    
                    loop_lim  = next_ackno  + len;
                    next_ackno = loop_lim;
                    sndpkt = make_packet(len);
                    memcpy(sndpkt->data, buffer, len);
                    sndpkt->hdr.seqno = next_ackno;

                    send_window[(next_seqno/DATA_SIZE) % window_size] = (tcp_packet *)sndpkt;

                    VLOG(DEBUG, "Sending packet %d to %s", 
                    next_ackno , inet_ntoa(serveraddr.sin_addr));
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
                    next_seqno = next_seqno + len;

                    }
            }
            stop_timer();
            /*resend pack if dont recv ack */
           
        } while( recvpkt->hdr.ackno != next_seqno && ( next_ackno < next_seqno ));
        free(sndpkt);
        
    /*}*/

    return 0;

}



