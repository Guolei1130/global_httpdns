package com.example.guolei.myapplication;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import java.net.InetAddress;
import java.net.UnknownHostException;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("test2");
        System.loadLibrary("xhook");
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method
        Button tv = (Button) findViewById(R.id.sample_text);
        tv.setText("xxxxx");
        tv.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                stringFromJNI();
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        try {
                            InetAddress[] addresses = InetAddress.getAllByName("www.baidu.com");
                            if (addresses.length == 0) {

                            }else {
                                Log.e("guolei", "run: "
                                         +addresses[0] .toString());
                            }
                        } catch (UnknownHostException e) {
                            e.printStackTrace();
                        }
                    }
                }).start();
            }
        });
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}
