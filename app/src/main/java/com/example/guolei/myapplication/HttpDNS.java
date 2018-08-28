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

    private HttpDnsService httpDns = HttpDnsProvider.getHttpDnsService();

    @Override
    public List<InetAddress> lookup(String hostname) throws UnknownHostException {
        String ip = httpDns.getIpByHostAsync(hostname);
        if (ip != null) {
            return Arrays.asList(InetAddress.getAllByName(ip));
        }
        return Dns.SYSTEM.lookup(hostname);
    }


}
