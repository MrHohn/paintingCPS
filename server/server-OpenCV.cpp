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
#include <getopt.h>
#include <unordered_map>
#include <errno.h>
#include <mfapi.h>
#include "MsgDistributor.h"

#define BUFFER_SIZE               1024  
#define PORT_NO                  20001
#define MAX_CONNECTION              10

static pthread_t mflistenThread;

struct Handle global_handle; // for MFAPI
// global flag for quit
int global_stop = 0;       
int orbit = 0;
int debug = 0;
MsgDistributor MsgD;
// class for image process
ImgMatch imgM;             
// map for result thread to search the queue address
unordered_map<string, queue<string>*> queue_map; 
// mutex lock for queue_map operation
pthread_mutex_t queue_map_lock; 
// map for transmit thread to search the semaphore address
unordered_map<string, sem_t*> sem_map; 
// mutex lock for sem_map operation
pthread_mutex_t sem_map_lock; 
// map to store logged in userID
unordered_map<String, uint> user_map; 
// pthread_t* thread_table;
// mutex lock for user_map operation
pthread_mutex_t user_map_lock; 

/******************************************************************************
Description.: Display a help message
Input Value.: -
Return Value: -
******************************************************************************/
void help(void)
{
    fprintf(stderr, " \n" \
            " Help for server-OpenCV application\n" \
            " ---------------------------------------------------------------\n" \
            " The following parameters can be passed to this software:\n\n" \
            " [-h | --help ]........: display this help\n" \
            " [-v | --version ].....: display version information\n"
            " [-orbit]..............: run in orbit mode\n" \
            " [-d]..................: debug mode, print more details\n" \
            " [-m]..................: mine GUID, a.k.a src_GUID\n" \
            " [-o]..................: other's GUID, a.k.a dst_GUID\n" \
            " \n" \
            " ---------------------------------------------------------------\n" \
            " Please start the server first\n"
            " \n");
}

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
Description.: print out the error message and exit
Input Value.:
Return Value:
******************************************************************************/
void errorSocket(const char *msg, int sock)
{
    errno = EBADF;
    perror(msg);
    close(sock); 
    printf("[server] Connection closed. --- error\n\n");
    pthread_exit(NULL); //terminate calling thread!
}

/******************************************************************************
Description: function for sending back the result
Input Value.:
Return Value:
******************************************************************************/
void server_result (int sock, string userID)
{
    // printf("result thread\n\n");

    int n;
    char response[] = "ok";
    char defMsg[] = "none";
    int matchedIndex;
    char sendInfo[256];
    vector<float> coord;
    sem_t *sem_match = new sem_t(); // create a new semaphore in heap
    queue<string> *imgQueue = 0;    // queue storing the file names 

    //  Init semaphore and put the address of semaphore into map
    if (sem_init(sem_match, 0, 0) != 0)
    {
        errorSocket("ERROR semaphore init failed", sock);
    }
    // grap the lock
    pthread_mutex_lock(&sem_map_lock);
    sem_map[userID] = sem_match;
    pthread_mutex_unlock(&sem_map_lock);

    // reponse to the client
    n = write(sock, response, sizeof(response));
    if (n < 0)
    {
        error("ERROR writting to socket");
    }

    while(!global_stop) 
    {
        sem_wait(sem_match);
        // get the address of image queue
        if (imgQueue == 0)
        {
            imgQueue = queue_map[userID];
        }

        // check if the queue is empty
        if (imgQueue->empty())
        {
            sem_map.erase(userID);
            queue_map.erase(userID);
            user_map.erase(userID);
            delete(sem_match);
            delete(imgQueue);
            sem_destroy(sem_match);
            errorSocket("ERROR image queue empty", sock);
        }

        // printf("\n----------- start matching -------------\n");
        string file_name = imgQueue->front(); 
        // printf("file name: %s\n", file_name.c_str());
        imgQueue->pop();

        // start matching the image
        imgM.matchImg(file_name);
        matchedIndex = imgM.getMatchedImgIndex();
        if (matchedIndex == 0) 
        {   
            // write none to client
            if (write(sock, defMsg, sizeof(defMsg)) < 0)
            {
               errorSocket("ERROR writting to socket", sock);
            }
            // printf("not match\n");            
        }
        else
        {
            // write index to client
            coord = imgM.calLocation();
            // sprintf(sendInfo, "%d,", matchedIndex);
            sprintf(sendInfo, "%d,%f,%f,%f,%f,%f,%f,%f,%f", matchedIndex, coord.at(0), coord.at(1), coord.at(2), coord.at(3), coord.at(4), coord.at(5), coord.at(6), coord.at(7));
            // printf("sendInfo: %s\n", sendInfo);
            if (write(sock, sendInfo, sizeof(sendInfo)) < 0)
            {
                errorSocket("ERROR writting to socket", sock);
            }
            // printf("matched image index: %d\n", matchedIndex);

        }

        // printf("------------- end matching -------------\n");
    }

    close(sock); 
    printf("[server] Connection closed. --- result\n\n");
    delete(sem_match);
    pthread_exit(NULL); //terminate calling thread!

}

/******************************************************************************
Description: function for transmitting the frames
Input Value.:
Return Value:
******************************************************************************/
void server_transmit (int sock, string userID)
{
    // printf("transmitting part\n");
    
    int n;
    char buffer[BUFFER_SIZE];
    char response[] = "ok";
    char file_name_temp[60];
    char *file_name;
    int write_length = 0;
    int length = 0;
    char *block_count_char;
    int block_count;
    int count = 0;
    queue<string> *imgQueue = new queue<string>();    // queue storing the file names 

    // grap the lock
    pthread_mutex_lock(&queue_map_lock);
    queue_map[userID] = imgQueue; // put the address of queue into map
    pthread_mutex_unlock(&queue_map_lock);

    pthread_mutex_t queueLock; // mutex lock for queue operation
    sem_t *sem_match = 0;

    // init the mutex lock
    if (pthread_mutex_init(&queueLock, NULL) != 0)
    {
        errorSocket("ERROR mutex init failed", sock);
    }

    if (!orbit)
    {
        // reponse to the client
        n = write(sock, response, sizeof(response));
        if (n < 0)
        {
            pthread_mutex_destroy(&queueLock);
            errorSocket("ERROR writting to socket", sock);
        }

        while (!global_stop)
        {
            // receive the file info
            bzero(buffer,BUFFER_SIZE);
            n = read(sock,buffer, sizeof(buffer));
            if (n <= 0)
            {
                pthread_mutex_destroy(&queueLock);
                // signal the result thread to terminate
                sem_post(sem_match);
                errorSocket("ERROR reading from socket", sock);
            } 

            // store the file name and the block count
            file_name = strtok(buffer, ",");
            strcpy(file_name_temp, file_name);
            printf("\n[server] file name: %s\n", file_name);
            block_count_char = strtok(NULL, ",");
            block_count = strtol(block_count_char, NULL, 10);
            // printf("block count: %d\n", block_count);

            // reponse to the client
            n = write(sock, response, sizeof(response));
            if (n <= 0)
            {
                pthread_mutex_destroy(&queueLock);
                // signal the result thread to terminate
                sem_post(sem_match);
                errorSocket("ERROR writting to socket", sock);
            } 

            FILE *fp = fopen(file_name, "w");  
            if (fp == NULL)  
            {  
                printf("File:\t%s Can Not Open To Write!\n", file_name);  
                break;
            }  

            // receive the data from server and store them into buffer
            bzero(buffer, sizeof(buffer));
            count = 0;
            while((length = recv(sock, buffer, BUFFER_SIZE, 0)))  
            {
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
                ++count;
                if (count >= block_count)
                {
                    // printf("block count full\n");
                    break;
                }
            }
            printf("[server] Recieve Finished!\n\n");  
            // finished 
            fclose(fp);

            // lock the queue, ensure there is only one thread modifying the queue
            pthread_mutex_lock(&queueLock);

            // store the file name to the waiting queue
            string file_name_string(file_name_temp);
            imgQueue->push(file_name_string);

            pthread_mutex_unlock(&queueLock);
            // get the address of sem_match
            if (sem_match == 0)
            {
                while (sem_map.find(userID) == sem_map.end());
                sem_match = sem_map[userID];
            }
            // signal the result thread to do image processing
            sem_post(sem_match);
        }

        close(sock);
    }
    // below is orbit mode
    else
    {
        // get the id length
        int id_length = 1;
        int divisor = 10;
        while (sock / divisor > 0)
        {
            ++id_length;
            divisor *= 10;
        }
        int recv_length = BUFFER_SIZE - 6 - id_length;
        char *file_size_char;
        int file_size;
        int received_size = 0;

        printf("\nstart receiving file\n");

        while (!global_stop)
        {
            bzero(buffer, BUFFER_SIZE);
            MsgD.recv(sock, buffer, BUFFER_SIZE);
            file_name = strtok(buffer, ",");
            strcpy(file_name_temp, file_name);
            printf("\n[server] file name: %s\n", file_name);
            file_size_char = strtok(NULL, ",");
            file_size = strtol(file_size_char, NULL, 10);
            printf("file size: %d\n", file_size);

            FILE *fp = fopen(file_name, "w");  
            if (fp == NULL)  
            {  
                printf("File:\t%s Can Not Open To Write!\n", file_name);  
            }  

            // receive the data from server and store them into buffer
            while(1)  
            {
                bzero(buffer, BUFFER_SIZE);
                MsgD.recv(sock, buffer, BUFFER_SIZE);
                
                if (file_size - received_size <= recv_length)
                {
                    int remain = file_size - received_size;
                    write_length = fwrite(buffer, sizeof(char), remain, fp);  
                    if (write_length < remain)  
                    {  
                        printf("File:\t Write Failed!\n");  
                        break;  
                    }
                    break;
                }

                write_length = fwrite(buffer, sizeof(char), recv_length, fp);  
                if (write_length < recv_length)  
                {  
                    printf("File:\t Write Failed!\n");  
                    break;  
                }  
                received_size += BUFFER_SIZE - 6 - id_length;
            }
            printf("[server] Recieve Finished!\n\n");  
            // finished 
            fclose(fp);

            // lock the queue, ensure there is only one thread modifying the queue
            pthread_mutex_lock(&queueLock);

            // store the file name to the waiting queue
            string file_name_string(file_name_temp);
            imgQueue->push(file_name_string);

            pthread_mutex_unlock(&queueLock);
            // get the address of sem_match
            if (sem_match == 0)
            {
                while (sem_map.find(userID) == sem_map.end());
                sem_match = sem_map[userID];
            }
            // signal the result thread to do image processing
            sem_post(sem_match);
        }
    }

    delete(imgQueue);
    printf("[server] Connection closed. --- transmit\n\n");
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
    char buffer[BUFFER_SIZE];
    string userID;
    char *threadType;
    char fail[BUFFER_SIZE] = "failed";

    // Receive the header
    bzero(buffer, BUFFER_SIZE);
    if (!orbit)
    {
        n = read(sock, buffer, sizeof(buffer));
        if (n < 0)
        {
            errorSocket("ERROR reading from socket", sock);
        } 
    }
    // below is orbit mode, using MFAPI
    else
    {
        MsgD.recv(sock, buffer, BUFFER_SIZE);
    }

    printf("[server] header content: %s\n\n",buffer);

    threadType = strtok(buffer, ",");
    userID = strtok(NULL, ",");

    // grap the lock
    pthread_mutex_lock(&user_map_lock);
    // confirm that this user does not log in
    if (user_map.find(userID) == user_map.end())
    {
        // put the new user into user map
        user_map[userID] = 1;
    }
    else
    {
        if (user_map[userID] == 1)
        {
            // increase user thread count
            user_map[userID] = 2;
        }
        else
        {
            // remember to unlock!
            pthread_mutex_unlock(&user_map_lock);
            // reponse to the client
            if (!orbit)
            {
                if (write(sock, "failed", sizeof("failed")) < 0)
                {
                    errorSocket("ERROR writting to socket", sock);
                }
                close(sock); 
            }
            else
            {
                MsgD.send(sock, fail, BUFFER_SIZE);
            }
            printf("[server] User exist. Connection closed.\n\n");
            return 0;
        }
    }
    pthread_mutex_unlock(&user_map_lock);

    if (strcmp(threadType, "transmit") == 0) 
    {
        server_transmit(sock, userID);
    }
    else if (strcmp(threadType, "result") == 0) 
    {
        server_result(sock, userID);
    }
    else
    {
        if (!orbit)
        {
            close(sock); 
        }
        printf("[server] Command Unknown. Connection closed.\n\n");
    }

    return 0;
}

/******************************************************************************
Description.: calling this function creates and starts the server threads
Input Value.: -
Return Value: -
******************************************************************************/
void server_main()
{
    // init part
    printf("\n[server] start initializing\n");

    if (!orbit)
    {
        int sockfd, newsockfd, portno;
        socklen_t clilen;
        struct sockaddr_in serv_addr, cli_addr;
        clilen = sizeof(cli_addr);

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
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
            error("ERROR on binding");
        }
        else
            printf("[server] bind tcp port %d sucessfully.\n",portno);

        if(listen(sockfd,5))
        {
            error("ERROR listening");
        }
        else 
            printf ("[server] listening the port %d sucessfully.\n\n", portno);    
        
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
                printf ("[server] server has got connect from %s, socket id: %d.\n", (char *)inet_ntoa(cli_addr.sin_addr), newsockfd);

            /* create thread and pass context to thread function */
            if (pthread_create(&thread_id, 0, serverThread, (void *)&(newsockfd)) == -1)
            {
                fprintf(stderr,"pthread_create error!\n");
                break; //break while loop
            }
            pthread_detach(thread_id);
            usleep(1000 * 5); //  sleep 5ms to avoid clients gain same sock

        } /* end of while */
        close(sockfd);
    }
    // below is orbit part
    else
    {
        int newsockfd;   
        // init finished, now wait for a client
        while (!global_stop) {
            pthread_t thread_id;
            //Block here. Until server accpets a new connection.
            newsockfd = MsgD.accept();
            if (newsockfd < 0)
            {
                // error("ERROR on accept");
                fprintf(stderr,"Accept error!\n");
                continue; //ignore current socket ,continue while loop.
            }
            else 
                printf ("[server] server has got new connection, socket id: %d.\n", newsockfd);

            /* create thread and pass context to thread function */
            if (pthread_create(&thread_id, 0, serverThread, (void *)&(newsockfd)) == -1)
            {
                fprintf(stderr,"pthread_create error!\n");
                break; //break while loop
            }
            pthread_detach(thread_id);
            usleep(1000 * 5); //  sleep 5ms to avoid clients gain same sock

        } /* end of while */
    }

}

/******************************************************************************
Description.: this is the mflisten thread
              it loops forever, listen on the src_GUID
Input Value.:
Return Value:
******************************************************************************/
void *mflisten_thread(void *arg)
{
    printf("\nin listen thread\n");
    while (!global_stop) {
        MsgD.listen();
    }

    return NULL;
}

/******************************************************************************
Description.: calling this function start the server and listener
Input Value.: -
Return Value: -
******************************************************************************/
void server_run()
{
    if (orbit)
    {
        /* create thread and pass context to thread function */
        if (pthread_create(&mflistenThread, 0, mflisten_thread, NULL) == -1)
        {
            fprintf(stderr,"pthread_create error!\n");
            exit(1);
        }
        pthread_detach(mflistenThread);   
    }

    server_main();
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
    pthread_cancel(mflistenThread);
    pthread_mutex_destroy(&queue_map_lock);
    pthread_mutex_destroy(&sem_map_lock);
    pthread_mutex_destroy(&user_map_lock);
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
    int src_GUID = -1, dst_GUID = -1;

    /* parameter parsing */
    while(1) {
        int option_index = 0, c = 0;
        static struct option long_options[] =
        {
            {"h", no_argument, 0, 0},
            {"help", no_argument, 0, 0},
            {"v", no_argument, 0, 0},
            {"version", no_argument, 0, 0},
            {"orbit", no_argument, 0, 0},
            {"d", no_argument, 0, 0},
            {"m", required_argument, 0, 0},
            {"o", required_argument, 0, 0},
            {0, 0, 0, 0}
        };

        c = getopt_long_only(argc, argv, "", long_options, &option_index);

        /* no more options to parse */
        if(c == -1) break;

        /* unrecognized option */
        if(c == '?')
        {
            help();
            return 0;
        }

        switch(option_index)
        {
            /* h, help */
        case 0:
        case 1:
            help();
            return 0;
            break;

            /* v, version */
        case 2:
        case 3:
            printf("Real-Time CPS Server Version: 0.1\n" \
            "Compilation Date.....: unknown\n" \
            "Compilation Time.....: unknown\n");
            return 0;
            break;

            /* orbit, run in orbit mode */
        case 4:
            orbit = 1;
            break;

            /* debug mode */
        case 5:
            debug = 1;
            break;

            /* mine GUID */
        case 6:
            src_GUID = strtol(optarg, NULL, 10);
            break;

            /* other's GUID */
        case 7:
            dst_GUID = strtol(optarg, NULL, 10);
            break;

        default:
            help();
            return 0;
        }
    }

    /* register signal handler for <CTRL>+C in order to clean up */
    // if(signal(SIGINT, signal_handler) == SIG_ERR)
    // {
    //     printf("could not register signal handler\n");
    //     exit(EXIT_FAILURE);
    // }

    // init the mutex lock
    if (pthread_mutex_init(&user_map_lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }

    // init the mutex lock
    if (pthread_mutex_init(&queue_map_lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }

    // init the mutex lock
    if (pthread_mutex_init(&sem_map_lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }

    if (orbit)
    {
        if (src_GUID != -1 && dst_GUID != -1)
        {
            if (debug) printf("src_GUID: %d, dst_GUID: %d\n", src_GUID, dst_GUID);
            /* init new Message Distributor */
            MsgD.init(src_GUID, dst_GUID, &global_handle);
        }
        else
        {
            printf("ERROR: please enter src_GUID and dst_GUID with flags -m & -o\n");
            exit(1);
        }

    }

    // imgM.init_DB(100,"./imgDB/","./indexImgTable","ImgIndex.yml");
    imgM.init_matchImg("./indexImgTable", "ImgIndex.yml", "./infoDB/");

    server_run();



/*

    // below is the testing stuffs

    // string message;
    int buffer_length = BUFFER_SIZE - 6 - 1;
    char buffer[buffer_length];
    int id1 = MsgD.accept();
    MsgD.recv(id1, buffer, buffer_length);
    // printf("receive message: [%s]\n", message.c_str());
    printf("receive message: [%s]\n", buffer);

    bzero(buffer, buffer_length);
    int id2 = MsgD.accept();
    MsgD.recv(id2, buffer, buffer_length);
    // printf("receive message: [%s]\n", message.c_str());
    printf("receive message: [%s]\n", buffer);

    // test incorrect id case
    // MsgD.recv(101);

    // message = MsgD.recv(id1);
    // printf("receive message: [%s]\n", message.c_str());
    // message = MsgD.recv(id1);
    // printf("receive message: [%s]\n", message.c_str());
    // message = MsgD.recv(id1);
    // printf("receive message: [%s]\n", message.c_str());

    // message = MsgD.recv(id2);
    // printf("receive message: [%s]\n", message.c_str());
    // message = MsgD.recv(id2);
    // printf("receive message: [%s]\n", message.c_str());
    // message = MsgD.recv(id2);
    // printf("receive message: [%s]\n", message.c_str());


    // get the id length
    int id_length = 1;
    int divisor = 10;
    while (id1 / divisor > 0)
    {
        ++id_length;
        divisor *= 10;
    }
    int recv_length = BUFFER_SIZE - 6 - id_length;
    char *file_size_char;
    int file_size;
    int received_size = 0;
    int write_length;

    printf("\nstart receiving file\n");

    bzero(buffer, buffer_length);
    MsgD.recv(id1, buffer, buffer_length);
    char *file_name;
    file_name = strtok(buffer, ",");
    printf("\n[server] file name: %s\n", file_name);
    file_size_char = strtok(NULL, ",");
    file_size = strtol(file_size_char, NULL, 10);
    printf("file size: %d\n", file_size);

    FILE *fp = fopen(file_name, "w");  
    if (fp == NULL)  
    {  
        printf("File:\t%s Can Not Open To Write!\n", file_name);  
    }  

    // receive the data from server and store them into buffer
    while(!global_stop)  
    {
        bzero(buffer, buffer_length);
        MsgD.recv(id1, buffer, buffer_length);
        
        if (file_size - received_size <= recv_length)
        {
            int remain = file_size - received_size;
            write_length = fwrite(buffer, sizeof(char), remain, fp);  
            if (write_length < remain)  
            {  
                printf("File:\t Write Failed!\n");  
                break;  
            }
            break;
        }

        write_length = fwrite(buffer, sizeof(char), recv_length, fp);  
        if (write_length < recv_length)  
        {  
            printf("File:\t Write Failed!\n");  
            break;  
        }  
        received_size += BUFFER_SIZE - 6 - id_length;
    }
    printf("[server] Recieve Finished!\n\n");  
    // finished 
    fclose(fp);
*/

    return 0;
}
