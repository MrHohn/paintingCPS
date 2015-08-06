package edu.rutgers.winlab.mfapi.mfping;

import android.os.AsyncTask;
import android.os.Bundle;
import android.app.Activity;
import android.content.Intent;
import android.view.Menu;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

public class PingActivity extends Activity {
	boolean client;
	String myGUID;
	String dstGUID;
	PingThread pt;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_ping);
		Intent intent = getIntent();
		client = intent.getBooleanExtra("CLIENT", true);
		myGUID = intent.getStringExtra("MYGUID");
		dstGUID = intent.getStringExtra("DSTGUID");
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.ping, menu);
		return true;
	}

	@Override
	protected void onStart() {
		super.onStart();
		if(pt == null){
			pt = new PingThread(1.0, myGUID, dstGUID, client, this);
			pt.execute(Integer.valueOf(1));
		}
	}
	
	@Override
	protected void onStop() {
		System.out.println("Stopping the application");
		super.onStop();
	}

	public void stopB(){
		System.out.println("Starting stopping process");
		pt.stop();
	}
	
	public void updateUI(String message){
		LinearLayout lView = (LinearLayout)findViewById(R.id.actionsProgression);
	    System.out.println(message);
	    TextView myText = new TextView(this);
	    myText.setText(message);
	    myText.setTextSize(13);
	    myText.setPadding(1, 3, 1, 3);
	    lView.addView(myText);
	}

}
