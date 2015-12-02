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

#include <string.h>
#include <mfapi.h>

using namespace std;

class MFPackager
{
public:
    MFPackager(int src_GUID, int dst_GUID, int debug);
    ~MFPackager();

private:
    int src_GUID;
    int dst_GUID;
    int debug = 0;
    struct Handle handle;
};

#endif /* MFPACKAGER_H */
