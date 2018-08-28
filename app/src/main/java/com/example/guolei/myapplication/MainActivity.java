package com.example.guolei.myapplication;

import android.os.Process;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.webkit.WebChromeClient;
import android.webkit.WebResourceRequest;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Button;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.UnknownHostException;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("xhook");
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        HttpDnsProvider.init(this);
        stringFromJNI();
        nativeInit();
        inlineHook();
        // Example of a call to a native method
        Button tv = (Button) findViewById(R.id.sample_text);
        tv.setText("xxxxx");
        tv.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        try {
                            InetAddress[] addresses = InetAddress.getAllByName("s.vipkidstatic.com");
                            if (addresses == null || addresses.length == 0) {

                            } else {
                                Log.e("xhook", "run: "
                                        + addresses[0].toString());
                            }
                        } catch (UnknownHostException e) {
                            e.printStackTrace();
                        }
                    }
                }).start();
            }
        });

        final WebView webView = findViewById(R.id.webview);
        webView.getSettings().setJavaScriptEnabled(true);
        webView.setWebChromeClient(new WebChromeClient() {

        });
        webView.setWebViewClient(new WebViewClient() {
            @Override
            public boolean shouldOverrideUrlLoading(WebView view, WebResourceRequest request) {
                return false;
            }
        });

        findViewById(R.id.load_webview).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                webView.loadUrl("http://www.aliyun.com");
            }
        });

        findViewById(R.id.read_map).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                int pid = Process.myPid();
                InputStream is = null;
                try {
                    is = Runtime.getRuntime().exec("cat /proc/" + pid + "/maps")
                            .getInputStream();
                    if (is != null) {
                        byte[] buffer = new byte[2048];
                        int len;
                        StringBuffer sb = new StringBuffer();
                        File file = new File(getExternalCacheDir(),"log.txt");
                        if (file.exists()) {
                            file.delete();
                        }
                        OutputStream outputStream = new FileOutputStream(file);
                        while ((len = is.read(buffer)) != -1) {
                            sb.append(new String(buffer, len));
                            outputStream.write(buffer,0,len);
                            outputStream.flush();
                        }
                        TextView tv = findViewById(R.id.message);
                        tv.setText(sb.toString());

                    }
                } catch (IOException e) {
                    e.printStackTrace();
                }finally {
                    if (is != null) {
                        try {
                            is.close();
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                }
            }
        });
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();

    public native int nativeInit();

    public native int inlineHook();
}
