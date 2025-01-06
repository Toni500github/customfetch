#include <jni.h>
#include <string>
#include <sstream>
#include "util.hpp"

#define ARRAY_SIZE(x) sizeof(x) / sizeof(x[0])

extern std::string mainAndroid_and_render(int argc, char *argv[], JNIEnv *env, jobject obj, bool do_not_load_config);

extern "C" JNIEXPORT jstring JNICALL
Java_org_toni_customfetch_1android_widget_CustomfetchMainRender_mainAndroid(JNIEnv *env, jobject obj, jstring args, jboolean do_not_load_config) {
    const std::string& str_args = env->GetStringUTFChars(args, nullptr);
    const std::vector<std::string>& tokens = split(str_args, ' ');
    char *argv[tokens.size()];
    for (size_t i = 0; i < tokens.size(); ++i)
        argv[i] = strdup(tokens[i].c_str());

    return env->NewStringUTF(mainAndroid_and_render(ARRAY_SIZE(argv), argv, env, obj, do_not_load_config).c_str());
}