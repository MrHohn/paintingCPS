package edu.rutgers.winlab.mfapi.mfping;

import java.util.concurrent.Semaphore;

import android.os.AsyncTask;

public class PingThread extends AsyncTask<Integer, String, Integer>{
	PingActivity pa;
	PingLogic pg;
	boolean client;
	double TO;
	Integer maxCycles;
	boolean cont;
	private final Semaphore sem;

	
	PingThread(double TO, String mine, String other, boolean client, PingActivity pa){
		this.pa = pa;
		this.TO = TO;
		pg = new PingLogic();
		this.client = client;
		pg.setClient(client);
		pg.setMine(mine);
		pg.setOther(other);
		cont = true;
		sem = new Semaphore(1);
		System.out.println("Semaphore initialized with " + sem.availablePermits() + " permits");
	}
	
	@Override
	protected Integer doInBackground(Integer... params) {
		maxCycles = params[0];
		pg.init();
		pg.startReceiving();
		System.out.println("Started receiving");
		try {
			if(client){
				System.out.println("Acquiring semaphore ");
				sem.acquire();
				System.out.println("Acquired semaphore ");
				for(int i=0; i<maxCycles && cont; i++){
					sem.release();
					System.out.println("Released semaphore ");
					pg.sendPing();
					System.out.println("Acquiring semaphore ");
					Thread.sleep(10000);
					sem.acquire();
					System.out.println("Acquired semaphore ");
				}
				sem.release();
			}
			else { System.out.println("Not a client");}
		} catch (InterruptedException e) {
			e.printStackTrace();
			return Integer.valueOf(-1);
		}
		try {
			System.out.println("Going to sleep");
			Thread.sleep(1000000000);
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		return Integer.valueOf(1);
	}
	
	@Override
	protected void onProgressUpdate(String... values) {
		pa.updateUI(values[0]);
	}

	@Override
	protected void onPostExecute(Integer i){
		System.out.println("Finished the execution, I'm in the UI thread "+i);
	}
	
	public void publish(String ping){
		publishProgress(ping);
	}
	
	public void stop(){
		try {
			System.out.println("Called stop from outside");
			sem.acquire();
			System.out.println("Setting bool to false");
			cont = false;
			sem.release();
			pg.stopReceiving();
			System.out.println("Stopped receiving");
			pg.clean();
			System.out.println("Cleaned up");
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
	}
	
}
