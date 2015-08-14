/*************************************************
#
# Purpose: This program is used to mark down the address of spout.
			When CPS server ask for the address, just tell it.
# Author.: Zihong Zheng (zzhonzi@gmail.com)
# Version: 1.0
# License: 
#
*************************************************/

import java.io.*;
import java.net.*;

class SpoutFinder {
	public static void main(String args[]) throws Exception {
		int port = 9877;
		DatagramSocket serverSocket = new DatagramSocket(port);
		System.out.println("Now wait for the spout...");
		String spoutIP = "none";

        while(true) {
			byte[] receiveData = new byte[1024];
			DatagramPacket receivePacket = new DatagramPacket(receiveData, receiveData.length);
			serverSocket.receive(receivePacket);
			String sentence = new String(receivePacket.getData());
			sentence = sentence.trim();
			System.out.println("New message");
			// message from spout
			if (sentence.equals("here")) {
				System.out.println("Found spout.");
				InetAddress IPAddress = receivePacket.getAddress();
				spoutIP = IPAddress.getHostAddress();
				System.out.println("Spout IP: " + spoutIP);
				System.out.println("Wait for CPS server to ask.");
			}
			// message from CPS server
			if (sentence.equals("where")) {
				InetAddress IPAddress = receivePacket.getAddress();
				
			}
		}
	}
}
