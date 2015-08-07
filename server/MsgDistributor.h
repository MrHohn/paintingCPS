/*************************************************
#
# Purpose: header for "MsgDistributor Class"
# Author.: Zihong Zheng (zzhonzi@gmail.com)
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
    int init(int src_GUID, int dst_GUID, int debug);
    int listen();
    int connect();
    int accept();
    int send(int sock, char* buffer, int size);
    int recv(int sock, char* buffer, int size);
    int close(int sock, int passive);

private:
    int src_GUID;
    int dst_GUID;
    int debug = 0;
    int stop = 0;
    int BUFFER_SIZE = 1024;
    int MAX_CHUNK = 50000;
    int mfsockid;
    struct Handle handle;
    pthread_mutex_t send_lock;          // lock for send
    pthread_mutex_t recv_lock;          // lock for receive
    pthread_mutex_t id_lock;            // lock for id operation
    pthread_mutex_t map_lock;           // mutex lock for map operation
    sem_t connect_sem;
    queue<int> connect_queue;
    sem_t accept_sem;
    unordered_map<int, sem_t*> sem_map; // map from mf socket id to semaphore 
    unordered_map<int, queue<char*>*> queue_map; // map from mf socket if to  message queue
    unordered_map<int, int> status_map; // map from mf socket id to status
};

#endif /* MSGDISTRIBUTOR_H */
