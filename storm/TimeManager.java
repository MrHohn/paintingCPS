import java.io.*;
import java.net.*;

class TimeManager {
	public static void main(String args[]) throws Exception {
		int port = 9876;
		DatagramSocket serverSocket = new DatagramSocket(9876);
		byte[] receiveData = new byte[1024];
		System.out.println("Now wait for the responser...");
		long currentTime;
		long first = 0;
		long time;
		int num = 1;

        while(true) {
        	DatagramPacket receivePacket = new DatagramPacket(receiveData, receiveData.length);
			serverSocket.receive(receivePacket);
			String sentence = new String( receivePacket.getData());
			System.out.println("Result ID: " + sentence + "Count: " + num);
			++num;
			currentTime = System.currentTimeMillis();
			if (first == 0) {
				first = currentTime;
			}
			time = (currentTime - first);

			System.out.println("Current time consumption: " + time);
		}
	}
}
