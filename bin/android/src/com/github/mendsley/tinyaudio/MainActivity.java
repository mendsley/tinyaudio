package com.github.mendsley.tinyaudio;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;

public class MainActivity extends Activity {

	@Override
	public void onCreate(Bundle savedInstance) {
		super.onCreate(savedInstance);

		TextView textView = new TextView(this);
		textView.setText("TinyAudio example");
		setContentView(textView);

		startSound();
	}

	private native void startSound();

	static {
		System.loadLibrary("sin");
	}
}
