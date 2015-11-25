/*************************************************
#
# Purpose: Simple test program for ClusterMonitor
# Author.: Zihong Zheng (zzhonzi@gmail.com)
# Version: 1.0
# License: 
#
*************************************************/

import java.io.*;
import java.net.*;

class TimeResponser {
	public static void main(String args[]) throws Exception {
		BufferedReader inFromUser = new BufferedReader(new InputStreamReader(System.in));
		DatagramSocket clientSocket = new DatagramSocket();
		// InetAddress IPAddress = InetAddress.getByName("localhost");
		InetAddress IPAddress = InetAddress.getByName("10.0.0.200");
		byte[] sendData = new byte[1024];
		while (true) {
			String sentence = inFromUser.readLine();
			sendData = sentence.getBytes();
			DatagramPacket sendPacket = new DatagramPacket(sendData, sendData.length, IPAddress, 9876);
			// System.out.println("Now send a message: " + sentence);
			clientSocket.send(sendPacket);
		}
		// clientSocket.close();
	}
}
