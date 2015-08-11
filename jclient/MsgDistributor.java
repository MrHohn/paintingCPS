/*************************************************
#
# Purpose: "Message Distributor" aims to distribute the message
            received through the unique GUID
# Author.: Zihong Zheng (zzhonzi@gmail.com)
# Version: 0.1
# License: 
#
*************************************************/

import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import java.util.concurrent.Semaphore;
import java.util.Queue;
import java.util.HashMap; 
import java.util.HashSet; 
import java.util.LinkedList;
import edu.rutgers.winlab.jmfapi.*;

public class MsgDistributor {
	private
		boolean debug = true;
		int BUFFER_SIZE = 1024;
		int MAX_CHUNK = 50000;
		int mfsockid;
		Lock sendLock;
		Lock recvLock;
		Lock idLock;
		Lock mapLock;
		Semaphore connectSem;
		Semaphore acceptSem;
		Queue<Integer> connectQueue;
		HashMap<Integer, Semaphore> semMap;
		HashMap<Integer, Queue<byte[]>> queueMap;
		HashMap<Integer, Integer> statusMap;
		GUID srcGUID;
		GUID dstGUID;
		String scheme = "basic";
		JMFAPI handler;

	public MsgDistributor() {
		mfsockid = -1;
    }

    public int init(int srcGUID, int dstGUID, boolean debug) {
    	if (mfsockid != -1) {
    		System.out.println("ERROR: Dont reinit MsgDistributor!");
    		return -1;
    	}

    	mfsockid = 0;
    	this.debug = debug;

    	sendLock = new ReentrantLock();
    	recvLock = new ReentrantLock();
    	idLock = new ReentrantLock();
    	mapLock = new ReentrantLock();
    	connectSem = new Semaphore(0);
    	acceptSem = new Semaphore(0);
    	connectQueue = new LinkedList<Integer>();
    	semMap = new HashMap<Integer, Semaphore>();
    	queueMap = new HashMap<Integer, Queue<byte[]>>();
    	statusMap = new HashMap<Integer, Integer>();

    	// init the mfapi
    	if (debug) System.out.println("Start to initialize the mf.");
		this.srcGUID = new GUID(srcGUID);
		this.dstGUID = new GUID(dstGUID);
		handler = new JMFAPI();
		try {
			handler.jmfopen(scheme, this.srcGUID);		
    		if (debug) System.out.println("Finished.");
		}
		catch (JMFException e) {
			mfsockid = -1;
			System.out.println(e.toString());
			return -1;
		}

    	return 0;
    }

    public int listen() {
    	if (mfsockid == -1) {
    		System.out.println("ERROR: Init MsgDistributor first!");
    		return -1;
    	}

    	byte[] buf = new byte[MAX_CHUNK];
		int ret;

		try {
			ret = handler.jmfrecv_blk(null, buf, MAX_CHUNK);
			if(ret < 0)
		    {
		        System.out.printf("mfrec error\n");
		        return -1;
		    }
		    if (debug) System.out.println("ret: " + ret);
			String bufString = new String(buf);
			String delims = "[,]";
			String[] tokens = bufString.split(delims);
			if (debug) System.out.println("receive new message, header: " + tokens[0]);
			if (tokens[0].equals("create")) {
		    	acceptSem.release();
			}
			else if (tokens[0].equals("accepted")) {
				int createdID = Integer.parseInt(tokens[1]);
				connectQueue.offer(createdID);
		    	connectSem.release();
			}
			else if (tokens[0].equals("sock")) {
				int sockID = Integer.parseInt(tokens[1]);
		        if (!semMap.containsKey(sockID)) {
    	            System.out.printf("ERROR: Socket ID not exist\n");
		        	return -1;
		        }
				int idLen = 1, divisor = 10;
				while (sockID / divisor != 0)
		        {
		            divisor *= 10;
		            ++idLen;
		        }
		        int contentStart = idLen + 6;
		        int contentLen = ret - idLen - 6;

		        // copy the message content
		        byte[] content = new byte[contentLen];
		        for (int i = contentStart; i < ret; ++i) {
		        	content[i - contentStart] = buf[i];
		        }

		        Queue<byte[]> idQueue = queueMap.get(sockID);
		        idQueue.offer(content);
		        Semaphore idSem = semMap.get(sockID);
		        idSem.release();
			}
			else if (tokens[0].equals("close")) {
				int sockID = Integer.parseInt(tokens[1]);
				if (debug) System.out.println("peer closed the connection");
				this.close(sockID, 1);
			}
			else {
		        System.out.printf("unable to classify this message, discard\n");
		        return -1;
			}
		}
		catch (JMFException e) {
			System.out.println(e.toString());
			return -1;
		}
    	
    	return 0;
    }

    // init a new mf socket and return the new created mf socket id, intiative side
    public int connect() {
    	if (mfsockid == -1) {
    		System.out.println("ERROR: Init MsgDistributor first!");
    		return -1;
    	}
    	try {    		
	    	int ret = 0;
	    	String headerString = String.format("create");
	    	byte[] header = headerString.getBytes("UTF-8");
	    	byte[] headerSend = new byte[BUFFER_SIZE];
	    	for (int i = 0; i < header.length; ++i) {
	    		headerSend[i] = header[i];
	    	}
	    	// send the connect request
	    	sendLock.lock();
	    	ret = handler.jmfsend(headerSend, BUFFER_SIZE, dstGUID);
	    	if(ret < 0)
		    {
		        System.out.printf ("mfsendmsg error\n");
		        sendLock.unlock();
		        return -1;
		    }
	    	sendLock.unlock();

	    	// wait for the response
	    	if (debug) System.out.println("wait for the connect response");
	    	idLock.lock();
	    	connectSem.acquire();

	    	if (debug) System.out.println("got response");
	    	int newID = connectQueue.poll();
	    	if (debug) System.out.println("accepted id: " + newID);

	    	// create new queue and semaphore for the new conncection
	    	Semaphore newConnectSem = new Semaphore(0);
	    	Queue<byte[]> newConnectQueue = new LinkedList<byte[]>();
	    	mapLock.lock();
	    	semMap.put(newID, newConnectSem);
	    	queueMap.put(newID, newConnectQueue);
	    	statusMap.put(newID, 1);
	    	if (debug) System.out.println("create and put references of queue, semaphore and status into maps");
	    	mapLock.unlock();
	    	idLock.unlock();
	    	return newID;
    	}
    	catch (Exception e) {
			e.printStackTrace();
			return -1;
        }
    }

    // accept a new connection, return the socket id
    public int accept() {
    	if (mfsockid == -1) {
    		System.out.println("ERROR: Init MsgDistributor first!");
    		return -1;
    	}

    	try {
	    	acceptSem.acquire();
	    	if (debug) System.out.println("new connection needed to accept");
	    	idLock.lock();
    	    // got a new connection, need to accpet, create a new id for it
    	    ++mfsockid;
		    while (semMap.containsKey(mfsockid))
		    {
		        ++mfsockid;
		    }
		    int newID = mfsockid;
	    	idLock.unlock();
	    	if (debug) System.out.println("create new id: " + newID);
	    	int ret = 0;
	    	String headerString = String.format("accepted,%d", newID);
	    	byte[] header = headerString.getBytes("UTF-8");
	    	byte[] headerSend = new byte[BUFFER_SIZE];
	    	for (int i = 0; i < header.length; ++i) {
	    		headerSend[i] = header[i];
	    	}
	    	sendLock.lock();
	    	ret = handler.jmfsend(headerSend, BUFFER_SIZE, dstGUID);
	    	if(ret < 0)
		    {
		        System.out.printf ("mfsendmsg error\n");
		        sendLock.unlock();
		        return -1;
		    }
	    	sendLock.unlock();

	    	// create new queue and semaphore for the new conncection
	    	Semaphore newConnectSem = new Semaphore(0);
	    	Queue<byte[]> newConnectQueue = new LinkedList<byte[]>();
	    	mapLock.lock();
	    	semMap.put(newID, newConnectSem);
	    	queueMap.put(newID, newConnectQueue);
	    	statusMap.put(newID, 1);
	    	if (debug) System.out.println("create and put addrs of queue, semaphore and status into maps");
	    	mapLock.unlock();
	    	return newID;
    	} 
    	catch (Exception e) {
			e.printStackTrace();
			return -1;
        }
	}

	public int send(int sockID, byte[] buf, int size) {
		if (mfsockid == -1) {
    		System.out.println("ERROR: Init MsgDistributor first!");
    		return -1;
    	}
    	if (!semMap.containsKey(sockID)) {
    		System.out.println("ERROR: Socket ID not exist");
    		return -1;
    	}

		int ret = 0;
    	try {
	    	int idLen = 1, divisor = 10;
	    	while (sockID / divisor != 0) {
	    		divisor *= 10;
	    		++idLen;
	    	}
	    	// int contentLen = BUFFER_SIZE - idLen - 6;
	    	int contentLen = size + idLen + 6;
	    	String headerString = String.format("sock,%d,", sockID);
	    	byte[] header = headerString.getBytes("UTF-8");
	    	byte[] content = new byte[contentLen];
	    	// copy the header
	    	for (int i = 0; i < header.length; ++i) {
	    		content[i] = header[i];
	    	}
	    	// copy the content
	    	for (int i = 0; i < size; ++i) {
	    		content[i + 6 + idLen] = buf[i];
	    	}
	    	// sendLock.lock();
	    	if (debug) System.out.printf("now send message in socket: %d\n", sockID);
		    ret = handler.jmfsend(content, contentLen, dstGUID);
		    if(ret < 0)
		    {
		        System.out.printf ("mfsendmsg error\n");
			    // sendLock.unlock();
		        return -1;
		    }
		    if (debug) System.out.printf("finish, ret: %d\n", ret);
		    // sendLock.unlock();
    	}
    	catch (Exception e) {
    		e.printStackTrace();
    		return -1;
    	}

    	return ret;
	}

	public int recv(int sockID, byte[] buf, int size) {
    	if (mfsockid == -1) {
    		System.out.println("ERROR: Init MsgDistributor first!");
    		return -1;
    	}
	    // check if the sock id is valid
    	if (!semMap.containsKey(sockID)) {
    		System.out.println("ERROR: Socket ID not exist");
    		return -1;
    	}

    	try {
		    // wait for buffer filled
	    	Semaphore recvSem = semMap.get(sockID);
	    	if (debug) System.out.printf("now wait for new message: %d\n", sockID);
    		recvSem.acquire();
		    // check if the connection is closed
	    	if (statusMap.get(sockID) == 0) {
		    	if (debug) System.out.printf("sockid[%d]: connection is closed\n", sockID);
		    	// remove the sock id from status map
		    	mapLock.lock();
		    	statusMap.remove(sockID);
		    	mapLock.unlock();
	    		return -1;
	    	}

	    	if (debug) System.out.printf("new message arrived: %d\n", sockID);
	    	Queue<byte[]> recvQueue = queueMap.get(sockID);
	    	byte[] recvByte = recvQueue.poll();
	    	int getSize = Math.min(recvByte.length, size);
	    	for (int i = 0; i < getSize; ++i) {
	    		buf[i] = recvByte[i];
	    	}
    	}
    	catch (Exception e) {
    		e.printStackTrace();
    	}

    	return 0;
	}

	public int close(int sockID, int mode) {
		if (mfsockid == -1) {
    		System.out.println("ERROR: Init MsgDistributor first!");
    		return -1;
    	}
    	if (!semMap.containsKey(sockID)) {
    		System.out.println("ERROR: Socket ID not exist");
    		return -1;
    	}

    	if (debug) System.out.println("now close the socket: " + sockID);
    	Semaphore closeSem = semMap.get(sockID);
    	// Queue<char[]> closeQueue = queueMap.get(sockID);
    	mapLock.lock();
    	// change the status from 1 to 0, means closed
    	statusMap.put(sockID, 0);
    	// signal the recv to end
    	closeSem.release();
    	semMap.remove(sockID);
    	queueMap.remove(sockID);
    	mapLock.unlock();

    	if (mode == 0) {
    		// if not in passive mode, then notify the peer to close the connection
    		try {
	    		int ret = 0;
	    		String headerString = String.format("close,%d", sockID);
		    	byte[] header = headerString.getBytes("UTF-8");
		    	byte[] headerSend = new byte[BUFFER_SIZE];
		    	for (int i = 0; i < header.length; ++i) {
		    		headerSend[i] = header[i];
		    	}
		    	// sendLock.lock();
		    	if (debug) System.out.println("send close command");
		    	ret = handler.jmfsend(headerSend, BUFFER_SIZE, dstGUID);
		    	if(ret < 0)
			    {
			        System.out.printf ("mfsendmsg error\n");
			        // sendLock.unlock();
			        return -1;
			    }
		    	if (debug) System.out.println("after send close command");
		    	// sendLock.unlock();
    		}
    		catch (Exception e) {
				e.printStackTrace();
				return -1;
        	}
    	}

    	return 0;
    }

    //TODO: something wrong when call jmfclose(), needed to fix
	public void end() {
		HashSet<Integer> sockSet = new HashSet<Integer>();
		for (Integer i : statusMap.keySet()) {
			sockSet.add(i);
		}
		for (Integer i : sockSet) {
			// close all remain sock ids
			System.out.println("Now close the sock id: " + i);
			this.close(i, 0);
		}

		// try {
		// 	handler.jmfclose();		
		// } catch (JMFException e) {
		// 	System.out.println(e.toString());
		// }
    }
}
