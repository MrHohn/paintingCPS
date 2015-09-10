/*************************************************
#
# Purpose: main program for "CPS client"
# Author.: Zihong Zheng (zzhonzi@gmail.com)
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
#include <errno.h>
#include <unordered_set>
#include "MsgDistributor.h"
// #include "AEScipher.h"

using namespace cv;
using namespace std;

#define BUFFER_SIZE               1024  
#define PORT_NO                  20001
#define DELAY                        1 // ms

static pthread_t transmitThread;
static pthread_t resultThread;
static pthread_t orbitThread;
static pthread_t mflistenThread;

pthread_mutex_t sendLock;  // mutex lock to make sure transmit order
unordered_set<int> id_set; // set to store the sock id used  
int global_dst_GUID; // used for close command

int global_stop = 0;
int orbit = 0;
int sb = 0;
bool test = false;
char *userID;
int debug = 0;
MsgDistributor MsgD;
int drawResult = 0;
string resultShown = "";
string result_title = "";
string result_artist = "";
string result_date = "";
float coord[8];
int delay_time = 0;

struct arg_transmit {
    int sock;
    char file_name[100];
};

/******************************************************************************
Description.: Display a help message
Input Value.: -
Return Value: -
******************************************************************************/
void help(void)
{
    fprintf(stderr, " \n" \
            " Help for client-OpenCV application\n" \
            " ---------------------------------------------------------------\n" \
            " The following parameters can be passed to this software:\n\n" \
            " [-h | --help ]........: display this help\n" \
            " [-v | --version ].....: display version information\n" \
            " [-id].................: user ID to input\n" \
            " [-orbit]..............: run in orbit mode\n" \
            " [-d]..................: debug mode, print more details\n" \
            " [-m]..................: mine GUID, a.k.a src_GUID\n" \
            " [-o]..................: other's GUID, a.k.a dst_GUID\n" \
            " [-t]..................: testing mode, please define delay\n" \
            " \n" \
            " ---------------------------------------------------------------\n" \
            " Please start the client after the server is started\n"
            " Your need to be the root or use sudo to run the orbit mode\n"
            " Last, you should run the mfstack first before run this application in orbit mode\n"
            " ---------------------------------------------------------------\n" \
            " sample commands:\n" \
            "   # run in debug mode\n" \
            "   ./client-OpenCV -d\n\n" \
            "   # input a specific id\n" \
            "   ./client-OpenCV -id yourid\n\n" \
            "   # run in test mode, send out the sample image per 2000ms\n" \
            "   ./client-OpenCV -t 2000\n\n" \
            "   # run in orbit mode, myGUID set to 101, otherGUID set to 102\n" \
            "   sudo ./client-OpenCV -orbit -m 101 -o 102\n" \
            " \n");
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
    if (debug) printf("In the receiver thread.\n");
    char buffer[BUFFER_SIZE];
    char header[100];
    char response[10];
    sprintf(header, "result,%s", userID);
    char *resultTemp;
    int sockfd, n;

    /*-----------------network part--------------*/

    if (!orbit)
    {
        int portno;
        struct sockaddr_in serv_addr;
        struct hostent *server;
        struct in_addr ipv4addr;
        portno = PORT_NO;
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) 
            error("ERROR opening socket");
        if (!sb)
        {
            char server_addr[] = "127.0.0.1";
            inet_pton(AF_INET, server_addr, &ipv4addr);
            server = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AF_INET);
        }
        else
        {
            server = gethostbyname("sb");        
        }
        if (debug) printf("\n[client] Host name: %s\n", server->h_name);
        // printf("\n[client] Server address: %s\n", server_addr);
        if (server == NULL) 
        {
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
        if (strcmp(response, "failed") == 0)
        {
            errno = EACCES;
            error("ERROR log in failed");
        }
    }
    // below is the orbit mode using MFAPI
    else
    {
        sockfd = MsgD.connect();
        // insert the new id into set
        id_set.insert(sockfd);
        printf("[client] result thread get connection to server\n");
        printf("[client] start receiving the result\n");
        // send the header first
        n = MsgD.send(sockfd, header, sizeof(header));
        if (n < 0) 
             error("ERROR writing to socket");

        // get the response
        n = MsgD.recv(sockfd, response, sizeof(response));
        if (n < 0) 
             error("ERROR reading from socket");
        if (strcmp(response, "failed") == 0)
        {
            errno = EACCES;
            error("ERROR log in failed");
        }

    }
    
    while(!global_stop)
    {
        bzero(buffer, sizeof(buffer));
        if (!orbit)
        {
            n = recv(sockfd, buffer, sizeof(buffer), 0);        
        }
        else
        {
            if (debug) printf("Still waiting here for the new result\n");
            n = MsgD.recv(sockfd, buffer, sizeof(buffer));
        }

        if (n < 0) 
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
                if (debug) printf("result: %s\n", buffer);
                result_title = strtok(buffer, ",");
                result_title = "Title: " + result_title;
                result_artist = strtok(NULL, ",");
                result_artist = "Artist: " + result_artist;
                result_date = strtok(NULL, ",");
                result_date = "Date: " + result_date;
                for (int i = 0; i < 8; ++i) {
                    resultTemp = strtok(NULL, ",");
                    coord[i] = atof(resultTemp);
                    if (debug) printf("%f\n", coord[i]);
                }
                drawResult = 1;
            }
        }
        else
        {
            printf("[client] server closed the connection\n");
            global_stop = 1;
        }
    }
    if (!orbit)
    {
        close(sockfd); // disconnect server    
    }
    printf("[client] connection closed --- result\n");

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
    if (debug) printf("transmit child thread\n");

    struct arg_transmit *args = (struct arg_transmit *)arg;
    int sockfd = args->sock;
    char *file_name = args->file_name;

    if (!orbit) {
        int n;
        char bufferSend[BUFFER_SIZE];
        char response[10];

        // stat of file, to get the size
        struct stat file_stat;

        // get the status of file
        if (stat(file_name, &file_stat) == -1)
        {
            perror("stat");
            exit(EXIT_FAILURE);
        }
        
        if (debug) printf("file size: %ld\n", file_stat.st_size);

        // gain the lock, insure transmit order
        pthread_mutex_lock(&sendLock);

        // send the file info, combine with ','
        printf("[client] file name: %s\n", file_name);
        bzero(bufferSend, sizeof(bufferSend)); 
        sprintf(bufferSend, "%s,%ld", file_name, file_stat.st_size);

        // send and read through the tcp socket
        n = write(sockfd, bufferSend, sizeof(bufferSend));
        if (n < 0) 
            error("ERROR writing to socket");

        // get the response
        n = read(sockfd, response, sizeof(response));
        if (n < 0) 
            error("ERROR reading from socket");

        FILE *fp = fopen(file_name, "r");  
        if (fp == NULL)  
        {  
            printf("File:\t%s Not Found!\n", file_name);
            exit(0);
        }  
        else  
        {  
            bzero(bufferSend, sizeof(bufferSend));  
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
    }
    // below is orbit mode, using MFAPI
    else
    {
        struct stat file_stat; // stat of file, to get the size
        int n;

        // get the id length and send length
        int id_length = 1;
        int divisor = 10;
        while (sockfd / divisor > 0)
        {
            ++id_length;
            divisor *= 10;
        }
        // int send_size = BUFFER_SIZE - 6 - id_length;
        int send_size = BUFFER_SIZE * 4; //4096 bytes per time
        char bufferSend[send_size];
        
        // get the status of file
        if (stat(file_name, &file_stat) == -1)
        {
            perror("stat");
            exit(EXIT_FAILURE);
        }
        if (debug) printf("one time size: %d\n", send_size);
        printf("file size: %ld\n", file_stat.st_size);

        // gain the lock, insure transmit order
        pthread_mutex_lock(&sendLock);

        // send the file info, combine with ','
        printf("[client] file name: %s\n", file_name);
        sprintf(bufferSend, "%s,%ld", file_name, file_stat.st_size);

        // send through the socket
        n = MsgD.send(sockfd, bufferSend, 100);
        if (n < 0) 
            error("ERROR writing to socket");

        FILE *fp = fopen(file_name, "r");  
        if (fp == NULL)  
        {  
            printf("File:\t%s Not Found!\n", file_name);  
            exit(0);
        }  
        else  
        {  
            bzero(bufferSend, sizeof(bufferSend));

            // get the response
            n = MsgD.recv(sockfd, bufferSend, 10);
            bzero(bufferSend, sizeof(bufferSend));

            int file_block_length = 0;
            // start transmitting the file
            while( (file_block_length = fread(bufferSend, sizeof(char), send_size, fp)) > 0)  
            {  
                // if (debug) printf("send length: %d\n", file_block_length);
                // send data to the client side  
                if (MsgD.send(sockfd, bufferSend, send_size) < 0)  
                {  
                    printf("Send File: %s Failed!\n", file_name);  
                    break;  
                }

                bzero(bufferSend, sizeof(bufferSend)); 
            }

            fclose(fp);  
            printf("[client] Transfer Finished!\n\n");  
        }
    }

    pthread_mutex_unlock(&sendLock);
    pthread_exit(NULL);

    return NULL;
}

/******************************************************************************
Description.: this is the display thread
              it loops forever, grabs a fresh frame and stores it to file,
              also send it out to the server
Input Value.:
Return Value:
******************************************************************************/
void *display_thread(void *arg)
{
    int index = 1;
    int count = 0;
    
    char file_name[100] = {0};



    /*-----------------network part--------------*/

    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    struct in_addr ipv4addr;
    portno = PORT_NO;
    char response[10];
    char header[100];
    sprintf(header, "transmit,%s", userID);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    if (!sb)
    {
        char server_addr[] = "127.0.0.1";
        inet_pton(AF_INET, server_addr, &ipv4addr);
        server = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AF_INET);
    }
    else
    {
        server = gethostbyname("sb");   
    }
    if (debug) printf("\n[client] Host name: %s\n", server->h_name);
    // printf("\n[client] Server address: %s\n", server_addr);
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
    if (strcmp(response, "failed") == 0)
    {
        errno = EACCES;
        error("ERROR log in failed");
    }
  
    /*-------------------end----------------------*/


    if (!test) {

        // // create window
        // namedWindow("Real-Time CPS", 1);

        VideoCapture capture(0);
        Mat frame;

        // set up the image format and the quality
        capture.set(CV_CAP_PROP_FRAME_WIDTH, 800);
        capture.set(CV_CAP_PROP_FRAME_HEIGHT, 600);
        vector<int> compression_params;
        compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
        compression_params.push_back(95);

        while (capture.isOpened() && !global_stop)
        {
            ++count;
            capture.read(frame);
            // capture >> frame;
            // writer << frame;
        
            if (count >= 50 && !frame.empty()) {
                count = 0;


                // set up the file name and encode the frame to jpeg
                sprintf(file_name, "pics/%s-%d.jpg", userID, index);
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
            
            if (drawResult)
            {
                putText(frame, result_title, Point(10, 30), CV_FONT_HERSHEY_COMPLEX, 0.6, Scalar(150, 0, 0), 2);
                putText(frame, result_artist, Point(10, 60), CV_FONT_HERSHEY_COMPLEX, 0.6, Scalar(150, 0, 0), 2);
                putText(frame, result_date, Point(10, 90), CV_FONT_HERSHEY_COMPLEX, 0.6, Scalar(150, 0, 0), 2);
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

    }
    // test mode
    else
    {
        while (!global_stop)
        {
            ++count;
        
            if (count >= 10) {
                count = 0;

                printf("\n[test mode] send an image\n");

                // set up the file name and encode the frame to jpeg
                sprintf(file_name, "pics/orbit-sample.jpg");
                ++index;


                /*-------------------send current frame here--------------*/

                pthread_t thread_id;
                struct arg_transmit trans_info;
                trans_info.sock = sockfd;
                bzero(&trans_info.file_name, BUFFER_SIZE);
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
            
            if (drawResult)
            {
                if (debug) printf("drawResult: %d\n", drawResult); 
                printf("\n[test] got result from server\n");
                printf("[test] %s\n", result_title.c_str());
                printf("[test] %s\n", result_artist.c_str());
                printf("[test] %s\n\n", result_date.c_str());
                // printf("[test] coordinates as below:\n");
                // printf("%f, %f\n", coord[0], coord[1]);
                // printf("%f, %f\n", coord[2], coord[3]);
                // printf("%f, %f\n", coord[4], coord[5]);
                // printf("%f, %f\n\n", coord[6], coord[7]);
                drawResult = 0;
            }

            usleep(100 * delay_time); // sleep a while to imitate video catching
        }
        exit(1);
    }

    
    close(sockfd); // disconnect server
    printf("[client] connection closed --- transmit\n");
    global_stop = 1;
    // exit(0);

    return 0;
}

/******************************************************************************
Description.: this is the orbit thread
              it loops forever, send a sample image out to the server in 
              a certain frequency
Input Value.:
Return Value:
******************************************************************************/
void *orbit_thread(void *arg)
{
    printf("\n---------- RUN IN ORBIT MODE ----------\n");

    int index = 1;
    int count = 0;
    int sockfd;
    int n;
    
    char file_name[100] = {0};

    /*-----------------network part--------------*/

    char response[10];
    char header[100];
    sprintf(header, "transmit,%s", userID);

    sockfd = MsgD.connect();
    if (sockfd < 0) 
    {
        printf("-------- The server is not available now. ---------\n\n");
        global_stop = 1;
        exit(0);
    }
    else
    {
        // insert the new id into set
        id_set.insert(sockfd);
        printf("[client] transmit thread get connection to server\n");
        printf("[client] start transmitting current frame\n\n");
    }

    // send the header first
    n = MsgD.send(sockfd, header, sizeof(header));
    if (n < 0) 
         error("ERROR writing to socket");

    // get the response
    n = MsgD.recv(sockfd, response, sizeof(response));
    if (n < 0) 
         error("ERROR reading from socket");
    if (strcmp(response, "failed") == 0)
    {
        errno = EACCES;
        error("ERROR log in failed");
    }
  
    /*-------------------end----------------------*/


    if (!test)
    {
        // // create window
        // namedWindow("Real-Time CPS", 1);

        VideoCapture capture(0);
        Mat frame;

        // set up the image format and the quality
        capture.set(CV_CAP_PROP_FRAME_WIDTH, 800);
        capture.set(CV_CAP_PROP_FRAME_HEIGHT, 600);
        vector<int> compression_params;
        compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
        compression_params.push_back(95);

        while (capture.isOpened() && !global_stop)
        {
            ++count;
            capture.read(frame);
            // capture >> frame;
            // writer << frame;
        
            if (count >= 50 && !frame.empty()) {
                count = 0;


                // set up the file name and encode the frame to jpeg
                sprintf(file_name, "pics/%s-%d.jpg", userID, index);
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
            
            if (drawResult)
            {
                putText(frame, result_title, Point(10, 30), CV_FONT_HERSHEY_COMPLEX, 0.6, Scalar(150, 0, 0), 2);
                putText(frame, result_artist, Point(10, 60), CV_FONT_HERSHEY_COMPLEX, 0.6, Scalar(150, 0, 0), 2);
                putText(frame, result_date, Point(10, 90), CV_FONT_HERSHEY_COMPLEX, 0.6, Scalar(150, 0, 0), 2);
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
    }
    // below is mf-test mode
    else{

        while (!global_stop)
        {
            ++count;
        
            if (count >= 10) {
                count = 0;

                printf("[mf-test] send an image\n");

                // set up the file name and encode the frame to jpeg
                sprintf(file_name, "pics/orbit-sample.jpg");
                ++index;


                /*-------------------send current frame here--------------*/

                pthread_t thread_id;
                struct arg_transmit trans_info;
                trans_info.sock = sockfd;
                bzero(&trans_info.file_name, BUFFER_SIZE);
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
            
            if (drawResult)
            {
                if (debug) printf("drawResult: %d\n", drawResult); 
                printf("\n[mf-test] got result from server\n");
                printf("[mf-test] %s\n", result_title.c_str());
                printf("[mf-test] %s\n", result_artist.c_str());
                printf("[mf-test] %s\n\n", result_date.c_str());
                // printf("[mf-test] coordinates as below:\n");
                // printf("%f, %f\n", coord[0], coord[1]);
                // printf("%f, %f\n", coord[2], coord[3]);
                // printf("%f, %f\n", coord[4], coord[5]);
                // printf("%f, %f\n\n", coord[6], coord[7]);
                drawResult = 0;
            }

            usleep(100 * delay_time); // sleep a while to imitate video catching
        }
    }

    // MsgD.close(sockfd, 0);
    printf("[client] connection closed --- transmit\n");
    global_stop = 1;
    pause();
    // exit(0);

    return NULL;
}

/******************************************************************************
Description.: this is the mflisten thread
              it loops forever, listen on the src_GUID
Input Value.:
Return Value:
******************************************************************************/
void *mflisten_thread(void *arg)
{
    if (debug) printf("\nin listen thread\n");
    while (!global_stop) {
        MsgD.listen();
    }

    return NULL;
}

/******************************************************************************
Description.: calling this function stops the running threads
Input Value.: -
Return Value: always 0
******************************************************************************/
int client_stop()
{
    int n;
    printf("Canceling threads.\n");
    n = pthread_cancel(resultThread);
    if (n)
    {
        printf("failed to cancel result thread\n");
    }
    if (!orbit)
    {
        n = pthread_cancel(transmitThread);
        if (n)
        {
            printf("failed to cancel thread\n");
        }
    }
    else
    {
        // cancel orbit thread
        n = pthread_cancel(orbitThread);
        if (n)
        {
            printf("failed to cancel orbit thread\n");
        }
        // cancel listen thread
        n = pthread_cancel(mflistenThread);
        if (n)
        {
            printf("failed to cancel listen thread\n");
        }
        // send connection close request
        printf("[client] now send the disconnection request.\n");
        // printf("src GUID: %d, dst GUID: %d\n", MsgD.src_GUID, MsgD.dst_GUID);
        struct Handle handle;
        mfopen(&handle, "basic\0", 0, global_dst_GUID);
        char content[BUFFER_SIZE];
        for (const auto &elem : id_set)
        {
            bzero(content, BUFFER_SIZE);
            sprintf(content, "close,%d", elem);
            mfsend(&handle, content, sizeof(content), global_dst_GUID, 0);
            // MsgD.close(elem, 0);
        }
        mfclose(&handle);
    }

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
    pthread_create(&resultThread, 0, result_thread, NULL);
    pthread_detach(resultThread);
    if (!orbit)
    {
        pthread_create(&transmitThread, 0, display_thread, NULL);
        pthread_detach(transmitThread);
    }
    // run in orbit mode
    else
    {
        pthread_create(&orbitThread, 0, orbit_thread, NULL);
        pthread_detach(orbitThread);
        // mflisten_thread();
        pthread_create(&mflistenThread, 0, mflisten_thread, NULL);
        pthread_detach(mflistenThread);
    }



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

    // if (orbit)
    // {
    //     delete(&MsgD);    
    // }
    printf("Done.\n");

    exit(0);
    return;
}


int main(int argc, char *argv[])
{
    char defUserID[] = "default";
    userID = strdup(defUserID);
    int src_GUID = -1, dst_GUID = -1;
    
    /* parameter parsing */
    while(1) 
    {
        int option_index = 0, c = 0;
        static struct option long_options[] =
        {
            {"h", no_argument, 0, 0},
            {"help", no_argument, 0, 0},
            {"v", no_argument, 0, 0},
            {"version", no_argument, 0, 0},
            {"id", required_argument, 0, 0},
            {"orbit", no_argument, 0, 0},
            {"d", no_argument, 0, 0},
            {"m", required_argument, 0, 0},
            {"o", required_argument, 0, 0},
            {"t", required_argument, 0, 0},
            {"sb", no_argument, 0, 0},
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
            printf("Real-Time CPS Client Version: 0.1\n" \
            "Compilation Date.....: unknown\n" \
            "Compilation Time.....: unknown\n");
            return 0;
            break;

            /* id, user id */
        case 4:
            userID = strdup(optarg);
            if (debug) printf("userID: %s\n", userID);
            break;

            /* orbit, run in orbit mode */
        case 5:
            orbit = 1;
            break;

            /* debug mode */
        case 6:
            debug = 1;
            break;

            /* mine GUID */
        case 7:
            src_GUID = strtol(optarg, NULL, 10);
            break;

            /* other's GUID */
        case 8:
            dst_GUID = strtol(optarg, NULL, 10);
            global_dst_GUID = dst_GUID;
            break;

            /* testing mode delay */
        case 9:
            delay_time = strtol(optarg, NULL, 10);
            test = true;
            break;

            /* sand box */
        case 10:
            sb = 1;
            break;

        default:
            help();
            return 0;
        }
    }

    /* register signal handler for <CTRL>+C in order to clean up */
    if (!orbit) {
        if(signal(SIGINT, signal_handler) == SIG_ERR)
        {
            printf("could not register signal handler\n");
            exit(EXIT_FAILURE);
        }

        if (pthread_mutex_init(&sendLock, NULL) != 0)
        {
            printf("\n mutex init failed\n");
            return 1;
        }   
    }

    if (orbit)
    {
        if (src_GUID != -1 && dst_GUID != -1)
        {
            if (debug) printf("src_GUID: %d, dst_GUID: %d\n", src_GUID, dst_GUID);
            /* init new Message Distributor */
            MsgD.init(src_GUID, dst_GUID, debug);
        }
        else
        {
            printf("ERROR: please enter src_GUID and dst_GUID with flags -m & -o\n");
            exit(1);
        }

    }

    client_run();

    pause();

    return 0;
}
