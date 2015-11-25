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

		while (true) {

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
					DataInputStream in = new DataInputStream(serverSock.getInputStream());
					PrintWriter out = new PrintWriter(new OutputStreamWriter(serverSock.getOutputStream()),true);

					// received the file size first
					byte[] receivedData = new byte[100];
					String message;
					int ret;
					if (debug) System.out.println("now read from socket");
					ret = in.read(receivedData);
					if (debug) System.out.println("ret: " + ret);
					if (debug) System.out.println("now response");
					out.println("ok");
					message = new String(receivedData, "UTF-8");
					message = message.trim();
					int fileSize = Integer.parseInt(message);
					if (debug) System.out.println("file size: " + fileSize);

					if (debug) System.out.println("start receiving image");
					byte[] img = new byte[fileSize];
					int offset = 0;
					int once = 2048;
					while (true) {
						if (fileSize - offset <= once) {
							ret = in.read(img, offset, fileSize - offset);
							if (debug) System.out.println("ret: " + ret);
							break;
						}
						else {
							ret = in.read(img, offset, once);
							if (debug) System.out.println("ret: " + ret);
						}
						offset += ret;
					}

					spoutServer.close();
				}


			}
			catch (Exception e) {
				e.printStackTrace();
			}
		}

	}
}