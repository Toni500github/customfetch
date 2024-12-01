#include <jni.h>

#include <string>

#include "query.hpp"
#include "util.hpp"

#define ARRAY_SIZE(x) sizeof(x) / sizeof(x[0])

extern "C" JNIEXPORT jstring JNICALL
Java_org_toni_customfetch_1android_widget_customfetch_idk(JNIEnv* env, jobject /* this */) {
    std::string hello = "Hello from JNI.";
    return env->NewStringUTF(hello.c_str());
}

extern std::string mainAndroid_and_render(int argc, char *argv[], JNIEnv *env, jobject obj);

extern "C" JNIEXPORT jstring JNICALL
Java_org_toni_customfetch_1android_widget_customfetchConfigureActivity_mainidk(JNIEnv *env, jobject obj) {
    char *argv[] = {"customfetch", "-D", "/storage/emulated/0/Documents/customfetch" };
    return env->NewStringUTF(mainAndroid_and_render(ARRAY_SIZE(argv), argv, env, obj).c_str());
}