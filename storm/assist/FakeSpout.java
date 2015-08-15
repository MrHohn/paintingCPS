import java.io.*;
import java.net.*;

public class FakeSpout {
    private static int num = 15;
    private static int index = 1;
    private static boolean found = false;
    private static int monitorPort = 9876;
    private static int spoutFinderPort = 9877;
    private static int spoutPort = 9878;
    private static boolean monitor = true;
    private static String monitorHost = "localhost";
    private static boolean testMode = false;
    private static boolean debug = true;

	public static void main(String args[]){

		try {
			DatagramSocket clientSocket = new DatagramSocket();
			InetAddress serverIP = InetAddress.getByName(monitorHost);
			//send the spout signal
			String buffer = "spout";
			byte[] sendData = new byte[1024];
			sendData = buffer.getBytes();
			DatagramPacket sendPacket = new DatagramPacket(sendData, sendData.length, serverIP, monitorPort);
			if (debug) System.out.println("send spout to monitor");
			if (monitor) clientSocket.send(sendPacket);
			if (!found) {
				// tell the SpoutFinder where the spout is
				buffer = "here";
				sendData = new byte[1024];
				sendData = buffer.getBytes();
				sendPacket = new DatagramPacket(sendData, sendData.length, serverIP, spoutFinderPort);
				clientSocket.send(sendPacket);
				found = true;
			}

			if (!testMode) {
				ServerSocket spoutServer = new ServerSocket(spoutPort);
				Socket serverSock;

				// wait for the CPS server to connect
				if (debug) System.out.println("wait for connection");
				serverSock = spoutServer.accept();
				if (debug) System.out.println("got connection");

				// initiallize the new accepted socket
				BufferedReader in = new BufferedReader(new InputStreamReader(serverSock.getInputStream()));
				DataInputStream receiveData = new DataInputStream(serverSock.getInputStream());
				PrintWriter out = new PrintWriter(new OutputStreamWriter(serverSock.getOutputStream()),true);

				// received the file size first
				String message;
				if (debug) System.out.println("now read from socket");
				message = in.readLine();
				if (debug) System.out.println("file size: " + message);
				int fileSize = Integer.parseInt(message);
				if (debug) System.out.println("now response");
				out.println("ok");
				if (debug) System.out.println("start receiving image");
				// byte[];

			}


		}
		catch (Exception e) {
			e.printStackTrace();
		}
	}
}