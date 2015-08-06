package edu.rutgers.winlab.mfstack.launcher;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import edu.rutgers.winlab.jmfapi.GUID;
import edu.rutgers.winlab.jmfapi.JMFAPI;
import edu.rutgers.winlab.jmfapi.JMFException;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.support.v4.app.NotificationCompat;
import android.support.v4.app.NotificationCompat.Builder;
import android.util.Log;

public class StackService extends Service {
	/*
	 * It should have the state of the stack process
	 * It should provide messages to give feedback
	 * It should send back the log if possible or at least save it somewhere
	 */
	private boolean running;
	private Process process;
	private Thread backgroundThread;
	private BackThread backgroundRunnable;

    final static String apptag = "MF_STACK_LAUNCHER";
	
	public StackService() {
		running = false;
	}
	
	
	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		//if not running launch the stack and start the notification in the foreground
		String GUID = intent.getStringExtra("GUID");
		int level = intent.getIntExtra("DEBUG_LEVEL", 1);
		start(startId, GUID, level);
		return super.onStartCommand(intent, flags, startId);
	}
	
	private void start(int startId, String GUID, int dLevel){
		if(!running){
			running = true;
			Intent resultIntent = new Intent(this, LauncherActivity.class);
			PendingIntent resultPendingIntent =
					PendingIntent.getActivity(this, 0, resultIntent, 0);
			Notification notification = new NotificationCompat.Builder(this)
		         .setContentTitle("MF Stack Status")
		         .setContentText("Starting the stack")
		         .setSmallIcon(R.drawable.ic_launcher)
		         .setContentIntent(resultPendingIntent)
		         .build();
			startForeground(startId, notification);
			try {
				//TODO: implement getting the input
				String l;
				if(dLevel == 1) l = "-f";
				else l = "-d";
				process = new ProcessBuilder()
				   .command("su", "-c", "mfandroidstack "+l+" /data/mfdemo/settings > /data/mfdemo/mflog 2> /data/mfdemo/mferr")
				   //.redirectErrorStream(true)
				   .start();
				/*InputStream in = process.getInputStream();
				InputStream err = process.getErrorStream();
				byte buffer[] = new byte[1024];
				int n = in.read(buffer);
				if(n>0) System.out.println(n + " " + new String(buffer,0,n-1));
				n =err.read(buffer);
				if(n>0) System.out.println(n + " " + new String(buffer,0,n-1));*/
				//readStream(in);
				backgroundRunnable = new BackThread(startId);
				backgroundThread = new Thread(backgroundRunnable);
				backgroundThread.start();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	}
	
	@Override
    public void onDestroy() {
        super.onDestroy();
        destroy();
    }
	
	private void destroy(){
		//Very bad way of implementing it, but there is not much we can do about it now...
		try {
			if (process != null) {
				/* process.destroy();
				 // use exitValue() to determine if process is still running. 
				 int ev = process.exitValue();
				 System.out.println(ev);*/
				Process temp = new ProcessBuilder()
						 .command("su","-c","killall mfandroidstack")
						 .start();
				temp.waitFor();
				backgroundRunnable.setCont(false);
				backgroundThread.interrupt();
				backgroundThread.join();
	         }
	     } catch (IOException e){ //(IllegalThreadStateException e) {
	    	 // process is still running, kill it.
	    	 /*System.out.println("The process is still running. I will kill more drastically...");
	    	 int ev = process.exitValue();
		     System.out.println(ev);*/
	    	 Log.e(apptag, "Problems killing the stack process: "+e.getMessage());
	     } catch (InterruptedException e){
             Log.e(apptag, "Problems killing the stack process: "+e.getMessage());
		 } finally {
			 //process.waitFor();
	    	 process = null;
		 }
	}

	
	@Override
	public IBinder onBind(Intent intent) {
		// TODO: Return the communication channel to the service.
		throw new UnsupportedOperationException("Not yet implemented");
	}
	
	public class BackThread implements Runnable {
		int notificationId;
		boolean cont;
		
		BackThread(int notId){
			notificationId = notId;
			cont = true;
		}
		
		public void setCont(boolean val){
			cont = val;
		}
		
		public void run(){
			JMFAPI temp;
			NotificationManager mNotificationManager =
			        (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
			Intent resultIntent = new Intent(getApplicationContext(), LauncherActivity.class);
			PendingIntent resultPendingIntent =
					PendingIntent.getActivity(getApplicationContext(), 0, resultIntent, 0);
			Builder mNotifyBuilder = new NotificationCompat.Builder(getApplicationContext())
					.setContentTitle("MF Stack Status")
					.setContentText("Starting the stack")
					.setSmallIcon(R.drawable.ic_launcher)
					.setContentIntent(resultPendingIntent);
			try {
				Thread.sleep(5000);
				while(cont){
					try{
						temp = new JMFAPI();
                        Log.d(apptag, "Call open to test if stack is running or not");
						temp.jmfopen("basic", new GUID(1));
						temp.jmfclose();
						mNotifyBuilder.setContentText("Running");
						System.out.println("Running");
						mNotificationManager.notify(notificationId,mNotifyBuilder.build());
					} catch (JMFException e){
						e.printStackTrace();
						mNotifyBuilder.setContentText("Problems with the stack");
						mNotificationManager.notify(notificationId,mNotifyBuilder.build());
					}
					Thread.sleep(60000);
				}
			} catch (InterruptedException e) {
                Log.e(apptag, "I was sleeping and got interrupted");
                e.printStackTrace();
			}
		}
	}
}

/*
Notification notification = new Notification(R.drawable.icon, getText(R.string.ticker_text), System.currentTimeMillis());
Intent notificationIntent = new Intent(this, ExampleActivity.class);
PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, notificationIntent, 0);
notification.setLatestEventInfo(this, getText(R.string.notification_title),
        getText(R.string.notification_message), pendingIntent);
startForeground(ONGOING_NOTIFICATION, notification);*/
