
#include <jni.h>
#include <string>

#include <unistd.h>
#include <dlfcn.h>
#include <android/log.h>

#include <netdb.h>

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "xhook.h"
#include "AndHook.h"


#define TAG_NAME        "xhook"
#define log_error(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, TAG_NAME, (const char *) fmt, ##args)

#define AKLog(...) __android_log_print(ANDROID_LOG_VERBOSE, __FUNCTION__, __VA_ARGS__)
#define AKHook(X)  AKHookFunction(reinterpret_cast<void *>(X), reinterpret_cast<void *>(my_##X), reinterpret_cast<void **>(&sys_##X))

#define NETID_UNSET 0u
/*
 * MARK_UNSET represents the default (i.e. unset) value for a socket mark.
 */
#define MARK_UNSET 0u

static int (*fp)(const char *hostname, const char *servname,
                 const struct addrinfo *hints, unsigned netid, unsigned mark,
                 struct addrinfo **res);

static int (*fp_getaddrinfo)(const char *hostname, const char *servname,
                             const struct addrinfo *hints, struct addrinfo **res);

static JavaVM *sjavaVM;

static jclass httpDnsProviderClz, httpDnsServiceClz;
static jmethodID getHttpDnsServiceMethod, getIpByHostAsyncMethod;

extern "C" int initMethod(JNIEnv *env) {
    httpDnsProviderClz = static_cast<jclass>(env->NewGlobalRef(
            env->FindClass("com/example/guolei/myapplication/HttpDnsProvider")));
    if (httpDnsProviderClz == NULL) {
        return -1;
    }
    getHttpDnsServiceMethod = env->GetStaticMethodID(httpDnsProviderClz,
                                                     "getHttpDnsService",
                                                     "()Lcom/alibaba/sdk/android/httpdns/HttpDnsService;");
    httpDnsServiceClz = static_cast<jclass>(env->NewGlobalRef(
            env->FindClass("com/alibaba/sdk/android/httpdns/HttpDnsService")));
    if (httpDnsServiceClz == NULL) {
        return -1;
    }
    getIpByHostAsyncMethod = env->GetMethodID(httpDnsServiceClz,
                                              "getIpByHostAsync",
                                              "(Ljava/lang/String;)Ljava/lang/String;");
    if (getHttpDnsServiceMethod == NULL || getIpByHostAsyncMethod == NULL) {
        return -1;
    }
    return 0;
}

static const char *getIpByHttpDns(const char *hostname) {
    JNIEnv *senv;
    sjavaVM->AttachCurrentThread(&senv, NULL);

    jobject httpdnsserverobj = senv->CallStaticObjectMethod(httpDnsProviderClz,
                                                            getHttpDnsServiceMethod);
    jstring ip = static_cast<jstring>(senv->CallObjectMethod(httpdnsserverobj,
                                                             getIpByHostAsyncMethod,
                                                             senv->NewStringUTF(hostname)));

    if (ip) {
        return senv->GetStringUTFChars(ip, 0);
    }
    return NULL;
}

static int new_android_getaddrinfofornet(const char *hostname, const char *servname,
                                         const struct addrinfo *hints, unsigned netid,
                                         unsigned mark, struct addrinfo **res) {
    log_error("hahahha,wo hook dao l ->android_getaddrinfofornet ");
    log_error("下面是hostname");
    log_error(hostname, "");
    if (hints->ai_flags == AI_NUMERICHOST) {
        if (fp) {
            return fp(hostname, servname, hints, netid, mark, res);
        }
    } else {
        const char *ip = getIpByHttpDns(hostname);
        if (ip != NULL) {
            log_error("httpdns 解析成功，直接走IP");
            log_error("下面是ip");
            log_error(ip, "");
            return fp(ip, servname, hints, netid, mark, res);
        } else {
            return fp(hostname, servname, hints, netid, mark, res);
        }

    }

    return 0;
}

static int new_getaddrinfo(const char *hostname, const char *servname,
                           const struct addrinfo *hints, struct addrinfo **res) {
    log_error("hahahha,wo hook dao l ->getaddrinfo ");
    log_error("下面是hostname");
    log_error(hostname, "");
    if (hints->ai_flags == AI_NUMERICHOST) {
        if (fp) {
            return fp_getaddrinfo(hostname, servname, hints, res);
        }
    } else {
        const char *ip = getIpByHttpDns(hostname);
        if (ip != NULL) {
            log_error("httpdns 解析成功，直接走IP");
            log_error("下面是ip");
            log_error(ip, "");
            return fp_getaddrinfo(ip, servname, hints, res);
        } else {
            return fp_getaddrinfo(hostname, servname, hints, res);
        }

    }
    return 0;
}

extern "C" int hook_android_getaddrinfofornet() {
    xhook_register(".*\\.so$", "android_getaddrinfofornet",
                   (void *) new_android_getaddrinfofornet, reinterpret_cast<void **>(&fp));
    xhook_enable_debug(1);
    xhook_refresh(1);
    log_error("版本 21以上");
    return 0;
}

extern "C" JNIEXPORT jstring
JNICALL
Java_com_example_guolei_myapplication_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";

    return env->NewStringUTF(hello.c_str());
}


static int
(*sys_getaddrinfo)(const char *__node, const char *__service, const struct addrinfo *__hints,
                   struct addrinfo **__result);

static int my_getaddrinfo(const char *__node, const char *__service, const struct addrinfo *__hints,
                          struct addrinfo **__result) {

    // 这里有用xHook的时候，把所有的的android_getaddrinfofornet方法都替换为new_android_getaddrinfofornet,
    // 因此我们这里直接调用 new_android_getaddrinfofornet就行
    return new_android_getaddrinfofornet(__node, __service, __hints, NETID_UNSET, MARK_UNSET,
                                         __result);
}

static int JNICALL hook_dns(JNIEnv *, jobject) {
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

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_guolei_myapplication_MainActivity_nativeInit(JNIEnv *env, jobject instance) {
    env->GetJavaVM(&sjavaVM);
    if (initMethod(env) == -1) {
        return -1;
    }
    hook_android_getaddrinfofornet();
    hook_dns(env, instance);
    return 0;
}
