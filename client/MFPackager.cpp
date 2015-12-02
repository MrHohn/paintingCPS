/*************************************************
#
# Purpose: "MFPackager" aims to pack and unpack the mf message
# Author.: Zihong Zheng (zzhonzi@gmail.com)
# Version: 0.1
# License: 
#
*************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "MFPackager.h"

MFPackager::MFPackager(int src_GUID, int dst_GUID, int set_debug) {
    this->src_GUID = src_GUID;
    this->dst_GUID = dst_GUID;
    debug = set_debug;

    /* init mfapi here */
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
}

MFPackager::~MFPackager()
{
    if (debug) printf("now delete the MsgDistributor class\n");

}
