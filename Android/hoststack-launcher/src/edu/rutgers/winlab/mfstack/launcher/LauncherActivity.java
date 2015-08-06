package edu.rutgers.winlab.mfstack.launcher;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;


import android.location.Location;
import android.os.Bundle;
import android.app.Activity;
import android.app.ActivityManager;
import android.app.ActivityManager.RunningServiceInfo;
import android.content.Context;
import android.content.Intent;
import android.content.res.AssetManager;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.EditText;
import android.widget.RadioGroup;
import android.widget.Toast;

/*
 * Activity that launches the stack
 * Initially will only launch it
 * End get the status of the stack
 */

public class LauncherActivity extends Activity {

    private final static String CLASS_TAG = "mf.LauncherActivity";

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_launcher);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.activity_launcher, menu);
		return true;
	}
	
	public void startStack(View view) throws IOException {
		if(!isServiceRunning()){
			Intent intent = new Intent(this, StackService.class);
			RadioGroup rg = (RadioGroup)findViewById(R.id.radioService);
			int level;
			if(rg.getCheckedRadioButtonId() == R.id.radioQuiet){
				level = 1;
			}
			else{
				level = 2;
			}
			intent.putExtra("DEBUG_LEVEL", level);
			startService(intent);
		}
		else{
			Toast.makeText(this, "Service already running", Toast.LENGTH_SHORT).show();
		}
	}
	
	public void stopStack(View view) throws IOException {
		Intent intent = new Intent(this, StackService.class);
		stopService(intent);
	}
	
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		startActivity(new Intent(this, SettingsActivity.class));
		return super.onOptionsItemSelected(item);
	}

	public void logStack(View view) throws IOException {
		Toast.makeText(this, "Function not yet implemented", Toast.LENGTH_SHORT).show();
		//Intent intent = new Intent(this, LogStatusActivity.class);
		//startService(intent);
	}

    private void createFileSystemStructure(DataOutputStream os) throws IOException{
        Log.d(CLASS_TAG, "Creating folder /data/mfdemo");
        os.writeBytes("mkdir /data/mfdemo\n" +
                "chmod 777 /data/mfdemo\n");
    }

    private void copyAssets(DataOutputStream os) {
        copyFileFromAssets(os, "libgnustl_shared.so", "/system/lib/", "644");
        copyFileFromAssets(os, "libpcap.so", "/system/lib/", "644");
        copyFileFromAssets(os, "libmfapi_jni.so", "/system/lib/", "644");
        copyFileFromAssets(os, "mfandroidstack", "/system/bin/", "744");
    }

    private void copyFileFromAssets(DataOutputStream os, String fileName, String destFolder, String mod){
        byte[] buffer = new byte[1024];
        int read;
        AssetManager assetManager = getAssets();
        InputStream in;
        FileOutputStream out;
        try {
            in = assetManager.open(fileName);
            out = openFileOutput(fileName, MODE_PRIVATE);
            while((read = in.read(buffer)) != -1){
                out.write(buffer, 0, read);
            }
            in.close();
            out.flush();
            out.close();
            String tempPath = getFilesDir().getAbsolutePath();
            String command = String.format("cp %s %s\n", tempPath + "/" +fileName, destFolder) +
                    String.format("chmod %s %s\n", mod, destFolder+"/"+fileName);
            Log.d(CLASS_TAG, command);
            os.write(command.getBytes());
        } catch (IOException e) {
            Log.e(CLASS_TAG, "Failed to copy asset file: " + e.getMessage());
        }
    }

    private void createSettingsFile(DataOutputStream os, int GUID) throws IOException{
        FileOutputStream out;
        try {
            out = openFileOutput("settings", MODE_PRIVATE);
            String toWrite = "INTERFACE = wifi,wlan0,auto\n" +
                    "POLICY = bestperformance\n"+
                    "BUFFER_SIZE = 10\n" +
                    "DEFAULT_GUID = " + Integer.toString(GUID) + "\n" +
                    "IF_SCAN_PERIOD = 5\n";
            out.write(toWrite.getBytes(), 0, toWrite.getBytes().length);
            out.flush();
            out.close();
            String tempPath = getFilesDir().getAbsolutePath();
            String command = String.format("cp %s %s\n", tempPath + "/settings", "/data/mfdemo/settings") +
                    String.format("chmod 777 %s\n", "/data/mfdemo/settings");
            Log.d(CLASS_TAG, command);
            os.writeBytes(command);
        } catch (IOException e) {
            Log.e(CLASS_TAG, "Failed to copy asset file: " + e.getMessage());
        }
    }

    private void deleteTempFiles() {
        deleteFile ("libgnustl_shared.so");
        deleteFile("libpcap.so");
        deleteFile("libmfapi.so");
        deleteFile("mfandroidstack");
        deleteFile("settings");
    }
	
	public void installStack(View view) throws IOException {
		Process p = null;
        EditText editText = (EditText)findViewById(R.id.editGUID);
        int i;
        try {
            i = Integer.valueOf(editText.getText().toString());
        } catch (Exception e) {
            i = 1;
        }
		try {
			p = Runtime.getRuntime().exec("su");
		} catch (IOException e) {
			e.printStackTrace();
		}
		try {
            DataOutputStream os = new DataOutputStream(p.getOutputStream());
            os.writeBytes("mount -orw,remount /system\n");
            os.flush();
            copyAssets(os);
            os.flush();
            createFileSystemStructure(os);
            os.flush();
            createSettingsFile(os, i);
            os.flush();
            os.writeBytes("mount -oro,remount /system\n");
            os.flush();
			os.writeBytes("exit\n");
			os.flush();
            DataInputStream is = new DataInputStream(p.getInputStream());
            String output = "";
            byte[] buffer = new byte[1024*10];
            int read;
            while(true){
                read = is.read(buffer);
                if(read >0) output += new String(buffer, 0, read);
                if(read<1024*10){
                    //we have read everything
                    break;
                }
            }
            Log.d(CLASS_TAG, "Output from system: " + output);
			p.waitFor();
			os.close();
            //deleteTempFiles();
            int exitValue =  p.exitValue();
		if (exitValue == 0) {
			Log.d(CLASS_TAG, "Everything perfect");
		}
		else {
			Log.e(CLASS_TAG, "Error somewhere: " + Integer.toString(exitValue));
		}
		} catch (InterruptedException e) {
            Log.e(CLASS_TAG, e.getMessage());
		} catch (NullPointerException nullPointerException) {
            Log.e(CLASS_TAG, nullPointerException.getMessage());
        }
	}
	
	private boolean isServiceRunning() {
		ActivityManager manager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
		for (RunningServiceInfo service : manager.getRunningServices(Integer.MAX_VALUE)) {
			if (StackService.class.getName().equals(service.service.getClassName())) {
				return true;
			}
		}
		return false;
	}

}
