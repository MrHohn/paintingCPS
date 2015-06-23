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

using namespace cv;
using namespace std;

#define BUFFER_SIZE               1024  
#define PORT_NO                  20001
#define DELAY                        1 // ms

static pthread_t transmitThread;
static pthread_t resultThread;

int global_stop = 0;
// Point center = Point(255,255);
// int r = 100;
// int drawCircle = 0;
// int drawLetter = 0;
// string letterShown = "Human";
int drawResult = 0;
string resultShown = "";

/******************************************************************************
Description.: tempalte for transfer integer to string
Input Value.:
Return Value:
******************************************************************************/
template <typename T>
string NumberToString ( T Number )
{
 ostringstream ss;
 ss << Number;
 return ss.str();
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
    char buffer[40];
    char header[] = "result";
    char response[10];
    // char *resultTemp = "";

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
        if (read(sockfd, buffer, sizeof(buffer)) < 0) 
        {
            error("ERROR reading from socket");
        }
        else if (n > 0)
        {
        //     printf("\n-------------------------------------\n");
        //     printf("[client] result from the server: %s", buffer);
        //     printf("-------------------------------------\n\n");
        // }
        // if (strcmp(buffer, "circle\n") == 0) 
        // {
        //     drawCircle = 1;
        // }
        // else if (strcmp(buffer, "letter\n") == 0) 
        // {
        //     drawLetter = 1;
        // }
        // else if (strcmp(buffer, "none\n") == 0)
        // {
        //     drawCircle = 0;
        //     drawLetter = 0;
        // }
        
            if (strcmp(buffer, "none") == 0)
            {
                drawResult = 0;
            }
            else
            {
                resultShown = buffer;
                resultShown = "matched index: " + resultShown;
                drawResult = 1;
                // printf("here\n");
                // printf("result from server: %s", buffer);
                // cout << resultShown << endl;
                // sprintf(resultTemp, "matched index: %s", buffer);
                // resultShown = resultTemp;
            }
        }
    }

    close(sockfd); // disconnect server
    printf("[client] connection closed\n");

    /*---------------------------end--------------------------*/

    // // /* cleanup now */
    // // pthread_cleanup_pop(1);
    global_stop = 1;

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
    
    // string file_name;
    char file_name[20] = {0};
    VideoCapture capture(0);
    Mat frame;

    // set up the image format and the quality
    vector<int> compression_params;
    compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
    compression_params.push_back(95);


    /*-----------------network part--------------*/

    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char bufferSend[BUFFER_SIZE];
    struct in_addr ipv4addr;
    char header[] = "transmit"; 
    char response[10];
    portno = PORT_NO;
  
    /*-------------------end----------------------*/

    // double rate = capture.get(CV_CAP_PROP_FPS);
    // printf("FPS: %f\n", rate);

    // // create window
    // namedWindow("Real-Time CPS", 1);

    while (capture.isOpened() && !global_stop)
    {
        ++count;
        capture.read(frame);
        // capture >> frame;
    
        if (count == 60) {
            count = 0;
            // writer << frame;
            // file_name = to_string(index);
            // file_name = NumberToString(index);
            // file_name = "pics/" + file_name + ".jpeg";
            sprintf(file_name, "pics/%d.jpeg", index);
            imwrite(file_name, frame, compression_params);
            ++index;

            /*-------------------send current frame here--------------*/

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

            // printf("Please enter the file name: ");
            // bzero(bufferSend,BUFFER_SIZE);
            // fgets(bufferSend,BUFFER_SIZE - 1,stdin);
            
            // send the header first
            n = write(sockfd, header, sizeof(header));
            if (n < 0) 
                 error("ERROR writing to socket");

            // get the response
            n = read(sockfd, response, sizeof(response));
            if (n < 0) 
                 error("ERROR reading from socket");

            // send the file name
            printf("[client] file: %s\n", file_name);
            n = write(sockfd, file_name, sizeof(file_name));
            if (n < 0) 
                 error("ERROR writing to socket");

            // get the response
            n = read(sockfd, response, sizeof(response));
            if (n < 0) 
                 error("ERROR reading from socket");

            // char file_name[] = "./pics/client.jpg";
            // char file_name[1024];
            // strcpy(file_name, buffer2);
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
                    // printf("file_block_length = %d\n", file_block_length);  

                    // send data to the client side  
                    if (send(sockfd, bufferSend, file_block_length, 0) < 0)  
                    // if (send(sockfd, bufferSend, BUFFER_SIZE, 0) < 0)  
                    {  
                        printf("Send File:\t%s Failed!\n", file_name);  
                        break;  
                    }  
                    // send(sockfd, "", 0, 0);

                    bzero(bufferSend, BUFFER_SIZE);  
                }

                fclose(fp);  
                printf("[client] transfer finished\n");  
            }

            /*---------------------------end--------------------------*/

            close(sockfd); // disconnect server
            printf("[client] connection closed\n\n");
        }
        
        // if (drawCircle)
        // {
        //     circle(frame, center, r, Scalar(0, 0, 255), 4);
        // }
        // if (drawLetter)
        // {
        //     putText(frame, letterShown, Point( frame.rows / 8,frame.cols / 8), CV_FONT_HERSHEY_COMPLEX, 1, Scalar(0, 0, 255), 4);
        // }
        if (drawResult)
        {
            putText(frame, resultShown, Point( frame.rows / 8,frame.cols / 8), CV_FONT_HERSHEY_COMPLEX, 1, Scalar(0, 0, 255), 4);
        }

        imshow("Real-Time CPS", frame);
        if (index == 1 && count == 1) {
            moveWindow("Real-Time CPS", 100, 150 ); 
        }
        if (cvWaitKey(20) == 27)
        {
            break;
        }
        // usleep(1000 * DELAY);
    }

    global_stop = 1;
    // exit(0);
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


int main()
{
    /* register signal handler for <CTRL>+C in order to clean up */
    if(signal(SIGINT, signal_handler) == SIG_ERR) {
        printf("could not register signal handler\n");
        exit(EXIT_FAILURE);
    }

    client_run();

    pause();

    return 0;
}
