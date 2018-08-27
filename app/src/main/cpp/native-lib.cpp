#define ANDROID_ARM_LINKER

#include <jni.h>
#include <string>

#include <unistd.h>
#include <dlfcn.h>
#include <android/log.h>

#include <netdb.h>

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <sys/atomics.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "linker.h"
#include "xhook.h"

#define TAG_NAME        "guolei"
#define log_error(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, TAG_NAME, (const char *) fmt, ##args)
#define log_info(fmt, args...) __android_log_print(ANDROID_LOG_INFO, TAG_NAME, (const char *) fmt, ##args)



extern "C" {
extern void *fake_dlopen(const char *filename, int flags);
extern void *fake_dlsym(void *handle, const char *symbol);
extern void fake_dlclose(void *handle);
}

static unsigned elfhash(const char *_name)
{
  const unsigned char *name = (const unsigned char *) _name;
  unsigned h = 0, g;
  while(*name) {
    h = (h << 4) + *name++;
    g = h & 0xf0000000;
    h ^= g;
    h ^= g >> 24;
  }
  return h;
}
static Elf32_Sym *soinfo_elf_lookup(soinfo *si, unsigned hash, const char *name)
{
  Elf32_Sym *s;
  Elf32_Sym *symtab = si->symtab;
  const char *strtab = si->strtab;
  unsigned n;
  n = hash % si->nbucket;
  for(n = si->bucket[hash % si->nbucket]; n != 0; n = si->chain[n]){
    s = symtab + n;
    if(strcmp(strtab + s->st_name, name)) continue;
    return s;
  }
  return NULL;
}
int hook_call(char *soname, char *symbol, unsigned newval) {
  soinfo *si = NULL;
  Elf32_Rel *rel = NULL;
  Elf32_Sym *s = NULL;
  unsigned int sym_offset = 0;
  if (!soname || !symbol || !newval)
    return 0;
  si = (soinfo*) fake_dlopen(soname, RTLD_NOW);
  if (!si)
    return 0;
  s = soinfo_elf_lookup(si, elfhash(symbol), symbol);
  if (!s)
    return 0;
  sym_offset = s - si->symtab;
  rel = si->plt_rel;
  /* walk through reloc table, find symbol index matching one we've got */
  for (int i = 0; i < si->plt_rel_count; i++, rel++) {
    unsigned type = ELF32_R_TYPE(rel->r_info);
    unsigned sym = ELF32_R_SYM(rel->r_info);
    unsigned reloc = (unsigned)(rel->r_offset + si->base);
    unsigned oldval = 0;
    if (sym_offset == sym) {
      switch(type) {
        case R_ARM_JUMP_SLOT:
          /* we do not have to read original value, but it would be good
            idea to make sure it contains what we are looking for */
//          size_t pagesize = sysconf(_SC_PAGESIZE);
//          const void* aligned_pointer = (const void*)(reloc & ~(pagesize - 1));
//          mprotect(aligned_pointer, pagesize, PROT_WRITE | PROT_READ);
          oldval = *(unsigned*) reloc;
          *((unsigned*)reloc) = newval;
//          mprotect(aligned_pointer, pagesize, PROT_READ);
          return 1;
        default:
          return 0;
      }
    }
  }
  return 0;
}


#if 0

The function below loads Android "libutils.so" to run the following c++ code:

    const char *str = "Hello, world!";
    android::String8 *str8 = new android::String8(str);
    size_t len = str8->getUtf32Length();
    log_info("%s: length = %d", str, (int) len);
    delete str8;

#endif

extern "C" int test_cplusplus() {
  log_info("testing libutils.so");
#ifdef __arm__
  void *handle = fake_dlopen("/system/lib/libutils.so", RTLD_NOW);
#elif defined(__aarch64__)
  void *handle = fake_dlopen("/system/lib64/libutils.so", RTLD_NOW);
#else
#error "Arch unknown, please port me"
#endif

  if (!handle) return log_error("cannot load libutils.so\n");

  // Constructor:  android::String8::String8(char const*)
  // The first argument is a pointer where "this" of a new object is to be stored.

  void (*create_string)(void **, const char *) =
  (typeof(create_string)) fake_dlsym(handle, "_ZN7android7String8C1EPKc");

  // Member function:  size_t android::String8::getUtf32Length() const
  // The argument is a pointer to "this" of the object

  size_t (*get_len)(void **) =
  (typeof(get_len)) fake_dlsym(handle, "_ZNK7android7String814getUtf32LengthEv");

  // Destructor:  android::String8::~String8()

  void (*delete_string)(void **) =
  (typeof(delete_string)) fake_dlsym(handle, "_ZN7android7String8D1Ev");

  // All required library addresses known now, so don't need its handle anymore

  fake_dlclose(handle);

  if (!create_string || !get_len || !delete_string)
    return log_error("required functions missing in libutils.so\n");

  // Fire up now.

  void *str8 = 0;
  const char *str = "Hello, world!";

  create_string(&str8, str);
  if (!str8) return log_error("failed to create string\n");

  size_t len = get_len(&str8);
  log_info("%s: length = %d", str, (int) len);

  delete_string(&str8);

  return 0;

}

int new_getaddrinfo(const char *hostname, const char *servname,
    const struct addinfo *hints, struct addrinfo **res) {
  log_error("hahahha,wo hook dao l ");
  return 0;
}

static int new_android_getaddrinfofornet(const char *hostname, const char *servname,
                          const struct addrinfo *hints, unsigned netid, unsigned mark, struct addrinfo **res)
{
  log_error("hahahha,wo hook dao l ->android_getaddrinfofornet ");
  return 0;
}


extern "C" int test_libc() {
  return 0;
}

extern "C" int hook_libc(){
#ifdef __arm__
  xhook_register("/system/lib/libc.so",  "getaddrinfo", (void *)new_getaddrinfo,  NULL);
//  xhook_register("/system/lib/libjavacore.so",  "android_getaddrinfofornet", (void *)new_android_getaddrinfofornet,  NULL);
#elif defined(__aarch64__)
  xhook_register("/system/lib64/libc.so",  "getaddrinfo", (void *)new_getaddrinfo,  NULL);
//  xhook_register("/system/lib64/libjavacore.so",  "android_getaddrinfofornet", (void *)new_android_getaddrinfofornet,  NULL);
#else
#error "Arch unknown, please port me"
#endif
  xhook_refresh(1);
  return 0;
}

extern "C" JNIEXPORT jstring
JNICALL
Java_com_example_guolei_myapplication_MainActivity_stringFromJNI(
    JNIEnv *env,
    jobject /* this */) {
  std::string hello = "Hello from C++";
  test_cplusplus();
  hook_libc();
  return env->NewStringUTF(hello.c_str());
}