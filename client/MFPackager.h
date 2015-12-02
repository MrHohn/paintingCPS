/*************************************************
#
# Purpose: header for "MFPackager Class"
# Author.: Zihong Zheng (zzhonzi@gmail.com)
# Version: 0.1
# License: 
#
*************************************************/

#ifndef MFPACKAGER_H
#define MFPACKAGER_H

#include <mfapi.h>

using namespace std;

class MFPackager {
public:
    MFPackager(int src_GUID, int dst_GUID, int debug);
    ~MFPackager();
    int getMyGUID();
    int sendImage(char* buf, int buf_size);
    int recvImage(char* buf, int buf_size);
    int sendResult(char* buf, int buf_size);
    int recvResult(char* buf, int buf_size);

private:
    int src_GUID;
    int dst_GUID;
    int debug = 0;
    struct Handle handle;
};

#endif /* MFPACKAGER_H */
