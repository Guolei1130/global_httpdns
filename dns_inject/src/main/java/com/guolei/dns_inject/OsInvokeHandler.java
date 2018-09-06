package com.guolei.dns_inject;

import android.system.OsConstants;
import android.text.TextUtils;
import android.util.Log;

import java.lang.reflect.Field;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.net.InetAddress;

public class OsInvokeHandler implements InvocationHandler {

    private Object mOrigin;
    private Field mAiFlagsField;

    OsInvokeHandler(Object os) {
        mOrigin = os;
    }

    @Override
    public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
        if (method.getName().equals("android_getaddrinfo")
                || method.getName().equals("getaddrinfo")) {
            try {
                if (mAiFlagsField == null) {
                    mAiFlagsField = args[1].getClass().getDeclaredField("ai_flags");
                    mAiFlagsField.setAccessible(true);
                }
            }catch (Exception e) {
                e.printStackTrace();
            }

            if (args[0] instanceof String && mAiFlagsField != null
                    && ((int) mAiFlagsField.get(args[1]) != OsConstants.AI_NUMERICHOST)) {
                //这里需要注意的是，当host为ip的时候，不进行拦截。
                String host = (String) args[0];
                String ip = Inject.getDnsInterface().getIpByHost(host);
                if (!TextUtils.isEmpty(ip)) {
                    return InetAddress.getAllByName(ip);
                }
            }
        }
        try {
            return method.invoke(mOrigin, args);
        } catch (InvocationTargetException e) {
            throw e.getCause();
        }
    }
}