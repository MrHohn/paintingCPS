package edu.rutgers.winlab.mfstack.launcher;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.widget.Toast;

public class BootReceiver extends BroadcastReceiver {
	public BootReceiver() {
	}

	@Override
	public void onReceive(Context context, Intent intent) {
		SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(context);
		boolean boot = sharedPref.getBoolean("pref_boot", false);
		if ("android.intent.action.BOOT_COMPLETED".equals(intent.getAction()) && boot) {
			Intent myIntent = new Intent(context, StackService.class);
			context.startService(myIntent);
			Toast.makeText(context, "Stack service started", Toast.LENGTH_SHORT).show();
		}
	}
}
