/*************************************************
#
# Purpose: This program is used to monitor the Storm cluster.
			It will record the number of prepares, spouts, finished 
			requests, and the current parallel bolts.
# Author.: Zihong Zheng (zzhonzi@gmail.com)
# Version: 1.0
# License: 
#
*************************************************/

import java.io.*;
import java.net.*;

class ClusterMonitor {
	public static void main(String args[]) throws Exception {
		int port = 9876;
		DatagramSocket serverSocket = new DatagramSocket(port);
		System.out.println("Now wait for the responser...");
		long currentTime;
		long first = 0;
		long time;
		int result = 0;
		int parallel = 0;
		int prepare = 0;
		int spout = 0;

        while(true) {
			byte[] receiveData = new byte[1024];
			DatagramPacket receivePacket = new DatagramPacket(receiveData, receiveData.length);
			serverSocket.receive(receivePacket);
			String sentence = new String(receivePacket.getData());
			sentence = sentence.trim();
			if (sentence.equals("start")) {
				++parallel;
			}
			else if (sentence.equals("prepare")) {
				++prepare;
			}
			else if (sentence.equals("spout")) {
				++spout;
			}
			else {
				++result;
				--parallel;
			}
			System.out.println("[Message]: " + sentence + " [prepare]: " + prepare + " [spout]: " + spout + " [result]: " + result + " [Parallel]: " + parallel);
			currentTime = System.currentTimeMillis();
			if (first == 0) {
				first = currentTime;
			}
			time = (currentTime - first);

			System.out.println("Current time consumption: " + time + " ms");
		}
	}
}
