package edu.rutgers.winlab.mfapi.mfping;

import java.nio.ByteBuffer;

import edu.rutgers.winlab.jmfapi.JMFAPI;
import edu.rutgers.winlab.jmfapi.JMFAPI.Options;

public class PingLogic {
	String other;
	String mine;
	int seq_n;
	int maxPending;
	long pending[];
	boolean client;
	PingThread pt;
	JMFAPI socket;
	Thread rt;
	Options opt = new Options((byte)'0',(byte)'0',(byte)'0',(byte)'0');
	
	PingLogic(){
		//default values
		client = true;
		other = "0";
		seq_n = 0;
		socket = new JMFAPI();
		pending = new long[maxPending];
	}

	public String getOther() {
		return other;
	}

	public void setOther(String other) {
		this.other = other;
	}
	
	public String getMine() {
		return mine;
	}

	public void setMine(String mine) {
		this.mine = mine;
	}

	public boolean isClient() {
		return client;
	}

	public void setClient(boolean client) {
		this.client = client;
	}
	
	public void init(){
		if(socket.jmfopen(other, mine, opt)==0){
			System.out.println("ERROR in init");
		}
		else{
			System.out.println("Correctly initialized");
		}
	}
	
	public void sendPing(){
		byte buffer[] = new byte[64];
		buffer[0] = (byte)0;
		System.arraycopy(ByteBuffer.allocate(4).putInt(seq_n).array(),
				0, buffer, 1, 4);
		System.arraycopy(ByteBuffer.allocate(8).putLong(System.currentTimeMillis()).array(),
				0, buffer, 5, 8);
		socket.jmfsend(buffer, 64, opt);
	}
	
	public int startReceiving(){
		rt = new Thread(new RecThread(client, socket));
		rt.start();
		return 1;
	}
	
	public int stopReceiving(){
		rt.interrupt();
		try {
			rt.join();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		return 1;
	}
	
	public void clean(){
		socket.jmfclose();
		socket = null;
	}
	
	private class RecThread implements Runnable {
		boolean client;
		JMFAPI sock;
		Options o;
		byte buffer[];
		
		RecThread(boolean c, JMFAPI j) {
			client = c;
			sock = j;
			o = new Options((byte)'0',(byte)'0',(byte)'0',(byte)'0');
		}

		public void run() {
			int ret;
			buffer = new byte[128];
			while(true){
				ret = sock.jmfrecv_blk(buffer, 64, o);
				System.out.println("Received something " + ret);
				if(ret<=0){
					System.out.println("Nothing to receive " + ret);
				}
				else{
					if(client){
						
					}
					else {
						System.out.println("Received something of size " + ret);
						sock.jmfsend(buffer, ret, o);
						pt.publish("Received ping request at time: " + System.currentTimeMillis());
					}
				}
			}
		}
	}
}
