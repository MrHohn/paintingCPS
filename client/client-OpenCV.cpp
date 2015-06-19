#include <stdio.h>
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

using namespace cv;
using namespace std;

#define BUFFER_SIZE               1024  
#define PORT_NO                  20001
#define DELAY                       5 // ms

static pthread_t transmitThread;
static pthread_t resultThread;

int global_stop = 0;

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
    printf("In the receiver thread.\n");

    /*-----------------network part--------------*/

    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    struct in_addr ipv4addr;
    char buffer[BUFFER_SIZE];
    char header[] = "result"; 
    char response[10];

    portno = PORT_NO;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    inet_pton(AF_INET, "127.0.0.1", &ipv4addr);
    server = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AF_INET);
    printf("\n[client] Host name: %s\n", server->h_name);
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
        printf("\n-------- The server is not available now. ---------\n\n");
        error("ERROR connecting");
    }
    printf("[client] result thread get connection to server\n");
    printf("[client] start receiving the result\n");

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
        n = read(sockfd, buffer, sizeof(buffer));
        if (n < 0) 
         error("ERROR writing to socket");
        else if (n > 0)
        {
            printf("\n-------------------------------------\n");
            printf("[client] result from the server: %s", buffer);
            printf("-------------------------------------\n\n");
       }
    }

    close(sockfd); // disconnect server
    printf("[client] connection closed\n");

    /*---------------------------end--------------------------*/

    // // /* cleanup now */
    // // pthread_cleanup_pop(1);

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
    
    string file_name;
    VideoCapture capture(0);
    Mat frame;

    // set up the image format and the quality
    vector<int> compression_params;
    compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
    compression_params.push_back(95);

    // double rate = capture.get(CV_CAP_PROP_FPS);
    // printf("FPS: %f\n", rate);

    // // create window
    // namedWindow("Real-Time CPS", 1);

    while (capture.isOpened() && !global_stop)
    {
        ++count;
        capture.read(frame);
        // capture >> frame;
        imshow("Real-Time CPS", frame);
        if (count == 30) {
            count = 0;
            // writer << frame;
            // file_name = to_string(index);
            file_name = NumberToString(index);
            imwrite("pics/" + file_name + ".jpeg", frame, compression_params);
            ++index;
        }
        if (cvWaitKey(20) == 27)
        {
            break;
        }
        usleep(1000 * DELAY);
    }
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
    // pthread_cancel(resultThread);
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
    printf("Launching threads.\n");
    pthread_create(&transmitThread, 0, transmit_thread, NULL);
    pthread_detach(transmitThread);
    // pthread_create(&resultThread, 0, result_thread, NULL);
    // pthread_detach(resultThread);
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