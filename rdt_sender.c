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
#include <math.h>

#include "packet.h"
#include "common.h"

#define STDIN_FD 0
#define RETRY 120 //milli second 
#define max_window_size 100000// Setting the max_window_size to 1000 as a window_size CONST
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

void resend_packet() 
{
    int index_to_send = 0;
    index_to_send = send_base / DATA_SIZE;

    if(sendto(sockfd, send_window[index_to_send], TCP_HDR_SIZE + get_data_size(send_window[index_to_send]), 0, 
            ( const struct sockaddr *)&serveraddr, serverlen) < 0)
    {
        error("sendto");
    };  

}

void signal_handler( int sig )
{
    if(sig == SIGALRM){

         VLOG(INFO, "Timout happend");
         resend_packet();
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
    signal(SIGALRM, signal_handler);
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
    int next_index = 0; //the index of the next packet to be sent 
    int current_index = 0; //the index of first packet in the window buffer
    int current_window_size = 0;
    int to_be_sent = 0; 
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

    init_timer(RETRY, signal_handler);
    next_seqno = 0;
    send_base = 0;
    int send_base_initial = 0; // base var to keep track of the base send for the first window_size packets
 


    int position_buffer = 0; //position in the buffer where we are inserting the read packet
    int base = 0;
    int seq_no = 0;

    while(1){

        len = fread(buffer, 1, DATA_SIZE,fp);

        if(len <= 0){

            sndpkt = make_packet(MSS_SIZE);
            sndpkt->hdr.data_size = 0;
            sndpkt->hdr.seqno = -1;
            send_window[position_buffer] = (tcp_packet*)sndpkt;
            break;
        }


        base = seq_no;
        seq_no = base + len;
        sndpkt = make_packet(len);
        memcpy(sndpkt->data, buffer,len);
        sndpkt->hdr.seqno = base;

        send_window[position_buffer] = (tcp_packet *)sndpkt;
        position_buffer ++;

    }



    int k = 0;

    while(1){

        if(send_window[k]->hdr.seqno == -1){

            printf("Exiting the loop 00 \n");
            break;
        }

        printf("Seqno of packet is %d\n", send_window[k]->hdr.seqno);
        k++;

    }


    // Send the first window_size packets
    position_buffer = 0;
    int i = 0;
    int length;
    while ( i < window_size)
    {



        length = send_window[position_buffer]->hdr.data_size;
        send_base_initial = next_seqno;
        next_seqno = send_base_initial +length;
        //end_of_array = next_seqno;



        VLOG(DEBUG, "Sending packet %d to %s", 
        send_base_initial, inet_ntoa(serveraddr.sin_addr));
        /*
         * If the sendto is called for the first time, the system will
         * will assign a random port number so that server can send its
         * response to the src port.
         */
        if(sendto(sockfd, send_window[position_buffer], TCP_HDR_SIZE + get_data_size(send_window[position_buffer]), 0, 
                    ( const struct sockaddr *)&serveraddr, serverlen) < 0)
        {
            error("sendto");
        }

        position_buffer++;
        i++;

    }

    printf("Packet sent");


    
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
            printf("3 duplicate acks forced timeout\n");
            // If three duplicate 

            resend_packet();

            window_size = 1;
            current_index = send_base / DATA_SIZE;
            next_seqno = send_window[current_index]->hdr.seqno + send_window[current_index]->hdr.data_size;


            
            continue;         
        }

        
        printf("recvpkt ackno = %i, send_base =  %i\n", recvpkt->hdr.ackno, send_base  );

        
        if ( recvpkt->hdr.ackno >= send_base ) {
            //printf("Inside this if statement\n");


            while ( send_base < recvpkt->hdr.ackno ) {

    
 
                next_index = (int)ceil( (double)next_seqno / (double)DATA_SIZE ); //index of next packet to be sent

                printf("NEXT PACKET TO BE SENT %d\n", next_index);

                printf("Index in our buffer %d\n",(int)ceil( (double)next_seqno / (double)DATA_SIZE ));
                if ( send_window[next_index] ->hdr.seqno == -1){

                    if(send_window[next_index] ->hdr.data_size == 0){

                        eof_reach= 1;
                     }   

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


                current_index = send_base / DATA_SIZE; 

                send_base = send_base + send_window[current_index]->hdr.data_size;
                //printf("Send BASE IS %d\n", send_base);



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
                    if(sendto(sockfd, send_window[next_index], TCP_HDR_SIZE + get_data_size(send_window[next_index]), 0, 
                                ( const struct sockaddr *)&serveraddr, serverlen) < 0)
                    {
                        error("sendto");
                    }
                    printf("Sending packet 0000000000000\n");
                    next_seqno += send_window[next_index]->hdr.data_size;
                    
                    window_size++;
                }


            }

            printf("111111111111111111111111\n");

            printf("Send base is%d\n", send_base);
            printf("Nextseqno is %d\n", next_seqno);
            current_window_size = (int)ceil ((double)(next_seqno - send_base)/ (double)DATA_SIZE);
            to_be_sent = window_size - current_window_size;

            printf("Before loop Current window size is %d\n", current_window_size);

            int check = 0;//check for end of file

            while(to_be_sent > 0){

                next_index = (int)ceil( (double)next_seqno / (double)DATA_SIZE );

                printf("INSIDE THE LOOP\n");


                if ( send_window[next_index] ->hdr.seqno == -1){

                    if(send_window[next_index] ->hdr.data_size == 0){

                        check= 1;
                        break;
                     }   

                }


                if(check != 1){
                    if(sendto(sockfd, send_window[next_index], TCP_HDR_SIZE + get_data_size(send_window[next_index]), 0, 
                                    ( const struct sockaddr *)&serveraddr, serverlen) < 0)
                        {
                            error("sendto");
                        }
                        printf("Sending packet 0000000000000\n");
                        next_seqno += send_window[next_index]->hdr.data_size;
                }

                to_be_sent--;


            }

            current_window_size = (int)ceil ((double)(next_seqno - send_base)/ (double)DATA_SIZE);

            printf("After loop Current window size is %d\n", current_window_size);




            //printf("11111111111111111\n");
        }
        stop_timer();
    } while(1);
    free(sndpkt);
    // Control should never reach this
    return 0;

    
}