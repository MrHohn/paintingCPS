/*************************************************
#
# Purpose: main program for "CPS client"
# Author.: Zihong Zheng
# Version: 0.1
# License: 
#
*************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <string.h>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <sys/time.h>
#include <sys/stat.h>
#include <getopt.h>

using namespace cv;
using namespace std;

#define BUFFER_SIZE               1024  
#define PORT_NO                  20001
#define DELAY                        1 // ms

static pthread_t transmitThread;
static pthread_t resultThread;
pthread_mutex_t sendLock; // mutex lock to make sure transmit order

int global_stop = 0;
char *userID;
// Point center = Point(255,255);
// int r = 100;
// int drawCircle = 0;
int drawResult = 0;
string resultShown = "";
float coord[8];

struct arg_transmit {
    int sock;
    char file_name[20];
};

/******************************************************************************
Description.: Display a help message
Input Value.: argv[0] is the program name and the parameter progname
Return Value: -
******************************************************************************/
void help(void)
{
    fprintf(stderr, " ---------------------------------------------------------------\n" \
            " Help for client-OpenCV application\n" \
            " ---------------------------------------------------------------\n" \
            " The following parameters can be passed to this software:\n\n" \
            " [-h | --help ]........: display this help\n" \
            " [-v | --version ].....: display version information\n" \
            " [-id ]................: user ID to input\n" 
            " ---------------------------------------------------------------\n");
}

/*
*****************************************************************************
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
Description.: this is the recognition result receiver thread
              it loops forever, receive the result from the server side 
              and display it
Input Value.:
Return Value:
******************************************************************************/
void *result_thread(void *arg)
{
    // printf("In the receiver thread.\n");

    /*-----------------network part--------------*/

    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    struct in_addr ipv4addr;
    char buffer[256];
    char header[] = "result";
    char response[10];
    char *resultTemp;

    portno = PORT_NO;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    char server_addr[] = "127.0.0.1";
    inet_pton(AF_INET, server_addr, &ipv4addr);
    server = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AF_INET);
    // printf("\n[client] Host name: %s\n", server->h_name);
    printf("\n[client] Server address: %s\n", server_addr);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length); 
    serv_addr.sin_port = htons(portno);

    // finished initialize, try to connect

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
    {
        printf("-------- The server is not available now. ---------\n\n");
        global_stop = 1;
        exit(0);
        // error("ERROR connecting");
    }
    else
    {
        printf("[client] result thread get connection to server\n");
        printf("[client] start receiving the result\n");   
    }

    // send the header first
    n = write(sockfd, header, sizeof(header));
    if (n < 0) 
         error("ERROR writing to socket");
    // get the response
    n = read(sockfd, response, sizeof(response));
    if (n < 0) 
         error("ERROR reading from socket");
    
    while(!global_stop)
    {
        bzero(buffer, sizeof(buffer));
        if (recv(sockfd, buffer, sizeof(buffer), 0) < 0) 
        {
            error("ERROR reading from socket");
        }
        else if (n > 0)
        {
            if (strcmp(buffer, "none") == 0)
            {
                drawResult = 0;
            }
            else
            {
                drawResult = 0;
                // printf("result: %s\n", buffer);
                resultShown = strtok(buffer, ",");
                resultShown = "matched index: " + resultShown;
                for (int i = 0; i < 8; ++i) {
                    resultTemp = strtok(NULL, ",");
                    coord[i] = atof(resultTemp);
                    // printf("%f\n", coord[i]);
                }
                drawResult = 1;
            }
        }
    }

    close(sockfd); // disconnect server
    printf("[client] connection closed\n");

    /*---------------------------end--------------------------*/

    // /* cleanup now */
    // pthread_cleanup_pop(1);
    global_stop = 1;

    return NULL;
}

/******************************************************************************
Description.: this is the transmit child thread
              it is responsible to send out one frame
Input Value.:
Return Value:
******************************************************************************/
void *transmit_child(void *arg)
{
    // printf("here\n");

    struct arg_transmit *args = (struct arg_transmit *)arg;
    int sockfd = args->sock;
    char *file_name = args->file_name;
    int n;
    char bufferSend[BUFFER_SIZE];
    char response[10];

    // stat of file, to get the size
    struct stat file_stat;
    int block_count = 0;
    char send_info[70];

    // get the status of file
    if (stat(file_name, &file_stat) == -1)
    {
        perror("stat");
        exit(EXIT_FAILURE);
    }
    if (file_stat.st_size % BUFFER_SIZE == 0)
    {
        block_count = file_stat.st_size / 1024;
    }
    else
    {
        block_count = file_stat.st_size / 1024 + 1;
    }
    // printf("block count: %d\n", block_count);

    // gain the lock, insure transmit order
    pthread_mutex_lock(&sendLock);

    // send the file info, combine with ','
    printf("[client] file name: %s\n", file_name);
    sprintf(send_info, "%s,%d", file_name, block_count);
    n = write(sockfd, send_info, sizeof(send_info));
    if (n < 0) 
         error("ERROR writing to socket");

    // get the response
    n = read(sockfd, response, sizeof(response));
    if (n < 0) 
         error("ERROR reading from socket");

    FILE *fp = fopen(file_name, "r");  
    if (fp == NULL)  
    {  
        // printf("File:\t%s Not Found!\n", file_name);  
        printf("File:\t%s Not Found!\n", file_name);  
    }  
    else  
    {  
        bzero(bufferSend, BUFFER_SIZE);  
        int file_block_length = 0;
        // start transmitting the file
        while( (file_block_length = fread(bufferSend, sizeof(char), BUFFER_SIZE, fp)) > 0)  
        {  
            // send data to the client side  
            if (send(sockfd, bufferSend, file_block_length, 0) < 0)  
            {  
                printf("Send File: %s Failed!\n", file_name);  
                break;  
            }  

            bzero(bufferSend, BUFFER_SIZE);  
        }

        fclose(fp);  
        printf("[client] Transfer Finished!\n\n");  
    }

    pthread_mutex_unlock(&sendLock);
    pthread_exit(NULL);

    return NULL;
}

/******************************************************************************
Description.: this is the transmit thread
              it loops forever, grabs a fresh frame and stores it to file,
              also send it out to the server
Input Value.:
Return Value:
******************************************************************************/
void *transmit_thread(void *arg)
{
    int index = 1;
    int count = 0;
    
    char file_name[60] = {0};
    VideoCapture capture(0);
    Mat frame;

    // set up the image format and the quality
    // capture.set(CV_CAP_PROP_FRAME_WIDTH, 640);
    // capture.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
    vector<int> compression_params;
    compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
    compression_params.push_back(95);


    /*-----------------network part--------------*/

    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    struct in_addr ipv4addr;
    portno = PORT_NO;
    char response[10];
    char header[] = "transmit";

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    char server_addr[] = "127.0.0.1";
    inet_pton(AF_INET, server_addr, &ipv4addr);
    server = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AF_INET);
    // printf("\n[client] Host name: %s\n", server->h_name);
    printf("\n[client] Server address: %s\n", server_addr);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length); 
    serv_addr.sin_port = htons(portno);

    // finished initialize, try to connect

    if (connect(sockfd,(struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
    {
        printf("-------- The server is not available now. ---------\n\n");
        global_stop = 1;
        exit(0);
        // error("ERROR connecting");
    }
    else
    {
        printf("[client] transmit thread get connection to server\n");
        printf("[client] start transmitting current frame\n\n");
    }

    // send the header first
    n = write(sockfd, header, sizeof(header));
    if (n < 0) 
         error("ERROR writing to socket");

    // get the response
    n = read(sockfd, response, sizeof(response));
    if (n < 0) 
         error("ERROR reading from socket");
  
    /*-------------------end----------------------*/

    // // create window
    // namedWindow("Real-Time CPS", 1);

    while (capture.isOpened() && !global_stop)
    {
        ++count;
        capture.read(frame);
        // capture >> frame;
        // writer << frame;
    
        if (count >= 30 && !frame.empty()) {
            count = 0;


            // set up the file name and encode the frame to jpeg
            sprintf(file_name, "pics/%s-%d.jpeg", userID, index);
            imwrite(file_name, frame, compression_params);
            ++index;

            /*-------------------send current frame here--------------*/

            pthread_t thread_id;
            struct arg_transmit trans_info;
            trans_info.sock = sockfd;
            strcpy(trans_info.file_name, file_name);
            /* create thread and pass socket and file name to send file */
            if (pthread_create(&thread_id, 0, transmit_child, (void *)&(trans_info)) == -1)
            {
                fprintf(stderr,"pthread_create error!\n");
                break; //break while loop
            }
            pthread_detach(thread_id);

            /*---------------------------end--------------------------*/
        }
        
        // if (drawCircle)
        // {
        //     circle(frame, center, r, Scalar(0, 0, 255), 4);
        // }
        if (drawResult)
        {
            putText(frame, resultShown, Point( frame.rows / 8,frame.cols / 8), CV_FONT_HERSHEY_COMPLEX, 1, Scalar(0, 0, 255), 4);
            line(frame, cvPoint(coord[0], coord[1]), cvPoint(coord[2], coord[3]), Scalar(0, 0, 255), 2);
            line(frame, cvPoint(coord[2], coord[3]), cvPoint(coord[4], coord[5]), Scalar(0, 0, 255), 2);
            line(frame, cvPoint(coord[4], coord[5]), cvPoint(coord[6], coord[7]), Scalar(0, 0, 255), 2);
            line(frame, cvPoint(coord[6], coord[7]), cvPoint(coord[0], coord[1]), Scalar(0, 0, 255), 2);
        }

        if(!frame.empty()){
            imshow("Real-Time CPS", frame);
        }
        if (index == 1 && count == 1) {
            moveWindow("Real-Time CPS", 100, 150 ); 
        }
        if (cvWaitKey(20) == 27)
        {
            break;
        }
        // usleep(1000 * DELAY);
    }
    
    close(sockfd); // disconnect server
    printf("[client] connection closed\n\n");
    global_stop = 1;
    // exit(0);

    return NULL;
}

/******************************************************************************
Description.: calling this function stops the running threads
Input Value.: -
Return Value: always 0
******************************************************************************/
int client_stop()
{
    // DBG("will cancel threads\n");
    printf("Canceling threads.\n");
    pthread_cancel(transmitThread);
    pthread_cancel(resultThread);
    return 0;
}

/******************************************************************************
Description.: calling this function creates and starts the client threads
Input Value.: -
Return Value: always 0
******************************************************************************/
int client_run()
{
    // DBG("launching threads\n");
    printf("\nLaunching threads.\n");
    pthread_create(&transmitThread, 0, transmit_thread, NULL);
    pthread_detach(transmitThread);
    pthread_create(&resultThread, 0, result_thread, NULL);
    pthread_detach(resultThread);
    return 0;
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
    usleep(1000 * 1000);

    /* clean up threads */
    printf("Force cancellation of threads and cleanup resources.\n");
    client_stop();

    usleep(1000 * 1000);

    printf("Done.\n");

    exit(0);
    return;
}


int main(int argc, char *argv[])
{
    char defUserID[] = "default";
    userID = strdup(defUserID);
    
    /* parameter parsing */
    while(1) {
        int option_index = 0, c = 0;
        static struct option long_options[] = {
            {"h", no_argument, 0, 0},
            {"help", no_argument, 0, 0},
            {"v", no_argument, 0, 0},
            {"version", no_argument, 0, 0},
            {"id", required_argument, 0, 0},
            {0, 0, 0, 0}
        };

        c = getopt_long_only(argc, argv, "", long_options, &option_index);

        /* no more options to parse */
        if(c == -1) break;

        /* unrecognized option */
        if(c == '?') {
            help();
            return 0;
        }

        switch(option_index) {
            /* h, help */
        case 0:
        case 1:
            help();
            return 0;
            break;

            /* v, version */
        case 2:
        case 3:
            printf("Real-Time CPS Client Version: 0.1\n" \
            "Compilation Date.....: unknown\n" \
            "Compilation Time.....: unknown\n");
            return 0;
            break;

            /* id, user id */
        case 4:
            userID = strdup(optarg);
            // printf("userID: %s\n", userID);
            // return 0;
            break;

        default:
            help();
            return 0;
        }
    }

    /* register signal handler for <CTRL>+C in order to clean up */
    if(signal(SIGINT, signal_handler) == SIG_ERR) {
        printf("could not register signal handler\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_mutex_init(&sendLock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }

    client_run();

    pause();

    return 0;
}
