/*************************************************
#
# Purpose: main program for "CPS server"
# Author.: Zihong Zheng
# Version: 0.1
# License: 
#
*************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include "ImgMatch.h"
#include <sys/time.h>
#include <queue>
// #include <sys/queue.h>
// #include <errno.h>

#define BUFFER_SIZE               1024  
#define PORT_NO                  20001
#define MAX_CONNECTION              10

/*
 * keep context for each server
 */
// context servers[MAX_SERVER_NUM];
 // context transmitServer;
 // context resultServer;

int global_stop = 0;       // global flag for quit
sem_t sem_imgProcess;      // semaphore for result thread
queue<string> imgQueue;    // queue storing the file names 
pthread_mutex_t queueLock; // mutex lock for queue operation
ImgMatch imgM;             // class for image process

/******************************************************************************
Description.: print out the error message and exit
Input Value.:
Return Value:
******************************************************************************/
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

/******************************************************************************
Description: function for sending back the result
Input Value.:
Return Value:
******************************************************************************/
void server_result (int sock)
{
    int n;
    // char buffer[BUFFER_SIZE];
    char response[] = "ok";
    char defMsg[] = "none";
    int matchedIndex;
    char sendIndex[40];
    char userLine[256];
    // int userNum;
    printf("result thread\n\n");

    // reponse to the client
    n = write(sock, response, sizeof(response));
    if (n < 0) 
        error("ERROR writting to socket");

    while(!global_stop) 
    {
        // read user input and send it to client(a simple simulation)
        if (fgets(userLine, sizeof(userLine), stdin)) {
            n = write(sock, userLine, sizeof(userLine));
            if (n < 0) 
                error("ERROR writting to socket");
        }

        // sem_wait(&sem_imgProcess);
        // string file_name = imgQueue.front(); 
        // printf("file name: %s\n", file_name.c_str());
        // imgQueue.pop();

        // start matching the image
        // imgM.matchImg(file_name);
        // matchedIndex = imgM.getMatchedImgIndex();
        // if (matchedIndex == 0) 
        // {   
        //     // write none to client
        //     // if (write(sock, defMsg, sizeof(defMsg)) < 0)
        //     // {
        //     //     error("ERROR writting to socket");
        //     // }
        // }
        // else
        // {
        //     // write index to client
        //     // itoa(matchedIndex, sendIndex, 10);
        //     sprintf(sendIndex, "%d", matchedIndex);
        //     // if (write(sock, sendIndex, sizeof(sendIndex)) < 0)
        //     // {
        //     //     error("ERROR writting to socket");
        //     // }
        //     // printf("\nmatched image index: %d\n", matchedIndex);
        // }


    }

    close(sock); 
    printf("[server] Connection closed.\n\n");
    pthread_exit(NULL); //terminate calling thread!

}

/******************************************************************************
Description: function for transmitting the frames
Input Value.:
Return Value:
******************************************************************************/
void server_transmit (int sock)
{
    int n;
    char buffer[BUFFER_SIZE];
    char response[] = "ok";

    char file_name[BUFFER_SIZE];
    int write_length = 0;
    int length = 0;  
    printf("transmitting part\n\n");

    // reponse to the client
    n = write(sock, response, sizeof(response));
    if (n < 0) 
        error("ERROR writting to socket");

    bzero(buffer,BUFFER_SIZE);
    n = read(sock,buffer, sizeof(buffer));
    if (n < 0) 
        error("ERROR reading from socket");
    // store the file name
    strncpy(file_name, buffer, BUFFER_SIZE);
    printf("[server] file name: %s\n", file_name);
    // n = write(sock,"I got your message",18);
    // if (n < 0) error("ERROR writing to socket");

    // reponse to the client
    n = write(sock, response, sizeof(response));
    if (n < 0) 
        error("ERROR writting to socket");


    FILE *fp = fopen(file_name, "w");  
    if (fp == NULL)  
    {  
        printf("File:\t%s Can Not Open To Write!\n", file_name);  
        exit(1);  
    }  

    // receive the data from server and store them into buffer
    bzero(buffer, sizeof(buffer));  
    while((length = recv(sock, buffer, BUFFER_SIZE, 0)))  
    {  
        // printf("%d\n", length);
        if (length < 0)  
        {  
            printf("Recieve Data From Client Failed!\n");  
            break;  
        }
  
        write_length = fwrite(buffer, sizeof(char), length, fp);  
        if (write_length < length)  
        {  
            printf("File:\t Write Failed!\n");  
            break;  
        }  
        bzero(buffer, BUFFER_SIZE);
    }
    printf("[server] Recieve File: %s From Client Finished!\n", file_name);  
    // finished 
    fclose(fp);

    // lock the queue, ensure there is only one thread modifying the queue
    pthread_mutex_lock(&queueLock);

    // store the file name to the waiting queue
    string file_name_string = file_name;
    imgQueue.push(file_name_string);

    pthread_mutex_unlock(&queueLock);
    // signal the result thread to do image processing
    sem_post(&sem_imgProcess);

    close(sock); 
    printf("[server] Connection closed.\n\n");
    pthread_exit(NULL); //terminate calling thread!

}

/******************************************************************************
Description.: There is a separate instance of this function 
              for each connection.  It handles all communication
              once a connnection has been established.
Input Value.:
Return Value: -
******************************************************************************/
void *serverThread (void * inputsock)
{
    int sock = *((int *)inputsock);
    int n;
    char buffer[20];

    // Receive the header
    bzero(buffer,20);
    n = read(sock, buffer, sizeof(buffer));
    if (n < 0) error("ERROR reading from socket");
    printf("[server] header content: %s\n",buffer);

    if (strcmp(buffer, "transmit") == 0) 
    {
        server_transmit(sock);
    }
    else if (strcmp(buffer, "result") == 0) 
    {
        server_result(sock);
    }
    else
    {
        close(sock); 
        printf("[server] Command Unknown. Connection closed.\n\n");
    }

    return 0;
}

/******************************************************************************
Description.: calling this function creates and starts the server threads
Input Value.: -
Return Value: -
******************************************************************************/
void run_server()
{
    // init part
    // int pid;
    printf("\n[server] start initializing\n");
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    clilen = sizeof(cli_addr);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        printf("ERROR opening socket\n");
        error("ERROR opening socket");
    } 
    else 
        printf ("[server] obtain socket descriptor successfully.\n"); 
    bzero((char *) &serv_addr, sizeof(serv_addr));
    // set up the port number
    portno = PORT_NO;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
    {
        printf("ERROR on binding");
        error("ERROR on binding");
    }
    else
        printf("[server] bind tcp port %d sucessfully.\n",portno);

    if(listen(sockfd,5))
        error("ERROR listening");
    else 
        printf ("[server] listening the port %d sucessfully.\n", portno);    
    
    // init finished, now wait for a client
    while (!global_stop) {
        pthread_t thread_id;
        //Block here. Until server accpets a new connection.
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
        {
            // error("ERROR on accept");
            fprintf(stderr,"Accept error!\n");
            continue; //ignore current socket ,continue while loop.
        }
        else 
            printf ("\n[server] server has got connect from %s.\n", (char *)inet_ntoa(cli_addr.sin_addr));

        /* create thread and pass context to thread function */
        if (pthread_create(&thread_id, 0, serverThread, (void *)&(newsockfd)) == -1)
        {
            fprintf(stderr,"pthread_create error!\n");
            break; //break while loop
        }
        pthread_detach(thread_id);

    } /* end of while */
    close(sockfd);
}

/******************************************************************************
Description.: pressing CTRL+C sends signals to this process instead of just
              killing the threads can tidily shutdown and free allocated
              resources. The function prototype is defined by the system,
              because it is a callback function.
Input Value.: sig tells us which signal was received
Return Value: -
******************************************************************************/
void signal_handler(int sig)
{

    /* signal "stop" to threads */
    printf("\nSetting signal to stop.\n");
    global_stop = 1;
    sem_destroy(&sem_imgProcess);
    pthread_mutex_destroy(&queueLock);
    usleep(1000 * 1000);

    /* clean up threads */
    printf("Force cancellation of threads and cleanup resources.\n");
    // client_stop();

    usleep(1000 * 1000);

    printf("Done.\n");

    exit(0);
    return;
}


int main(int argc, char *argv[])
{
    /* register signal handler for <CTRL>+C in order to clean up */
    if(signal(SIGINT, signal_handler) == SIG_ERR) {
        printf("could not register signal handler\n");
        exit(EXIT_FAILURE);
    }

    const unsigned int nSemaphoreCount = 0; //initial value of semaphore
    int nRet = -1;
    nRet = sem_init(&sem_imgProcess, 0, nSemaphoreCount);
    if (0 != nRet)
    {
        return -1;
    }

    if (pthread_mutex_init(&queueLock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }

    // ImgMatch imgM;
    // imgM.init_DB(100,"./imgDB/","./indexImgTable","ImgIndex.yml");
    imgM.init_matchImg("./indexImgTable", "ImgIndex.yml", "./infoDB/");
    // imgM.matchImg("./imgDB/2.jpg");
    // imgM.showMatchImg();

    run_server();

    return 0; /* we never get here */
}
