
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

#define TAG_NAME        "xhook"
#define log_error(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, TAG_NAME, (const char *) fmt, ##args)


static int (*fp)(const char *hostname, const char *servname,
                 const struct addrinfo *hints, unsigned netid, unsigned mark,
                 struct addrinfo **res);

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
            fp(hostname, servname, hints, netid, mark, res);
        }
    } else {
        const char *ip = getIpByHttpDns(hostname);
        if (ip != NULL) {
            log_error("httpdns 解析成功，直接走IP");
            log_error("下面是ip");
            log_error(ip, "");
            fp(ip, servname, hints, netid, mark, res);
        } else {
            fp(hostname, servname, hints, netid, mark, res);
        }

    }

    return 0;
}

extern "C" int hook_libjavacore() {
    xhook_register("/system/lib/libjavacore.so", "android_getaddrinfofornet",
                   (void *) new_android_getaddrinfofornet, reinterpret_cast<void **>(&fp));
    xhook_refresh(1);
    xhook_enable_debug(1);
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

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_guolei_myapplication_MainActivity_nativeInit(JNIEnv *env, jobject instance) {
    env->GetJavaVM(&sjavaVM);
    if (initMethod(env) == -1) {
        return -1;
    }
    hook_libjavacore();
    return 0;
}