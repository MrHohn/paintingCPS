/*************************************************
#
# Purpose: "Message Distributor" aims to distribute the message
			received through the unique GUID
# Author.: Zihong Zheng
# Version: 0.1
# License: 
#
*************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sstream>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include "MsgDistributor.h"

MsgDistributor::MsgDistributor()
{
    mfsockid = -1;
}

MsgDistributor::~MsgDistributor()
{
    pthread_mutex_destroy(&queue_map_lock);
    pthread_mutex_destroy(&sem_map_lock);
    pthread_mutex_destroy(&id_lock);
    pthread_mutex_destroy(&send_lock);
    pthread_mutex_destroy(&recv_lock);
    sem_destroy(&connect_sem);
    sem_destroy(&accept_sem);
    if (mfsockid != -1)
    {
    	// close mf here
    	mfclose(&handle);
    }
}

void MsgDistributor::init(int src_GUID, int dst_GUID)
{
    if (mfsockid != -1)
    {
        printf("ERROR: Dont reinit MsgDistributor!\n");
        exit(1);
    }

    mfsockid = 0;
    this->src_GUID = src_GUID;
    this->dst_GUID = dst_GUID;

    /* init mfapi here, actually only for the receive part */
    int ret = 0;
    printf("------ open the MF now -------\n");
    ret = mfopen(&handle, "basic\0", 0, src_GUID);
    if(ret)
    {
        printf("mfopen error\n"); 
        exit(1);
    }
    printf("------ Success -------\n");
    /* finish init */

    if (pthread_mutex_init(&id_lock, NULL) != 0
        || pthread_mutex_init(&sem_map_lock, NULL) != 0
        || pthread_mutex_init(&queue_map_lock, NULL) != 0
        || pthread_mutex_init(&send_lock, NULL) != 0
        || pthread_mutex_init(&recv_lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        exit(1);
    }

    if (sem_init(&connect_sem, 0, 0) != 0
        || sem_init(&accept_sem, 0, 0) != 0)
    {
        printf("semaphore init failed\n");
    }

}

// listen on the GUID
int MsgDistributor::listen()
{
    if (mfsockid == -1)
    {
        printf("ERROR: Init MsgDistributor first!\n");
        exit(1);
    }

    int ret = 0;
    char buffer[BUFFER_SIZE];

    // get the new message
    ret = mfrecv_blk(&handle, NULL, buffer, BUFFER_SIZE, NULL, 0);
    if(ret < 0){
        printf("mfrec error\n"); 
        exit(1);
    }

    char *new_message = strtok(buffer, ",");
    if (strcmp(new_message, "create") == 0)
    {
        sem_post(&accept_sem);
    }
    else if (strcmp(new_message, "accepted") == 0)
    {
        char *created_id_char = strtok(NULL, ",");
        int created_id = strtol(created_id_char, NULL, 10);
        connect_queue.push(created_id);
        sem_post(&connect_sem);
    }
    else if (strcmp(new_message, "sock") == 0)
    {
        char *id_char = strtok(NULL, ",");
        string content = strtok(NULL, ",");
        int id = strtol(id_char, NULL, 10);
        queue<string> *id_queue = queue_map[id];
        id_queue->push(content);
        sem_t *id_sem = sem_map[id];
        sem_post(id_sem);
    }
    else
    {
        printf("unable to classify this message, discard");
        return -1;
    }


    return mfsockid;
}

// init a new mf socket and return the new created mf socket id, intiative side
int MsgDistributor::connect()
{
    if (mfsockid == -1)
    {
        printf("ERROR: Init MsgDistributor first!\n");
        exit(1);
    }

    int ret = 0;
    char header[BUFFER_SIZE];
    sprintf(header, "create");
    pthread_mutex_lock(&id_lock);

    // send the connect request
    pthread_mutex_lock(&send_lock);
    ret = mfsend(&handle, header, sizeof(header), dst_GUID, 0);
    if(ret < 0){
        printf ("mfsendmsg error\n");
        pthread_mutex_unlock(&send_lock);
        pthread_mutex_unlock(&id_lock);
        exit(1);
    }
    pthread_mutex_unlock(&send_lock);

    // wait for the response
    sem_wait(&connect_sem);
    if (stop)
    {
        return -1;
    }

    int newID = connect_queue.front();
    connect_queue.pop();

    // create new queue and semaphore for the new conncection
    sem_t *new_connection_sem = new sem_t(); // create a new semaphore in heap
    if (sem_init(new_connection_sem, 0, 0) != 0)
    {
        printf("semaphore init failed\n");
    }
    queue<string> *new_connection_queue = new queue<string>(); // queue storing the message
    pthread_mutex_lock(&queue_map_lock);
    pthread_mutex_lock(&sem_map_lock);
    queue_map[newID] = new_connection_queue; // put the address of queue into map
    sem_map[newID] = new_connection_sem; // put the address of semaphore into map
    pthread_mutex_unlock(&queue_map_lock);
    pthread_mutex_unlock(&sem_map_lock);

    pthread_mutex_unlock(&id_lock);

    return newID;
}

// accept a new connection, return the socket id
int MsgDistributor::accept()
{
    if (mfsockid == -1)
    {
        printf("ERROR: Init MsgDistributor first!\n");
        exit(1);
    }

    sem_wait(&accept_sem);
    if (stop)
    {
        return -1;
    }
    pthread_mutex_lock(&id_lock);
    // got a new connection, need to accpet, create a new id for it
    ++mfsockid;
    while (sem_map.find(mfsockid) != sem_map.end()) {
        ++mfsockid;
    }

    int ret = 0;
    char header[BUFFER_SIZE];
    sprintf(header, "accepted,%d", mfsockid);
    pthread_mutex_lock(&send_lock);
    ret = mfsend(&handle, header, sizeof(header), dst_GUID, 0);
    if(ret < 0){
        printf ("mfsendmsg error\n");
        pthread_mutex_unlock(&send_lock);
        pthread_mutex_unlock(&id_lock);
        exit(1);
    }
    pthread_mutex_unlock(&send_lock);
    int newID = mfsockid;

    // create new queue and semaphore for the new conncection
    sem_t *new_connection_sem = new sem_t(); // create a new semaphore in heap
    if (sem_init(new_connection_sem, 0, 0) != 0)
    {
        printf("semaphore init failed\n");
    }
    queue<string> *new_connection_queue = new queue<string>(); // queue storing the message
    pthread_mutex_lock(&queue_map_lock);
    pthread_mutex_lock(&sem_map_lock);
    queue_map[newID] = new_connection_queue; // put the address of queue into map
    sem_map[newID] = new_connection_sem; // put the address of semaphore into map
    pthread_mutex_unlock(&queue_map_lock);
    pthread_mutex_unlock(&sem_map_lock);

    pthread_mutex_unlock(&id_lock);

    return newID;
}

//  send the message according to the socket id
int MsgDistributor::send(int sock, char* buffer, int size)
{

    return 0;
}

//  receive the message according to the socket id
int MsgDistributor::recv(int sock, char* buffer, int size)
{
    return 0;
}

// close the message channel
int MsgDistributor::close(int sock)
{
    return 0;
}
