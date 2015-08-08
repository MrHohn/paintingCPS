/*************************************************
#
# Purpose: "Message Distributor" aims to distribute the message
            received through the unique GUID
# Author.: Zihong Zheng (zzhonzi@gmail.com)
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
    if (debug) printf("now delete the MsgDistributor class\n");
    pthread_mutex_destroy(&map_lock);
    pthread_mutex_destroy(&id_lock);
    pthread_mutex_destroy(&send_lock);
    pthread_mutex_destroy(&recv_lock);
    sem_destroy(&connect_sem);
    sem_destroy(&accept_sem);
    if (mfsockid != -1)
    {
        if (debug) printf("now close the mf\n");
        // close mf here
        mfclose(&handle);
    }
}

int MsgDistributor::init(int src_GUID, int dst_GUID, int set_debug)
{
    if (mfsockid != -1)
    {
        printf("ERROR: Dont reinit MsgDistributor!\n");
        return -1;
    }

    mfsockid = 0;
    this->src_GUID = src_GUID;
    this->dst_GUID = dst_GUID;
    debug = set_debug;

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
        || pthread_mutex_init(&map_lock, NULL) != 0
        || pthread_mutex_init(&send_lock, NULL) != 0
        || pthread_mutex_init(&recv_lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return -1;
    }

    if (sem_init(&connect_sem, 0, 0) != 0
        || sem_init(&accept_sem, 0, 0) != 0)
    {
        printf("semaphore init failed\n");
    }

    return 0;
}

// listen on the GUID
int MsgDistributor::listen()
{
    if (mfsockid == -1)
    {
        printf("ERROR: Init MsgDistributor first! (listen)\n");
        return -1;
    }

    int ret = 0;
    char buffer[MAX_CHUNK];

    // get the new message
    if (debug) printf("start listening on GUID: %d\n", src_GUID);
    ret = mfrecv_blk(&handle, NULL, buffer, MAX_CHUNK, NULL, 0);
    if(ret < 0)
    {
        printf("mfrec error\n"); 
        return -1;
    }

    if (debug) printf("ret: %d\n", ret);
    char *new_message = strtok(buffer, ",");
    if (debug) printf("receive new message, header: %s\n", new_message);
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
        int id = strtol(id_char, NULL, 10);
        if (debug) printf("sockid: %d\n", id);
        // if (debug) printf("id_char - new_message = %d\n", (int)(id_char - new_message));
        // get the remain all part as content
        int id_length = 1, divisor = 10;
        while (id / divisor != 0)
        {
            divisor *= 10;
            ++id_length;
        }
        int content_length = ret - id_length - 6;
        char *content_char = (char*)(id_char + id_length + 1);
        char *content = (char *)malloc(content_length);
        memcpy(content, content_char, content_length);
        // string content = content_char;
        if (sem_map.find(id) == sem_map.end())
        {
            printf("ERROR: Socket ID not exist (listen)\n");
            return -1;
        }
        else
        {
            queue<char*> *id_queue = queue_map[id];
            id_queue->push(content);
            sem_t *id_sem = sem_map[id];
            sem_post(id_sem);
        }
    }
    else if (strcmp(new_message, "close") == 0)
    {
        char *id_char = strtok(NULL, ",");
        int id = strtol(id_char, NULL, 10);
        if (debug) printf("peer closed the connection\n");
        this->close(id, 1);
    }
    else
    {
        printf("unable to classify this message, discard\n");
        return -1;
    }

    return 0;
}

// init a new mf socket and return the new created mf socket id, intiative side
int MsgDistributor::connect()
{
    if (mfsockid == -1)
    {
        printf("ERROR: Init MsgDistributor first!\n");
        return -1;
    }

    int ret = 0;
    char header[BUFFER_SIZE];
    sprintf(header, "create");
    pthread_mutex_lock(&id_lock);

    // send the connect request
    pthread_mutex_lock(&send_lock);
    if (debug) printf("now send the connect command to other\n");
    ret = mfsend(&handle, header, sizeof(header), dst_GUID, 0);
    if(ret < 0)
    {
        printf ("mfsendmsg error\n");
        pthread_mutex_unlock(&send_lock);
        pthread_mutex_unlock(&id_lock);
        return -1;
    }
    pthread_mutex_unlock(&send_lock);

    // wait for the response
    if (debug) printf("finished sent connet command, ret: %d\n", ret);
    if (debug) printf("wait for the connect response\n");
    sem_wait(&connect_sem);
    if (stop)
    {
        printf("connect stop\n");
        return -1;
    }

    if (debug) printf("got connect response\n");
    int newID = connect_queue.front();
    connect_queue.pop();
    if (debug) printf("accpeted id: %d\n", newID);

    // create new queue and semaphore for the new conncection
    sem_t *new_connection_sem = new sem_t(); // create a new semaphore in heap
    if (sem_init(new_connection_sem, 0, 0) != 0)
    {
        printf("semaphore init failed\n");
    }
    queue<char*> *new_connection_queue = new queue<char*>(); // queue storing the message
    pthread_mutex_lock(&map_lock);
    queue_map[newID] = new_connection_queue; // put the address of queue into map
    sem_map[newID] = new_connection_sem; // put the address of semaphore into map
    status_map[newID] = 1; // put the status of connection into map
    if (debug) printf("create and put addrs of queue, semaphore and status into maps\n");

    pthread_mutex_unlock(&map_lock);
    pthread_mutex_unlock(&id_lock);

    return newID;
}

// accept a new connection, return the socket id
int MsgDistributor::accept()
{
    if (mfsockid == -1)
    {
        printf("ERROR: Init MsgDistributor first! (accept)\n");
        return -1;
    }

    sem_wait(&accept_sem);
    if (stop)
    {
        printf("accept stop\n");
        return -1;
    }

    if (debug) printf("new connection needed to accept\n");    
    pthread_mutex_lock(&id_lock);
    // got a new connection, need to accpet, create a new id for it
    ++mfsockid;
    while (sem_map.find(mfsockid) != sem_map.end())
    {
        ++mfsockid;
    }

    if (debug) printf("create new id: %d\n", mfsockid);
    int ret = 0;
    char header[BUFFER_SIZE];
    sprintf(header, "accepted,%d,", mfsockid);
    pthread_mutex_lock(&send_lock);
    if (debug) printf("now response to other\n");    
    ret = mfsend(&handle, header, sizeof(header), dst_GUID, 0);
    if(ret < 0)
    {
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
    queue<char*> *new_connection_queue = new queue<char*>(); // queue storing the message
    pthread_mutex_lock(&map_lock);
    queue_map[newID] = new_connection_queue; // put the address of queue into map
    sem_map[newID] = new_connection_sem; // put the address of semaphore into map
    status_map[newID] = 1; // put the status of connection into map
    if (debug) printf("create and put addrs of queue, semaphore and status into maps\n");  

    pthread_mutex_unlock(&map_lock);
    pthread_mutex_unlock(&id_lock);

    return newID;
}

//  send the message according to the socket id
int MsgDistributor::send(int sock, char* buffer, int size)
{
    if (mfsockid == -1)
    {
        printf("ERROR: Init MsgDistributor first!\n");
        return -1;
    }
    if (sem_map.find(sock) == sem_map.end())
    {
        printf("ERROR: Socket ID not exist (send)\n");
        return -1;
    }

    // pthread_mutex_lock(&send_lock);
    int ret = 0;
    int id_length = 1, divisor = 10;
    while (sock / divisor != 0)
    {
        divisor *= 10;
        ++id_length;
    }
    int content_length = size + id_length + 6;
    char content[content_length];
    sprintf(content, "sock,%d,", sock);
    char *subindex = (char*)(content + 6 + id_length);
    memcpy(subindex, buffer, size);
    if (debug) printf("size: %d, content_length: %d\n", size, content_length);
    if (debug) printf("now send message in socket: %d\n", sock);
    ret = mfsend(&handle, content, sizeof(content), dst_GUID, 0);
    if(ret < 0)
    {
        printf ("mfsendmsg error\n");
        // pthread_mutex_unlock(&send_lock);
        return -1;
    }
    if (debug) printf("finish, ret: %d\n", ret);

    // pthread_mutex_unlock(&send_lock);

    return ret;

}

// TODO size might exceed the actual recv size, needed to fix
//  receive the message according to the socket id
int MsgDistributor::recv(int sock, char *buffer, int size)
{
    if (mfsockid == -1)
    {
        printf("ERROR: Init MsgDistributor first!\n");
        return -1;
    }
    // check if the sock id is valid
    if (sem_map.find(sock) == sem_map.end())
    {
        printf("ERROR: Socket ID not exist (recv: %d)\n", sock);
        return -1;
    }

    sem_t *recv_sem = sem_map[sock];
    // wait for buffer filled
    if (debug) printf("now wait for new message in sock: %d\n", sock);
    sem_wait(recv_sem);
    if (debug) printf("sock:%d new message arrive\n", sock);
    // check if the connection is closed
    if (status_map[sock] == 0)
    {
        if (debug) printf("sockid[%d]: connection is closed\n", sock);
        // remove the sock id from status map
        pthread_mutex_lock(&map_lock);
        status_map.erase(sock);
        pthread_mutex_unlock(&map_lock);
        return -1;
    }
    if (stop)
    {
        if (debug) printf("recv stop\n");
        return -1;
    }

    if (debug) printf("new message in socket: %d\n", sock);
    queue<char*> *recv_queue = queue_map[sock];
    char  *recv_char = recv_queue->front();
    recv_queue->pop();

    memcpy(buffer, recv_char, size);
    free(recv_char);

    return 1;
}

// close the message channel
int MsgDistributor::close(int sock, int passive)
{
    if (mfsockid == -1)
    {
        printf("ERROR: Init MsgDistributor first!\n");
        exit(1);
    }
    if (sem_map.find(sock) == sem_map.end())
    {
        printf("ERROR: Socket ID not exist (close)\n");
        return -1;
    }

    if (debug) printf("now close the socket: %d\n", sock);
    sem_t *close_sem = sem_map[sock];
    queue<char*> *close_queue = queue_map[sock];
    pthread_mutex_lock(&map_lock);
    // change the status from 1 to 0, means closed
    status_map[sock] = 0;
    // signal the recv to end
    sem_post(close_sem);
    // free the memory
    // delete(close_sem);
    delete(close_queue);
    sem_map.erase(sock);
    queue_map.erase(sock);
    pthread_mutex_unlock(&map_lock);

    if (passive == 0)
    {
        // pthread_mutex_lock(&send_lock);
        int ret = 0;
        char content[BUFFER_SIZE];
        sprintf(content, "close,%d,", sock);
        if (debug) printf("send close command\n");
        ret = mfsend(&handle, content, sizeof(content), dst_GUID, 0);
        if(ret < 0)
        {
            printf ("mfsendmsg error\n");
            // pthread_mutex_unlock(&send_lock);
            return -1;
        }
        if (debug) printf("after send close command\n");
        // pthread_mutex_unlock(&send_lock);        
    }

    return 0;
}
