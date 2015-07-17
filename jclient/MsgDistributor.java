/*************************************************
#
# Purpose: "Message Distributor" aims to distribute the message
            received through the unique GUID
# Author.: Zihong Zheng
# Version: 0.1
# License: 
#
*************************************************/

import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import java.util.concurrent.Semaphore;
import java.util.Queue;
import java.util.HashMap; 

public class MsgDistributor {
	private
		boolean debug = true;
		int BUFFER_SIZE = 1024;
		int srcGUID;
		int dstGUID;
		int mfsockid;
        Lock sendLock;
        Lock recvLock;
        Lock idLock;
        Lock mapLock;
        Semaphore connetSem;
        Semaphore acceptSem;
        Queue<Integer> connectQueue;
        HashMap<Integer, Semaphore> semMap;
        HashMap<Integer, Queue<String>> queueMap;
        HashMap<Integer, Integer> statusMap;

	public MsgDistributor() {
		mfsockid = -1;
    }

    public int init(int srcGUID, int dstGUID, boolean debug) {
    	if (mfsockid != -1) {
    		System.out.println("ERROR: Dont reinit MsgDistributor!");
    		return -1;
    	}

    	mfsockid = 0;
    	this.srcGUID = srcGUID;
    	this.dstGUID = dstGUID;
    	this.debug = debug;

    	sendLock = new ReentrantLock();
    	recvLock = new ReentrantLock();
    	idLock = new ReentrantLock();
    	mapLock = new ReentrantLock();
    	connetSem = new Semaphore(0);
    	acceptSem = new Semaphore(0);

    	return 0;
    }

    public int listen() {
    	return 0;
    }

    public int connect() {
    	return 0;
    }

    public int accept() {
    	return 0;
    }

}