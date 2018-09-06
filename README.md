### 2018/9/6日更新

写一个jar包，以compileOnly的方式依赖，就可以不反射了

```
  public void hookOs() {
    Os origin = Libcore.os;

    Libcore.os = (Os) Proxy.newProxyInstance(origin.getClass().getClassLoader(),
        new Class[]{Os.class},
        new OsInvokeHandler(origin));

  }
```

### 什么是HttpDns，为什么要接入HttpDns

>HTTPDNS使用HTTP协议进行域名解析，代替现有基于UDP的DNS协议，域名解析请求直接发送到阿里云的HTTPDNS服务器，从而绕过运营商的Local DNS，能够避免Local DNS造成的域名劫持问题和调度不精准问题。

### HttpDns最佳实践？

阿里云文档中，提到的HttpDns的最佳实践的几个场景，虽然在一定程度上能解决我们的问题，但是存在一定的缺陷、

* webview，能用httpdns的请求方式有限
* 无法做到全局替换

那么，我们是否能够寻找一种全局替换的方案呢？或者解决的场景能够更多一点的方案呢？

### 从现有的资料中寻找方案

经过一番搜索，找到了 Android弟的这边文章 [一种全局拦截并监控 DNS 的方式](https://fucknmb.com/2018/04/16/%E4%B8%80%E7%A7%8D%E5%85%A8%E5%B1%80%E6%8B%A6%E6%88%AA%E5%B9%B6%E7%9B%91%E6%8E%A7DNS%E7%9A%84%E6%96%B9%E5%BC%8F/) 以及这边文章[如何为Android应用提供全局的HttpDNS服务](https://juejin.im/entry/5a11c3db51882561a20a11cc)。 在第一篇文章提到的方案中，缺陷是非常明显的

1. 只支持7.0以上版本
2. 不支持webview，虽然作者说支持webview，实际上是不支持的，为什么不支持，后面会给出解释

而在第一篇文章中，提到的方案也是这样。在7.0之下 hook coonnect这个，没有相应的代码，对我我等菜鸡来说，难度太大。

因此，现有的方案中，不合适。

### 从源码中寻求突破方法-动态代理Os接口

我们先来看下Dns解析的过程是什么样的，以API 25的SDK为例，发现下面的代码片段。

```
            StructAddrinfo hints = new StructAddrinfo();
            hints.ai_flags = AI_ADDRCONFIG;
            hints.ai_family = AF_UNSPEC;
            // If we don't specify a socket type, every address will appear twice, once
            // for SOCK_STREAM and one for SOCK_DGRAM. Since we do not return the family
            // anyway, just pick one.
            hints.ai_socktype = SOCK_STREAM;
            InetAddress[] addresses = Libcore.os.android_getaddrinfo(host, hints, netId);
```

那么，我们继续跟踪源代码。看看Libcore.os是个什么东西。

```
public final class Libcore {
    private Libcore() { }
    public static Os os = new BlockGuardOs(new Posix());
}
```

而Os是一个接口，代码片段如下。

```

public interface Os {
    public FileDescriptor accept(FileDescriptor fd, SocketAddress peerAddress) throws ErrnoException, SocketException;
    public boolean access(String path, int mode) throws ErrnoException;
    public InetAddress[] android_getaddrinfo(String node, StructAddrinfo hints, int netId) throws GaiException;
```

Posix的代码片段如下

```
public final class Posix implements Os {
    Posix() { }
    public native FileDescriptor accept(FileDescriptor fd, SocketAddress peerAddress) throws ErrnoException, SocketException;
    public native boolean access(String path, int mode) throws ErrnoException;
    public native InetAddress[] android_getaddrinfo(String node, StructAddrinfo hints, int netId) throws GaiException;
```

Libcore.os.android_getaddrinfo实际上是调用的Posix的android_getaddrinfo这个native方法。

到此为止，我们可以明确，通过动态代理Os这个接口，并且替换Libcore.os这个字段，我们在调用android_getaddrinfo这个方法中，插入HttpDns解析，那么，就可以解决部分的问题(除webview以外的所有场景)。当然，我们要考虑适配的问题，在4.4版本上，android_getaddrinfo这个方法是getaddrinfo。因此，我们写下如下的代码。

```

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
                Log.e("xhook", "ai_flag get error ");
            }

            if (args[0] instanceof String && mAiFlagsField != null
                    && ((int) mAiFlagsField.get(args[1]) != OsConstants.AI_NUMERICHOST)) {
                //这里需要注意的是，当host为ip的时候，不进行拦截。
                String host = (String) args[0];
                String ip = HttpDnsProvider.getHttpDnsService().getIpByHostAsync(host);
                Log.e("xhook", "invoke: success -> host:" + host + "; ip :" + ip);
                return InetAddress.getAllByName(ip);
            }
        }
        try {
            return method.invoke(mOrigin, args);
        } catch (InvocationTargetException e) {
            throw e.getCause();
        }
    }
}

```

但是，这种方法也是有缺陷的。

1. 9.0 非公开API限制
2. Webview你还没给我解决(别急，WebView放在最后，狗头保命)

### 进一步优化-下沉到so hook

既然他调用的是native的方法，那么，我们可不可以通过hook 这个native方法去达到目的呢？当然可以。我们先看下Posix的native方法对应的代码。代码在[Libcore_io_Posix.cpp中](https://android.googlesource.com/platform/libcore/+/android-cts-7.1_r16/luni/src/main/native/libcore_io_Posix.cpp)，如下。


![](https://user-gold-cdn.xitu.io/2018/9/1/165939c2a3ae434e?w=1896&h=1510&f=png&s=381796)

注意看，这个实际上是调用的android_getaddrinfofornet这个方法，实际上这个方法的实现是在libc中的，但是，我们现在暂时略过inline hook这个方法，一步一步来。那么。我们能如何hook到这个方法呢？咦，好像爱奇艺最近开源了一个hook方案，试一下？捎带说一下，这些代码最后是编译成libjavacore.so的。说试就试，动手写下如下代码。PS：忽略我垃圾的c++ style

```

static int new_android_getaddrinfofornet(const char *hostname, const char *servname,
                                         const struct addrinfo *hints, unsigned netid,
                                         unsigned mark, struct addrinfo **res) {
    LOGE("hahahha,wo hook dao l ->android_getaddrinfofornet ");
    LOGE("下面是hostname");
    LOGE(hostname, "");
    if (hints->ai_flags == AI_NUMERICHOST) {
        if (fp_android_getaddrinfofornet) {
            return fp_android_getaddrinfofornet(hostname, servname, hints, netid, mark, res);
        }
    } else {
        const char *ip = getIpByHttpDns(hostname);
        if (ip != NULL) {
            LOGE("httpdns 解析成功，直接走IP");
            LOGE("下面是ip");
            LOGE(ip, "");
            //这里就比较神奇了，传的是IP，但是ai_flags 不是ip，但是还正常,查看源码的时候，没发现在这个代码里面判断
            // ai_flags 不numberhost还是其他
            return fp_android_getaddrinfofornet(ip, servname, hints, netid, mark, res);
        } else {
            return fp_android_getaddrinfofornet(hostname, servname, hints, netid, mark, res);
        }

    }

    return 0;
}

extern "C" int hook_android_getaddrinfofornet() {
    if (fp_android_getaddrinfofornet) {
        return 0;
    }
    //libjavacore.so 这里可以换成
    int result = xhook_register(".*\\libjavacore.so$", "android_getaddrinfofornet",
                                (void *) new_android_getaddrinfofornet,
                                reinterpret_cast<void **>(&fp_android_getaddrinfofornet));
    xhook_refresh(1);
#if DEBUG
    xhook_enable_sigsegv_protection(0);
    xhook_enable_debug(1);
    LOGE("built type debug");
#elif RELEASE
    xhook_enable_sigsegv_protection(1);
    xhook_enable_debug(0);
    LOGE("built type release");
#endif
    return result;
}

```

上面省略了一点代码(完整的代码链接放在末尾)。运行,一般场景，一点问题没有，舒服。那么，用Webview试下。结果，出问题了，没走。。好吧，我们把".*\\libjavacore.so$"换成 ".*\\*.so$",一股脑都替换，我就不信了。再试。还是不行。。。。Webview成精了。这个方案还是没解决Webview的问题，好气！

优点：

1. 兼容性强,兼容4.4-9.0

缺点：

1. 还是不支持Webview


### 寻找Webview的解决方案

一番搜索之后，发现如下文章。[webview接入HttpDNS实践](https://blog.csdn.net/u012455213/article/details/78281026) 但是，结果啪啪打脸，没关系，总算给了我们点思路不是么？文中作者说我们hook掉libwebviewchromium.so 就行，ok，按照他的思路，我们去/system/app/WebViewGoogle/lib/arm/libwebviewchromium.so这个路径下看看有没有这个so(这里说明一下，我是小米6 8.0),看下结果。


![](https://user-gold-cdn.xitu.io/2018/9/1/16593aaa8415769f?w=647&h=558&f=png&s=103237)

没有啊！！

我把这个apk拉出来，用jadx反编译，看了看。发现这个库是通过System.loadLibrary去加载的
![](https://user-gold-cdn.xitu.io/2018/9/1/16593b78e922581f?w=468&h=248&f=png&s=23898)
![](https://user-gold-cdn.xitu.io/2018/9/1/16593aca0b7b4d23?w=1152&h=302&f=png&s=75579)

场面一度十分尴尬。现在，我们查看下进程的maps信息。

![](https://user-gold-cdn.xitu.io/2018/9/1/16593b64d2d5aa86?w=2242&h=426&f=png&s=165017)

在关于webview的这几个so里面，都没有找到android_getinfofornet。没得办法，我们只能去看下libwebviewchromium。

![](https://user-gold-cdn.xitu.io/2018/9/1/16593b8823139d56?w=1406&h=142&f=png&s=51762)

哦？也是用的libc的。爱奇艺的xHook，是PCL/GOT表hook方案，而这个so，我们又加载不到我们的进程来，没办法，只能inline hook libc.so了。

### inline hook，webview也可以了。

我们先看下相关的代码。

```

__BIONIC_WEAK_FOR_NATIVE_BRIDGE
int
getaddrinfo(const char *hostname, const char *servname,
    const struct addrinfo *hints, struct addrinfo **res)
{
	return android_getaddrinfofornet(hostname, servname, hints, NETID_UNSET, MARK_UNSET, res);
}
__BIONIC_WEAK_FOR_NATIVE_BRIDGE
```

在上面的过程中，我们已经hook 掉了android_getaddrinfofornet，因此，我们只要hook,getaddrinfo，让这个方法调用我们自己的掉了android_getaddrinfofornet方法就可以解决了，美滋滋。

我们现在的问题变成了，哪个inline hook的方案稳定。这个事情很恐怖，因为很多inline hook的相对稳定的方案是不开源的，我这里呢？用的是Lody的AndHook。所有，最后的代码如下。


```
static int my_getaddrinfo(const char *__node, const char *__service, const struct addrinfo *__hints,
                          struct addrinfo **__result) {

    // 这里有用xHook的时候，把所有的的android_getaddrinfofornet方法都替换为new_android_getaddrinfofornet,
    // 因此我们这里直接调用 new_android_getaddrinfofornet就行
    if (fp_android_getaddrinfofornet != NULL) {
        return new_android_getaddrinfofornet(__node, __service, __hints, NETID_UNSET, MARK_UNSET,
                                             __result);
    } else if (fp_android_getaddrinfoforiface != NULL) {
        return new_android_getaddrinfoforiface(__node, __service, __hints, NULL, 0,
                                               __result);
    }
    return EAI_FAIL;
}

static int JNICALL hooj_libc_getaddrinfo(JNIEnv *, jobject) {
    static bool hooked = false;
    int result = -1;

    AKLog("starting native hook...");
    if (!hooked) {
        AKHook(getaddrinfo);

        // typical use case
        const void *libc = AKGetImageByName("libc.so");
        if (libc != NULL) {
            AKLog("base address of libc.so is %p", AKGetBaseAddress(libc));

            void *p = AKFindSymbol(libc, "getaddrinfo");
            if (p != NULL) {
                AKHookFunction(p,                                        // hooked function
                               reinterpret_cast<void *>(my_getaddrinfo),   // our function
                               reinterpret_cast<void **>(&sys_getaddrinfo) // backup function pointer
                );
                AKLog("hook getaddrinfo success");
                result = 0;
            }
            AKCloseImage(libc);
        } //if

        hooked = true;
    } //if

    return result;
}

```

编译运行、测试，ok。一切顺利。


### 总结

到此，对HttpDns全局替换的研究就告一段落了。我们最终实现的方案还是不错的。

1. 支持4.4以上版本
2. 支持Webview


当然，缺点也是相当比较明显的。

1. 依赖inline hook，inline hook的方案是相对比较复杂、兼容性也比较差的，不敢保证Lody大神的AndHook绝对稳定可靠
2. 鬼知道国内的厂商会不会随便修改函数名、so名


[这里是全套代码，喜欢的给个star](https://github.com/Guolei1130/global_httpdns)

