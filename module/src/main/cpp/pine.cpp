#include <dlfcn.h>
#include "pine.h"

bool pine_start(JNIEnv *env, bool isSystem){
/*
     * auto classloader = JNI_FindClass(env, "java/lang/ClassLoader");
    auto getsyscl_mid = JNI_GetStaticMethodID(
            env, classloader, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");
    auto sys_classloader = JNI_CallStaticObjectMethod(env, classloader, getsyscl_mid);
    if (!sys_classloader) [[unlikely]] {
        LOGE("getSystemClassLoader failed!!!");
        return;
    }
    auto in_memory_classloader = JNI_FindClass(env, "dalvik/system/InMemoryDexClassLoader");
    auto initMid = JNI_GetMethodID(env, in_memory_classloader, "<init>",
                                   "(Ljava/nio/ByteBuffer;Ljava/lang/ClassLoader;)V");
    auto byte_buffer_class = JNI_FindClass(env, "java/nio/ByteBuffer");
    auto dex_buffer = env->NewDirectByteBuffer(dex.data(), dex.size());
    if (auto my_cl = JNI_NewObject(env, in_memory_classloader, initMid,
                                   dex_buffer, sys_classloader)) {
        inject_class_loader_ = JNI_NewGlobalRef(env, my_cl);
    } else {
        LOGE("InMemoryDexClassLoader creation failed!!!");
        return;
    }
     */
    auto classloader = env->FindClass("java/lang/ClassLoader");
    auto getsyscl_mid = env->GetStaticMethodID(classloader, "getSystemClassLoader",
                                               "()Ljava/lang/ClassLoader;");
    auto sys_classloader = env->CallStaticObjectMethod(classloader, getsyscl_mid);
    if (!sys_classloader) {
        LOGD("getSystemClassLoader failed!!!");
        return false;
    }
    auto path_classloader = env->FindClass("dalvik/system/PathClassLoader");
    auto initMid = env->GetMethodID(path_classloader, "<init>",
                                    "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/ClassLoader;)V");
    auto modulePath = env->NewStringUTF("/system/app/testpine/testpine.apk");
#if defined(__arm__)
    auto libPath = env->NewStringUTF("/system/app/testpine/lib/armeabi-v7a");
#elif defined(__aarch64__)
    auto libPath = env->NewStringUTF("/system/app/testpine/lib/arm64-v8a");
#endif
    if (auto my_cl = env->NewObject(path_classloader, initMid, modulePath, libPath,
                                    sys_classloader)) {
        jmethodID loadClass_mid = env->GetMethodID(path_classloader, "loadClass",
                                                   "(Ljava/lang/String;)Ljava/lang/Class;");
        auto entry_class = static_cast<jclass>(env->CallObjectMethod(my_cl, loadClass_mid,
                                                                       env->NewStringUTF(
                                                                               "com.hexl.testpine.PineFramework")));
        if (!entry_class) {
            LOGD("entry class not loadClass");
            return false;
        } else {
            LOGD("entry class loadSuccess");
        }
        jmethodID init_mid = env->GetStaticMethodID(entry_class, "init", "(Z)V");
        if (init_mid) {
            env->CallStaticVoidMethod(entry_class, init_mid, isSystem);
            return true;
        } else {
            LOGD("method {init_mid} id is null");
        }
    }
    return false;
}

void setDebuggable(bool debug) {
    void *base =dlopen_compat("libart.so", RTLD_NOW);
    if (base == nullptr) {
        return;
    }
    void *addrInstance =dlsym_compat(base, "_ZN3art7Runtime9instance_E");
    if (addrInstance == nullptr) {
        return;
    }
    void **instance = reinterpret_cast<void **>(addrInstance);
    if (instance == nullptr || *instance == nullptr) {
        return;
    }
    void *pVoid =dlsym_compat(base, "_ZN3art7Runtime17SetJavaDebuggableEb");
    if (pVoid == nullptr) {
        return;
    }
    reinterpret_cast<void (*)(void *, bool)>((long) pVoid)(*instance, debug);
}