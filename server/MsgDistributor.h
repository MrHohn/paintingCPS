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
    MsgDistributor();
    ~MsgDistributor();
    int init(int src_GUID, int dst_GUID);
    int listen();
    int connect();
    int accept();
    int send(int sock, char* buffer, int size);
    string recv(int sock);
    int close(int sock);

private:
    int debug = 1;
    int stop = 0;
    int BUFFER_SIZE = 1024;
    int src_GUID;
    int dst_GUID;
    int mfsockid;
    struct Handle handle;
    pthread_mutex_t send_lock;          // lock for send
    pthread_mutex_t recv_lock;          // lock for receive
    pthread_mutex_t id_lock;            // lock for id operation
    sem_t connect_sem;
    queue<int> connect_queue;
    sem_t accept_sem;
    unordered_map<int, sem_t*> sem_map; // map from mf socket id to semaphore 
    pthread_mutex_t sem_map_lock;       // mutex lock for sem_map operation
    unordered_map<int, queue<string>*> queue_map; // map from mf socket if to  message queue 
    pthread_mutex_t queue_map_lock;     // mutex lock for queue_map operation
};

#endif /* MSGDISTRIBUTOR_H */
