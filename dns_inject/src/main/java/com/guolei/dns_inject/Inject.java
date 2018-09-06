package com.guolei.dns_inject;

//                    _    _   _ _
//__      _____  _ __| | _| |_(_) | ___
//\ \ /\ / / _ \| '__| |/ / __| | |/ _ \
// \ V  V / (_) | |  |   <| |_| | |  __/
//  \_/\_/ \___/|_|  |_|\_\\__|_|_|\___|


import java.lang.reflect.Proxy;

import libcore.io.Libcore;
import libcore.io.Os;

/**
 * Copyright © 2013-2018 Worktile. All Rights Reserved.
 * Author: guolei
 * Email: 1120832563@qq.com
 * Date: 18/9/6
 * Time: 下午9:58
 * Desc:
 */
public class Inject {

    private static DnsInterface sDnsInterface;
    private static volatile boolean sHooked;

    public static void injectDns(DnsInterface dnsInterface){
        sDnsInterface = dnsInterface;
        hookOs();
    }

    private static void hookOs() {
        if (sHooked) {
            return;
        }
        Os origin = Libcore.os;
        Libcore.os = (Os) Proxy.newProxyInstance(origin.getClass().getClassLoader(),
                new Class[]{Os.class},new OsInvokeHandler(origin));
        sHooked = true;
    }

    public static DnsInterface getDnsInterface() {
        return sDnsInterface;
    }
}
