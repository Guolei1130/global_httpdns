package com.example.guolei.myapplication;

import android.content.Context;
import android.os.Build;

import com.alibaba.sdk.android.httpdns.DegradationFilter;
import com.alibaba.sdk.android.httpdns.HttpDns;
import com.alibaba.sdk.android.httpdns.HttpDnsService;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import okhttp3.Dns;

public class HttpDNS implements Dns {

    private HttpDnsService httpDns;

    {
        httpDns = HttpDns.getService(null, "");

        DegradationFilter degradationFilter = new DegradationFilter() {
            @Override
            public boolean shouldDegradeHttpDNS(String s) {
                return detectIfProxyExist(null);
            }
        };
        //
        httpDns.setDegradationFilter(degradationFilter);
        //pre resolve
        httpDns.setPreResolveHosts(new ArrayList<>(Arrays.asList("www.aliyun.com", "www.taobao.com")));
        httpDns.setExpiredIPEnabled(true);

        httpDns.getIpByHostAsync("");
    }

    @Override
    public List<InetAddress> lookup(String hostname) throws UnknownHostException {
        String ip = httpDns.getIpByHostAsync(hostname);
        if (ip != null) {
            return Arrays.asList(InetAddress.getAllByName(ip));
        }
        return Dns.SYSTEM.lookup(hostname);
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
}
