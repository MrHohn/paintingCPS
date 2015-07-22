/*************************************************
#
# Purpose: main program for "Java CPS client"
# Author.: Zihong Zheng
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

public class JClient {
    private
        boolean debug;
        boolean globalStop = false;
        Lock sendLock;
        MsgDistributor msgD;
        int srcGUID;
        int dstGUID;
        int BUFFER_SIZE = 1024;
        String userID = "java";
        // HashSet<Integer> idSet;

    public JClient (int src, int dst, boolean mode) {
        sendLock = new ReentrantLock();
        msgD = new MsgDistributor();
        // idSet = new HashSet<Integer>();
        srcGUID = src;
        dstGUID = dst;
        debug = mode;
    }

    private static void usage(){
        System.out.println("Usage:");
        System.out.println("JCLient <dst_GUID> <src_GUID>");
        System.out.println("compile: javac *.java -cp jmfapi-1.0-SNAPSHOT.jar");
        System.out.println("run: java -cp .:jmfapi-1.0-SNAPSHOT.jar JClient 101 102");
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
                    if (response.equals("none")) {
                        // do nothing
                    }
                    else {
                        System.out.println("received result: " + response);
                    }
                }
            }
        }
    }

    class TransmitThread implements Runnable {
        public void run() {
            System.out.println("transmit thread begin!");

            while (!globalStop) {
                String fileName = "./pics/orbit-sample.jpg";
                if (debug) System.out.println("send one image");
                sendLock.lock();
                sendLock.unlock();

                // send one image per second
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }

            }
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

        JClient newTest = new JClient(src, dst, debug);
        // newTest.startClient();



//-----------below is for test------------

        // byte[] buf = new byte[1024];
        // for (int i = 0; i < 4; ++i) {
        //     buf[i] = (byte)0x21;
        // }
        // // buf[4] = (byte)0x00;
        // String bufString = new String(buf);
        // bufString = bufString.trim();
        // System.out.println(bufString);
        // if (bufString.equals("!!!!")) {
        //     System.out.println("equals");
        // }

        // String command = "accept,1,hello";
        // String delims = "[,]";
        // String[] tokens = command.split(delims);
        // for (String ele : tokens) {
        //     System.out.println(ele);
        // }
        // System.out.println(command);

        // byte[] command = {'a', 'b', 'c', 'd'};
        // int id = 10;
        // for (byte c : command) {
        //     // System.out.printf("%s\n", c);
        //     System.out.printf("%x\n", c);
        // }

        // String commandString = new String(command);
        // commandString = commandString.trim();
        // System.out.println(commandString);
        // if (commandString.equals("abcd")) {
        //     System.out.println("equals");
        // }

        // int newID = 10;
        // String headerString = String.format("accepted,%d", newID);
        // System.out.println(headerString);
        // try {
        //     byte[] header = headerString.getBytes("UTF-8");
        //     byte[] real = new byte[20];
        //     for (int i = 0; i < header.length; ++i) {
        //         real[i] = header[i];
        //     }
        //     for (byte c : real) {
        //         System.out.printf("%x\n", c);
        //     }
        // } catch (Exception e) {
        //     e.printStackTrace();
        // }
    }
}
