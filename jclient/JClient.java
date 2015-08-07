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

    public JClient (int src, int dst, boolean mode) {
        // sendLock = new ReentrantLock();
        msgD = new MsgDistributor();
        // idSet = new HashSet<Integer>();
        srcGUID = src;
        dstGUID = dst;
        debug = mode;
    }

    private static void usage(){
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
            System.out.println("result thread begin!");
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
                        // do nothing
                    }
                    else {
                        System.out.println("received result: " + tokens[0]);
                    }
                }
            }
        }
    }

    class TransmitThread implements Runnable {
        public void run() {
            System.out.println("transmit thread begin!");
            int sockID;
            int ret = 0;
            String fileName = "./pics/orbit-sample.jpg";
            byte[] buf = new byte[BUFFER_SIZE];
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

            int divisor = 10;
            int idLen = 1;
            while (idLen / divisor > 0) {
                ++idLen;
                divisor *= 10;
            }
            int sendSize = BUFFER_SIZE - 6 - idLen;
            if (debug) System.out.printf("one time size: %d\n", sendSize);

            while (!globalStop) {
                try {
                    if (debug) System.out.println("send one image");
                    buf = new byte[BUFFER_SIZE];

                    File frame = new File(fileName);
                    FileInputStream readFile = new FileInputStream(fileName);                
                    long fileLen = frame.length();
                    int length;

                    // send the file info, combine with ','
                    System.out.println("[client] file name: " + fileName);
                    String content = String.format(fileName + ",%d", fileLen);
                    try {
                        byte[] temp = content.getBytes("UTF-8");            
                        for (int i = 0; i < temp.length; ++i) {
                            buf[i] = temp[i];
                        }
                    }
                    catch (Exception e) {
                        e.printStackTrace();
                    }
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

                    while ((length = readFile.read(buf, 0, sendSize)) > 0) {
                        msgD.send(sockID, buf, BUFFER_SIZE);
                    }
                }
                catch (Exception e) {
                    e.printStackTrace();
                    System.exit(1);
                }

                // send one image per 2 seconds
                try {
                    Thread.sleep(2000);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }

            System.out.println("tramsmit thread end");

        }
    }

    class MFListenThread implements Runnable {
        public void run() {
            System.out.println("mf listen thread begin!");

            while (!globalStop) {
                if (debug) System.out.println("now listen on GUID: " + srcGUID);
                msgD.listen();
            }
        }
    }

    public void startClient() {

        System.out.println(Thread.currentThread().getName() + "thread begin!");

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
        System.out.println(Thread.currentThread().getName() + "thread end!");
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

    public static void main(String[] args) {
        if(args.length < 2){
            usage();
            return;
        }
        int src =  Integer.parseInt(args[0]);
        int dst =  Integer.parseInt(args[1]);
        // System.out.println("srcGUID: " + src);
        // System.out.println("dstGUID: " + dst);
        boolean debug = false;
        // check if need to run in debug mode
        if (args.length > 2) {
            if (args[2].equals("d")) {
                debug = true;
            }
        }

        JClient client = new JClient(src, dst, debug);
        // add hookup to terminate the program gracefully
        client.addShutdownHook();
        // start the client
        client.startClient();
    }
}
