package edu.rutgers.winlab.mfapi.mfping;

import android.os.Bundle;
import android.app.Activity;
import android.content.Intent;
import android.view.Menu;
import android.view.View;
import android.widget.EditText;
import android.widget.RadioGroup;
import android.widget.Toast;

public class MainActivity extends Activity {

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.activity_main, menu);
		return true;
	}
	
	public void onStart(View view){
		boolean client;
		String dstGUID, myGUID;
		EditText text = (EditText)findViewById(R.id.editGUID);
		myGUID = text.getText().toString();
		if(myGUID.isEmpty()){
			Toast.makeText(this, "You need to select a GUID", Toast.LENGTH_SHORT).show();
			return;
		}
		text = (EditText)findViewById(R.id.editGUID2);
		dstGUID = text.getText().toString();
		if(dstGUID.isEmpty()){
			Toast.makeText(this, "You need to select a destination GUID", Toast.LENGTH_SHORT).show();
			return;
		}
		System.out.println("Local GUID = " + myGUID);
		System.out.println("Destionation GUID = " + dstGUID);
		RadioGroup rg = (RadioGroup)findViewById(R.id.radioService);
		client = rg.getCheckedRadioButtonId() == R.id.radioClient;
		System.out.println("Client? " + client);
		Intent intent;
    	intent = new Intent(this, PingActivity.class);
    	//intent.putExtra("TYPE_OF_SERVICE", which_check);
    	intent.putExtra("CLIENT", client);
    	intent.putExtra("MYGUID", myGUID);
    	intent.putExtra("DSTGUID", dstGUID);
    	startActivity(intent);
	}

}
