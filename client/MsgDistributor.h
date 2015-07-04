/*************************************************
#
# Purpose: header for "MsgDistributor Class"
# Author.: Zihong Zheng
# Version: 0.1
# License: 
#
*************************************************/

#ifndef MSGDISTRIBUTOR_H
#define MSGDISTRIBUTOR_H

#include <sys/types.h> 
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unordered_map>
#include <queue>
#include <mfapi.h>

using namespace std;

class MsgDistributor
{
public:
    MsgDistributor(int src_GUID);
    ~MsgDistributor();
    // init a new mf socket and return the new created mf socket id
    int MsgConnect();
    int MsgAccept();
    //  send and receive the message according to the socket id
    int MsgSend(int sock, char* buffer, int size);
    int MsgRecv(int sock, char* buffer, int size); 
    // close the message channel
    int MsgClose(int sock);

private:
    int src_GUID;
    int mfsockid;
    struct Handle handle;
    // lock for socked id
    pthread_mutex_t id_lock;
    // map from mf socket id to semaphore 
    unordered_map<int, sem_t*> sem_map; 
    // mutex lock for sem_map operation
    pthread_mutex_t sem_map_lock; 
    // map from mf socket if to  message queue 
    unordered_map<int, queue<string>*> queue_map; 
    // mutex lock for queue_map operation
    pthread_mutex_t queue_map_lock; 
};

#endif /* MSGDISTRIBUTOR_H */
