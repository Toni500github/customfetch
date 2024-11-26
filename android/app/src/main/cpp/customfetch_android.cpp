#include <jni.h>

#include <string>

#include "query.hpp"
#include "util.hpp"

extern "C" JNIEXPORT jstring JNICALL
Java_org_toni_customfetch_1android_widget_customfetch_idk(JNIEnv* env, jobject /* this */) {
    std::string hello = "Hello from JNI.";
    return env->NewStringUTF(hello.c_str());
}

extern "C"
JNIEXPORT jstring JNICALL
Java_org_toni_customfetch_1android_widget_customfetchConfigureActivity_idk(JNIEnv *env, jobject/* thiz*/) {
    Query::System system;
    return env->NewStringUTF(system.arch().c_str());
    //return Java_org_toni_customfetch_1android_widget_customfetch_idk(env, thiz);
}

char *argv[] = {"customfetch"};
int mainAndroid(int argc, char *argv[]);
extern "C"
JNIEXPORT jstring JNICALL
Java_org_toni_customfetch_1android_SettingsActivity_mainidk(JNIEnv *env, jobject/* thiz*/) {
    mainAndroid(1, argv);
    return env->NewStringUTF("test");
}