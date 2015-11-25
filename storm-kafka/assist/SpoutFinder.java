/*************************************************
#
# Purpose: This program is used to mark down the address of spout.
			When CPS server ask for the address, just tell it.
# Author.: Zihong Zheng (zzhonzi@gmail.com)
# Version: 1.0
# License: 
#
*************************************************/

package assist;

import java.io.*;
import java.net.*;

class SpoutFinder {
	public static void main(String args[]) throws Exception {
		int spoutFinderPort = 9877;
		int serverPort = 9879;
		DatagramSocket serverSocket = new DatagramSocket(spoutFinderPort);
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
				System.out.println("Found new spout.");
				InetAddress IPAddress = receivePacket.getAddress();
				spoutIP = IPAddress.getHostAddress();
				System.out.println("Spout IP: " + spoutIP);
				System.out.println("Wait for CPS server to ask.");
			}
			// message from CPS server
			if (sentence.equals("where")) {
				System.out.println("CPS server querying.");
				// send back the ip of spout
				DatagramSocket clientSocket = new DatagramSocket();
				InetAddress serverIP = receivePacket.getAddress();
				byte[] sendData = new byte[1024];
				sendData = spoutIP.getBytes();
				DatagramPacket sendPacket = new DatagramPacket(sendData, sendData.length, serverIP, serverPort);
				System.out.println("send back the spout IP: " + spoutIP);
				clientSocket.send(sendPacket);
			}
		}
	}
}
