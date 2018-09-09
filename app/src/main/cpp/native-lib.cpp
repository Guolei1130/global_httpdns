
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


#define TAG_NAME        "httpdns"
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, TAG_NAME, (const char *) fmt, ##args)

#define AKLog(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG_NAME, __VA_ARGS__)
#define AKHook(X)  AKHookFunction(reinterpret_cast<void *>(X), reinterpret_cast<void *>(my_##X), reinterpret_cast<void **>(&sys_##X))

#define NETID_UNSET 0u
/*
 * MARK_UNSET represents the default (i.e. unset) value for a socket mark.
 */
#define MARK_UNSET 0u

static int (*fp_android_getaddrinfofornet)(const char *hostname, const char *servname,
                                           const struct addrinfo *hints, unsigned netid,
                                           unsigned mark,
                                           struct addrinfo **res);

static int (*fp_android_getaddrinfoforiface)(const char *hostname, const char *servname,
                                             const struct addrinfo *hints, const char *iface,
                                             int mark, struct addrinfo **res);

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
      addrinfo *newAddrinfo = new addrinfo;
      newAddrinfo->ai_addr = hints->ai_addr;
      newAddrinfo->ai_addrlen = hints->ai_addrlen;
      newAddrinfo->ai_canonname = hints->ai_canonname;
      newAddrinfo->ai_family = hints->ai_family;
      newAddrinfo->ai_next = hints->ai_next;
      newAddrinfo->ai_socktype = hints->ai_socktype;
      newAddrinfo->ai_protocol = hints->ai_protocol;
      newAddrinfo->ai_flags = AI_NUMERICHOST;
      return fp_android_getaddrinfofornet(ip, servname, newAddrinfo, netid, mark, res);
    } else {
      return fp_android_getaddrinfofornet(hostname, servname, hints, netid, mark, res);
    }

  }

  return 0;
}

static int new_android_getaddrinfoforiface(const char *hostname, const char *servname,
                                           const struct addrinfo *hints, const char *iface,
                                           int mark,
                                           struct addrinfo **res) {
  LOGE("hahahha,wo hook dao l ->android_getaddrinforiface ");
  LOGE("下面是hostname");
  LOGE(hostname, "");
  if (hints->ai_flags == AI_NUMERICHOST) {
    if (fp_android_getaddrinfoforiface) {
      return fp_android_getaddrinfoforiface(hostname, servname, hints, iface, mark, res);
    }
  } else {
    const char *ip = getIpByHttpDns(hostname);
    if (ip != NULL) {
      LOGE("httpdns 解析成功，直接走IP");
      LOGE("下面是ip");
      LOGE(ip, "");
      //这里就比较神奇了，传的是IP，但是ai_flags 不是ip，但是还正常,查看源码的时候，没发现在这个代码里面判断
      // ai_flags 不numberhost还是其他
      addrinfo *newAddrinfo = new addrinfo;
      newAddrinfo->ai_addr = hints->ai_addr;
      newAddrinfo->ai_addrlen = hints->ai_addrlen;
      newAddrinfo->ai_canonname = hints->ai_canonname;
      newAddrinfo->ai_family = hints->ai_family;
      newAddrinfo->ai_next = hints->ai_next;
      newAddrinfo->ai_socktype = hints->ai_socktype;
      newAddrinfo->ai_protocol = hints->ai_protocol;
      newAddrinfo->ai_flags = AI_NUMERICHOST;
      return fp_android_getaddrinfoforiface(ip, servname, newAddrinfo, iface, mark, res);
    } else {
      return fp_android_getaddrinfoforiface(hostname, servname, hints, iface, mark, res);
    }

  }
  return 0;
}

// 5.0版本之前，android_getaddrinfofornet 叫android_getaddrinfoforiface
//int android_getaddrinfoforiface(const char *hostname, const char *servname,
//                           const struct addrinfo *hints, const char *iface, int mark, struct addrinfo **res)
extern "C" int hook_android_getaddrinfofornet() {
  if (fp_android_getaddrinfofornet) {
    return 0;
  }
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

extern "C" int hook_android_getaddrinfoforiface() {
  if (fp_android_getaddrinfofornet) {
    return 0;
  }
  int result = xhook_register(".*\\libjavacore.so$", "android_getaddrinfoforiface",
                              (void *) new_android_getaddrinfoforiface,
                              reinterpret_cast<void **>(&fp_android_getaddrinfoforiface));
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

jint hookForKitkat(JNIEnv *pEnv, jobject pJobject);

jint hookForL(JNIEnv *env, jobject instance);

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


extern "C"
JNIEXPORT jint JNICALL
Java_com_example_guolei_myapplication_MainActivity_nativeInit(JNIEnv *env, jobject instance,
                                                              jint version) {
  env->GetJavaVM(&sjavaVM);
  if (initMethod(env) == -1) {
    return -1;
  }
//    if (version >= 27) {
//        return -1;
//    }
  if (version >= 21) {
    return hookForL(env, instance);
  } else if (version >= 19) {
    return hookForKitkat(env, instance);
  }
  return -1;
}

jint hookForL(JNIEnv *env, jobject instance) {
  if (hook_android_getaddrinfofornet() != 0) {
    xhook_clear();
    return -1;
  }
  return hooj_libc_getaddrinfo(env, instance);
}

jint hookForKitkat(JNIEnv *env, jobject instance) {
  if (hook_android_getaddrinfoforiface() != 0) {
    xhook_clear();
    return -1;
  }
  return hooj_libc_getaddrinfo(env, instance);
}
