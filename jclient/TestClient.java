/*************************************************
#
# Purpose: Test program for "JClient"
# Author.: Zihong Zheng (zzhonzi@gmail.com)
# Version: 0.1
# License: 
#
*************************************************/

import java.io.*;

public class TestClient {
	public static void main(String[] args) {
        if(args.length < 2){
            JClient.usage();
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
        // client.addShutdownHook();
        // start the client
        client.start();

        try {
            // now read the img file
            String fileName = "./pics/orbit-sample.jpg";
            File frame = new File(fileName);
            FileInputStream readFile = new FileInputStream(fileName);                
            int size = (int)frame.length();
            byte[] img = new byte[size];
            int length = readFile.read(img, 0, size);
            if (length != size) {
                System.out.println("Read image error!");
                System.exit(1);
            }

            // below is for user input
            BufferedReader userEntry = new BufferedReader(new InputStreamReader(System.in));
            String enter;
            System.out.println("\nEnter \"send\" to send an image, \"quit\" to end the program");
            String result;

            while (true) {
                enter = userEntry.readLine(); // wait for user enter something

                if (enter.equals("send")) {
                    System.out.println("\nNow send an image.");
                    client.sendImage(img, size);
                    result = client. getResult();
                    System.out.println("\nresult from the server: " + result + "\n");
                }
                else if (enter.equals("quit")) {
                    break;
                }
                else {
                    System.out.println("Please enter send or quit.");
                }
            }
        } catch (IOException ex) {
            ex.printStackTrace();
            System.exit(1);
        }

        System.out.println("Test end.");
        client.stop();
        System.exit(1);
    }
}
