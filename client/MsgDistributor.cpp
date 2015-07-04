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

MsgDistributor::MsgDistributor(int GUID)
{
	src_GUID = GUID;
	/* init mfapi here */
	int ret = 0;
	ret = mfopen(&handle, "basic\0", 0, src_GUID);
	if(ret) {
		printf("mfopen error\n"); 
		exit(1);
	}
	/* finish init */

	mfsockid = 0;

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


}

MsgDistributor::~MsgDistributor()
{
	// close mf here
	mfclose(&handle);
}
