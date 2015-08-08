/*************************************************
#
# Purpose: main program for "Java CPS client"
# Author.: Zihong Zheng (zzhonzi@gmail.com)
# Version: 0.1
# License: 
#
*************************************************/

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import java.util.concurrent.Semaphore;
import java.util.HashSet;
import java.util.Queue;
import java.util.LinkedList;
import java.io.*;

public class JClient {
    private
        boolean debug;
        boolean globalStop = false;
        // Lock sendLock;
        MsgDistributor msgD;
        int srcGUID;
        int dstGUID;
        int BUFFER_SIZE = 1024;
        String userID = "java";
        // HashSet<Integer> idSet;
        Semaphore resultSem;
        Semaphore sendStartSem;
        Semaphore sendEndSem;
        Queue<String> resultQueue;
        byte[] img = null;
        int imgSize = 0;

    // srcGUID = the GUID of Android phone, usually should be 101
    // dstGUID = the GUID of server, usually should be 102
    // debug = whether to run in debug mode which would print out more details.
    public JClient (int src, int dst, boolean mode) {
        // sendLock = new ReentrantLock();
        resultSem = new Semaphore(0);
        sendStartSem = new Semaphore(0);
        sendEndSem = new Semaphore(0);
        resultQueue = new LinkedList<String>();
        msgD = new MsgDistributor();
        // idSet = new HashSet<Integer>();
        srcGUID = src;
        dstGUID = dst;
        debug = mode;
    }

    public static void usage(){
        System.out.println("Usage:");
        System.out.println("JCLient <dst_GUID> <src_GUID>");
        System.out.println("  compile: javac *.java -cp jmfapi-1.0-SNAPSHOT.jar");
        System.out.println("  run: sudo java -cp .:jmfapi-1.0-SNAPSHOT.jar JClient 101 102");
        System.out.println("Or using makefile:");
        System.out.println("  compile: make");
        System.out.println("  run: make run");
    }
 
    class ResultThread implements Runnable {
        public void run() {
            if (debug) System.out.println("result thread begin!");
            int sockID, ret = 0;
            byte[] buf = new byte[BUFFER_SIZE];
            String response;

            String header = "result,";
            header += userID;
            try {
                byte[] temp = header.getBytes("UTF-8");            
                for (int i = 0; i < temp.length; ++i) {
                    buf[i] = temp[i];
                }
            }
            catch (Exception e) {
                e.printStackTrace();
            }

            // try to connect the server
            sockID = msgD.connect();
            if (debug) System.out.printf("[jclient] result thread get connection to server\n");
            if (debug) System.out.printf("[jclient] start receiving the result\n");
            // send the header first
            ret = msgD.send(sockID, buf, BUFFER_SIZE);
            if (ret < 0) {
                System.out.println("mfsend error");
                System.exit(1);
            }

            // get the response
            buf = new byte[BUFFER_SIZE];
            ret = msgD.recv(sockID, buf, BUFFER_SIZE);
            if (ret < 0) {
                System.out.println("mfrecv error");
                System.exit(1);
            }
            response = new String(buf);
            response = response.trim();
            if (response.equals("failed")) {
                System.out.println("log in failed");
                System.exit(1);
            }

            while (!globalStop) {
                buf = new byte[BUFFER_SIZE];
                if (debug) System.out.println("still waiting here for a new result");
                ret = msgD.recv(sockID, buf, BUFFER_SIZE);
                if (ret < 0) {
                    System.out.println("mfrecv error");
                    System.exit(1);
                }
                else {
                    response = new String(buf);
                    response = response.trim();
                    String[] tokens = response.split("[,]");
                    if (tokens[0].equals("none")) {
                        resultQueue.offer(new String("none"));
                    }
                    else {
                        if (debug) System.out.println("\nreceived result: " + tokens[0]  + tokens[1] + tokens[2] + "\n");
                        String result = tokens[0] + "," + tokens[1] + "," + tokens[2];
                        resultQueue.offer(new String(result));
                    }

                    // signal the getResult call
                    resultSem.release();
                }
            }
        }
    }

    class TransmitThread implements Runnable {
        public void run() {
            if (debug) System.out.println("transmit thread begin!");
            int sockID;
            int ret = 0;
            String fileName = "./pics/orbit-sample.jpg";
            int sendSize = BUFFER_SIZE * 4;
            byte[] buf = new byte[sendSize];
            String response;

            String header = "transmit,";
            header += userID;
            try {
                byte[] temp = header.getBytes("UTF-8");            
                for (int i = 0; i < temp.length; ++i) {
                    buf[i] = temp[i];
                }
            }
            catch (Exception e) {
                e.printStackTrace();
            }

            // try to connect the server
            sockID = msgD.connect();
            if (debug) System.out.printf("[jclient] transmit thread get connection to server\n");
            if (debug) System.out.printf("[jclient] start transmitting current frame\n\n");
            // send the header first
            ret = msgD.send(sockID, buf, 100);
            if (ret < 0) {
                System.out.println("mfsend error");
                System.exit(1);
            }

            // get the response
            buf = new byte[sendSize];
            ret = msgD.recv(sockID, buf, 10);
            if (ret < 0) {
                System.out.println("mfrecv error");
                System.exit(1);
            }
            response = new String(buf);
            response = response.trim();
            if (response.equals("failed")) {
                System.out.println("log in failed");
                System.exit(1);
            }

            if (debug) System.out.printf("one time size: %d\n", sendSize);

            while (!globalStop) {
                try {
                    sendStartSem.acquire();                
                } catch (Exception e) {
                    e.printStackTrace();
                    break;
                }
                if (globalStop) {
                    break;
                }

                try {
                    if (debug) System.out.println("send one image");
                    buf = new byte[sendSize];

                    // send the file info, combine with ','
                    if (debug) System.out.println("[client] file name: " + fileName);
                    String content = String.format(fileName + ",%d", imgSize);
                    try {
                        byte[] temp = content.getBytes("UTF-8");            
                        for (int i = 0; i < temp.length; ++i) {
                            buf[i] = temp[i];
                        }
                    }
                    catch (Exception e) {
                        e.printStackTrace();
                    }
                    ret = msgD.send(sockID, buf, 100);
                    if (ret < 0) {
                        System.out.println("mfsend error");
                        System.exit(1);
                    }

                    // get the response
                    buf = new byte[sendSize];
                    ret = msgD.recv(sockID, buf, 10);
                    if (ret < 0) {
                        System.out.println("mfrecv error");
                        System.exit(1);
                    }
                    
                    int length;
                    int index = 0;
                    int remain = imgSize;
                    while (true) {
                        if (remain >= sendSize) {
                            System.arraycopy(img, index, buf, 0, sendSize);
                            msgD.send(sockID, buf, sendSize);
                            remain -= sendSize;
                            index += sendSize;
                            buf = new byte[sendSize];
                        }
                        else {
                            System.arraycopy(img, index, buf, 0, remain);
                            msgD.send(sockID, buf, remain);
                            break;
                        }

                    }

                }
                catch (Exception e) {
                    e.printStackTrace();
                    System.exit(1);
                }

                // signal the send trigger function to return
                sendEndSem.release();

            }

            System.out.println("tramsmit thread end");

        }
    }

    class MFListenThread implements Runnable {
        public void run() {
            if (debug) System.out.println("mf listen thread begin!");

            while (!globalStop) {
                if (debug) System.out.println("now listen on GUID: " + srcGUID);
                msgD.listen();
            }
        }
    }

    // start() method is used to create connection to server using MFAPI, make sure you've call it before you send out an image or request a result.
    // And notice that, this program would return once it finished set up the connection.
    public void start() {
        // initialize the Message Distributor
        msgD.init(srcGUID, dstGUID, debug);

        ExecutorService pool = Executors.newCachedThreadPool();
        TransmitThread transmitT = new TransmitThread();
        ResultThread resultT = new ResultThread();
        MFListenThread listenT = new MFListenThread();
        pool.execute(transmitT);
        pool.execute(resultT);
        pool.execute(listenT);

        pool.shutdown();
    }

    private void addShutdownHook() {  
        Runtime.getRuntime().addShutdownHook(new Thread() {
            public void run() {
                try {
                    globalStop = true;
                    System.out.println("\nSet up the stop signal to all threads.");
                    Thread.sleep(1000);
                    System.out.println("Now send the close command to server and close the mf handler");
                    msgD.end();
                    System.out.println("JClient end.");
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        });
    }

    // stop() method is used to close the connection to server and also close the MFAPI interface
    // This part might be still buggy, but we should have it.
    public int stop() {
        globalStop = true;
        System.out.println("\nSet up the stop signal to all threads.");
        sendStartSem.release();
        System.out.println("Now send the close command to server and close the mf handler");
        msgD.end();
        System.out.println("JClient end.");

        return 0;
    }

    // sendImage(byte[] , int ) method is used to trigger an action to send out your image to server.
    // assume the image is stored in an byte array as below: 
    // byte[] imgArray;
    // Also, it would return once the image is sent out
    public int sendImage(byte[] img, int size) {
        try {
            this.img = img;
            imgSize = size;
            // now signal to send the image
            sendStartSem.release();
            // wait for action complete
            sendEndSem.acquire();
        } catch (Exception e) {
            e.printStackTrace();
            return -1;
        }

        return 0;
    }

    // getResult() method would return you a result sent by server
    // if the image does not match any paintings in our database, the result would be "none"
    // otherwise, it would be like "author,title,date"
    // last, this method would keep waiting if there is no result from the server
    // it would return immediately once the server send you back a result("none" or information)
    public String getResult() {
        try {
            // now wait for a result
            if (debug) System.out.println("now wait for a result");
            resultSem.acquire();
            
            // get a result, take it out of the queue and return
            String result = resultQueue.poll();

            return result;

        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }
}
