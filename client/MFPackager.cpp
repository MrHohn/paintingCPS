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
#include <string.h>
#include "MFPackager.h"

MFPackager::MFPackager(int src_GUID, int dst_GUID, int set_debug) {
    this->src_GUID = src_GUID;
    this->dst_GUID = dst_GUID;
    debug = set_debug;

    /* init mfapi here */
    int ret = 0;
    printf("------ open the MF now -------\n");
    ret = mfopen(&handle, "basic\0", 0, src_GUID);
    if (ret) {
        printf("mfopen error\n"); 
        exit(1);
    }
    printf("------ Success -------\n");
    /* finish init */
}

MFPackager::~MFPackager() {
    if (debug) printf("now delete the MFPackager class\n");

}

int MFPackager::getMyGUID() {
    return src_GUID;
}

int MFPackager::sendImage(char* buf, int buf_size) {
    char tmp[buf_size + 10];
    sprintf(tmp, "%d,", buf_size);
    memcpy(tmp + 10, buf, buf_size);

    int ret = mfsend(&handle, tmp, sizeof(tmp), dst_GUID, 0);
    if (ret < 0) {
        printf ("mfsend res error\n");
        return -1;
    }
    if (debug) printf("send ret: %d\n", ret);

    return ret;
}

// return the size of image
int MFPackager::recvImage(char* buf, int buf_size) {
    char tmp[buf_size];
    int ret = mfrecv_blk(&handle, NULL, tmp, buf_size, NULL, 0);
    if (ret < 0) {
        printf("mfrec res error\n"); 
        return -1;
    }
    if (debug) printf("recv ret: %d\n", ret);

    char* token;
    token = strtok(tmp, " ,");
    int img_size = atoi(token);
    memcpy(buf, tmp + 10, img_size);

    return img_size;
}

int MFPackager::sendResult(char* buf, int buf_size) {
    int ret = mfsend(&handle, buf, buf_size, dst_GUID, 0);
    if (ret < 0) {
        printf ("mfsend res error\n");
        return -1;
    }
    if (debug) printf("send ret: %d\n", ret);

    return ret;
}

int MFPackager::recvResult(char* buf, int buf_size) {
    int ret = mfrecv_blk(&handle, NULL, buf, buf_size, NULL, 0);
    if (ret < 0) {
        printf("mfrec res error\n"); 
        return -1;
    }
    if (debug) printf("recv ret: %d\n", ret);

    return ret;
}
