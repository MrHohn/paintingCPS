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
    ret = mfopen(&handle, "basic\0", 0, src_GUID);
    if(ret)
    {
        printf("mfopen error\n"); 
        exit(1);
    }
    /* finish init */

    // init the mutex lock
    if (pthread_mutex_init(&id_lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        exit(1);
    }

    // init the mutex lock
    if (pthread_mutex_init(&sem_map_lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        exit(1);
    }

    // init the mutex lock
    if (pthread_mutex_init(&queue_map_lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        exit(1);
    }

    // init the mutex lock
    if (pthread_mutex_init(&send_lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        exit(1);
    }

    // init the mutex lock
    if (pthread_mutex_init(&recv_lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        exit(1);
    }    
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

    ret = mfsend(&handle, header, sizeof(header), dst_GUID, 0);
    if(ret < 0){
        printf("mfrec error\n"); 
        exit(1);
    }

    int id_temp = mfsockid;
    pthread_mutex_unlock(&id_lock);

    return id_temp;
}

// init a new mf socket and return the new created mf socket id, passive side
int MsgDistributor::accept()
{
    if (mfsockid == -1)
    {
        printf("ERROR: Init MsgDistributor first!\n");
        exit(1);
    }

    return mfsockid;
}
