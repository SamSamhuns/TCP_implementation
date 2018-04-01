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

#include"packet.h"
#include"common.h"

#define STDIN_FD 0
#define RETRY 120 //milli second 
#define window_size 10 // Setting the window_size to 10 as a window_size CONST

int next_seqno=0;
int send_base=0;

int sockfd, serverlen;
struct sockaddr_in serveraddr;
struct itimerval timer; 
tcp_packet *sndpkt;
tcp_packet *recvpkt;
sigset_t sigmask; 

// Buffer to hold the send window of window_size i.e. array of tcp_packets  
tcp_packet *send_window[window_size];  

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
            // If Packet is NULL, do not send packet
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
    char buffer[DATA_SIZE]; // Send packet buffer
    char recv_buffer[DATA_SIZE]; // Receive packet buffer
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
    int send_base = 0;
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
            sndpkt = make_packet(len);
            memcpy(sndpkt->data, buffer, len);
            sndpkt->hdr.seqno = send_base_initial;

            // The packet is uploaded to the send_window buffer
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
        int loop_lim = 0; // Variable to check the next_seqno in the main traversing code
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

            // TEST CODE FOR TESTING END OF FILE
            //*****************************************************************************************************************
            /*if ( eof_reach == 1 )
            {
                int i =0;
                while ( i < window_size )
                {
                    sndpkt = (tcp_packet*) send_window[i];
                    if(sendto(sockfd, sndpkt, TCP_HDR_SIZE + get_data_size(sndpkt), 0, 
                            ( const struct sockaddr *)&serveraddr, serverlen) < 0)
                    {
                        error("sendto");
                    };  
                    i++;
                }
            }
            printf("recv pck ackno %i, next_seqno %i and value of fread %i \n",recvpkt->hdr.ackno, next_seqno, fread(buffer, 1, DATA_SIZE, fp));
            if ( recvpkt->hdr.ackno == next_seqno && fread(buffer, 1, DATA_SIZE, fp)==0 )
            {
                return 0;
            }*/

            //printf("Outside if statement rcvpck ackno = %i, send_base =  %i\n", recvpkt->hdr.ackno, send_base  );
            if ( recvpkt->hdr.ackno >= send_base )
            {
                //printf("Inside this if statement\n");
                while ( send_base < recvpkt->hdr.ackno )
                    {

                    len = fread(buffer, 1, DATA_SIZE, fp);
                    printf("%i\n", len );
                    // If len = 0 , set the eof_reach variable to 0
                    if ( len <= 0 )
                    {
                        eof_reach = 1;
            //****************************************************************************************************************
            // TEST CODE
                        /*
                        printf("send_base %i and next_seqno %i BEFORE while loop \n", send_base, next_seqno);
                        while ( recvpkt->hdr.ackno != next_seqno )
                        {
                            printf("send_base %i and next_seqno %i INSIDE while loop \n", send_base, next_seqno);
                            int i = 0;
                            while (i < window_size)
                            {
                                printf("send_base %i and next_seqno %i INSIDE another while loop \n\n", send_base, next_seqno);
                                if ( send_window[i] != NULL )
                                {
                                    sndpkt = (tcp_packet*) send_window[i];
                                }
                                printf("DATA SIZE% i \n",sndpkt->hdr.data_size);
                                printf("send base before %i\n", send_base );
                                printf("recvpkt hdr ackno  %i\n", recvpkt->hdr.ackno);
                                printf("sndpkt hdr seqno  %i\n", sndpkt->hdr.seqno );
                                if ( recvpkt->hdr.ackno == sndpkt->hdr.seqno )
                                {
                                    send_window[i] = NULL;
                                    send_base += sndpkt->hdr.data_size;
                                }
                                printf("send base after %i\n", send_base );
                                if ( send_window[i] == NULL )
                                {
                                    continue;
                                }
                                if(sendto(sockfd, sndpkt, TCP_HDR_SIZE + get_data_size(sndpkt), 0, 
                                            ( const struct sockaddr *)&serveraddr, serverlen) < 0)
                                {
                                    error("sendto");
                                }
                                i++;
                            }
                        }*/
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
                    loop_lim  = send_base + len;
                    send_base = loop_lim;
                    sndpkt = make_packet(len);
                    memcpy(sndpkt->data, buffer, len);
                    sndpkt->hdr.seqno = next_seqno;

                    //printf("After sndpack created statement rcvpck ackno = %i, send_base =  %i\n",recvpkt->hdr.ackno, send_base );
                    // if eof has been reached then do not re-assign send_window packets with sndpcks
                    if ( eof_reach != 1)
                    {
                        send_window[(next_seqno/DATA_SIZE) % window_size] = (tcp_packet *)sndpkt;
                    }

                    VLOG(DEBUG, "Sending packet %d to %s\n", 
                    send_base , inet_ntoa(serveraddr.sin_addr));
                    
                    // if eof has been reached set send_base to recvpkt's ackno
                    if ( eof_reach == 1)
                    {
                        send_base = recvpkt->hdr.ackno;
                    }

                    // If eof has reached then do not send any more packets but wait for timeouts and resend packets
                    if ( eof_reach != 1)
                    {
                        if(sendto(sockfd, sndpkt, TCP_HDR_SIZE + get_data_size(sndpkt), 0, 
                                    ( const struct sockaddr *)&serveraddr, serverlen) < 0)
                        {
                            error("sendto");
                        }
                        next_seqno = next_seqno + len;
                    }
                    }
            }
            stop_timer();
        } while(1);
        free(sndpkt);
    // Control should never reach this
    return 0;
}
