package com.example.guolei.myapplication;

import android.content.Context;
import android.os.Build;
import android.util.Log;

import com.alibaba.sdk.android.httpdns.DegradationFilter;
import com.alibaba.sdk.android.httpdns.HttpDns;
import com.alibaba.sdk.android.httpdns.HttpDnsService;

import java.lang.reflect.Field;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;

public class HttpDnsProvider {

    private static volatile HttpDnsService sHttpDnsService;

    public static HttpDnsService getHttpDnsService() {
        return sHttpDnsService;
    }

    public static void init(Context context) {
        sHttpDnsService = HttpDns.getService(context, "145232",
                "a2d05dad155421cc8fb1d9d78408c1b8");

        DegradationFilter degradationFilter = new DegradationFilter() {
            @Override
            public boolean shouldDegradeHttpDNS(String s) {
                return detectIfProxyExist(null);
            }
        };
        //
        sHttpDnsService.setDegradationFilter(degradationFilter);
        //pre resolve
        sHttpDnsService.setPreResolveHosts(new ArrayList<>(Arrays.asList("www.aliyun.com",
                "www.taobao.com",
                "s.vipkidstatic.com")));
        sHttpDnsService.setExpiredIPEnabled(true);

    }

    private static boolean detectIfProxyExist(Context ctx) {
        boolean IS_ICS_OR_LATER = Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH;
        String proxyHost;
        int proxyPort;
        if (IS_ICS_OR_LATER) {
            proxyHost = System.getProperty("http.proxyHost");
            String port = System.getProperty("http.proxyPort");
            proxyPort = Integer.parseInt(port != null ? port : "-1");
        } else {
            proxyHost = android.net.Proxy.getHost(ctx);
            proxyPort = android.net.Proxy.getPort(ctx);
        }
        return proxyHost != null && proxyPort != -1;
    }


    private static volatile boolean mHooked;

    public static void globalReplaceByHookOs() {
        if (mHooked) {
            return;
        }
        mHooked = true;
        try {
            Class libcoreClz = Class.forName("libcore.io.Libcore");
            Field osField = libcoreClz.getField("os");
            Object origin = osField.get(null);
            Object proxy = Proxy.newProxyInstance(libcoreClz.getClassLoader(),
                    new Class[]{Class.forName("libcore.io.Os")},
                    new OsInvokeHandler(origin));
            osField.set(null, proxy);
        } catch (Exception e) {
            e.printStackTrace();
            Log.e("xhook", "globalReplaceByHookOs: " + e.getMessage());
        }
    }
}
