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

extern "C"
JNIEXPORT jstring JNICALL
Java_org_toni_customfetch_1android_widget_customfetchConfigureActivity_idk(JNIEnv *env, jobject/* thiz*/) {
    Query::CPU system;
    return env->NewStringUTF(system.name().c_str());
    //return Java_org_toni_customfetch_1android_widget_customfetch_idk(env, thiz);
}


char *argv[] = {"customfetch", "-m", "${auto}Hello ${red}from $<os.name>", "-D", "/storage/emulated/0/Documents/customfetch/" };
std::string mainAndroid_and_render(int argc, char *argv[]);
extern "C"
JNIEXPORT jstring JNICALL
Java_org_toni_customfetch_1android_widget_customfetchConfigureActivity_mainidk(JNIEnv *env, jobject/* thiz*/) {
    return env->NewStringUTF(mainAndroid_and_render(ARRAY_SIZE(argv), argv).c_str());
}