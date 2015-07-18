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
        boolean debug = true;
        boolean globalStop = false;
        Lock sendLock;
        MsgDistributor msgD;
        // HashSet<Integer> idSet;

    public JClient () {
        sendLock = new ReentrantLock();
        msgD = new MsgDistributor();
        // idSet = new HashSet<Integer>();
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
            System.out.println("now wait for new result from server");
        }
    }

    class TransmitChild implements Runnable {
        private int sock;
        private String fileName;

        public TransmitChild (int sock, String fileName) {
            this.sock = sock;
            this.fileName = fileName;
        }

        public void run() {
            System.out.println("send one image");
            sendLock.lock();
            sendLock.unlock();
        }
    }

    class TransmitThread implements Runnable {
        public void run() {
            System.out.println("transmit thread begin!");

            while (!globalStop) {

                if (debug) System.out.println("create a transmit child thread");
                (new Thread(new TransmitChild(1, "./pics/default-1.jpg"))).start();

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
                if (debug) System.out.println("now listen on GUID: ?");
                msgD.listen();
            }
        }
    }

    public void startClient() {

        System.out.println(Thread.currentThread().getName() + "thread begin!");

        // initialize the Message Distributor
        msgD.init(101, 102, true);
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
        JClient newTest = new JClient();
        // newTest.startClient();

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
    }
}
