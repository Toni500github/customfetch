/*
 * Copyright 2024 Toni500git
 * 
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _UTILS_HPP
#define _UTILS_HPP

#include <dlfcn.h>
#include <sys/types.h>

#include <iostream>
#include <string>
#include <vector>

#include "fmt/color.h"
#include "fmt/base.h"
#include "platform.hpp"

// clang-format off
constexpr std::size_t operator""_len(const char*, std::size_t ln) noexcept
{ 
    return ln;
}

struct byte_units_t
{
    std::string unit;
    double      num_bytes;
};

constexpr const char NOCOLOR[] = "\033[0m";
constexpr const char NOCOLOR_BOLD[] = "\033[0m\033[1m";
constexpr const char UNKNOWN[] = "(unknown)";

// magic line to be sure that I don't cut the wrong line
constexpr const char MAGIC_LINE[] = "(cut this shit NOW!! RAHHH)";

/* lib  = library to load (string)
 * code = code to execute if anything goes wrong
 */
#define LOAD_LIBRARY(lib, code)            \
    void* handle = dlopen(lib, RTLD_LAZY); \
    if (!handle)                           \
        code;

/* ret_type = type of what the function returns
 * func     = the function name
 * ...      = the arguments in a function if any
 */
#define LOAD_LIB_SYMBOL(ret_type, func, ...)   \
    typedef ret_type (*func##_t)(__VA_ARGS__); \
    func##_t func = reinterpret_cast<func##_t>(dlsym(handle, #func));

#define UNLOAD_LIBRARY() dlclose(handle);

#define BOLD_COLOR(x) (fmt::emphasis::bold | fmt::fg(x))

bool         hasEnding(const std::string_view fullString, const std::string_view ending);
bool         hasStart(const std::string_view fullString, const std::string_view start);
std::string  name_from_entry(size_t dev_entry_pos);
std::string  vendor_from_entry(size_t vendor_entry_pos, const std::string_view vendor_id);
std::string  binarySearchPCIArray(const std::string_view vendor_id, const std::string_view pci_id);
std::string  binarySearchPCIArray(const std::string_view vendor_id);
std::string  read_shell_exec(const std::string_view cmd);
void         getFileValue(u_short& iterIndex, const std::string_view line, std::string& str, const size_t& amount);
byte_units_t auto_devide_bytes(const double num, const std::uint16_t base, const std::string_view maxprefix = "");
byte_units_t devide_bytes(const double num, const std::string_view prefix);
bool         is_file_image(const unsigned char* bytes);
void         ctrl_d_handler(const std::istream& cin);
std::string  expandVar(std::string ret);
bool         taur_exec(const std::vector<std::string_view> cmd_str, const bool noerror_print = true);
std::string  which(const std::string_view command);
std::string  get_data_path(const std::string_view file);
std::string  get_data_dir(const std::string_view dir);
std::string  get_relative_path(const std::string_view relative_path, const std::string_view _env, const long long mode);
bool         read_binary_file(std::ifstream& f, std::string& ret);
void         replace_str(std::string& str, const std::string_view from, const std::string_view to);
bool         read_exec(std::vector<const char*> cmd, std::string& output, bool useStdErr = false, bool noerror_print = true);
std::string  str_tolower(std::string str);
std::string  str_toupper(std::string str);
void         strip(std::string& input);
std::string  read_by_syspath(const std::string_view path);
fmt::rgb     hexStringToColor(const std::string_view hexstr);
std::string  shorten_vendor_name(std::string vendor);
std::string  getHomeConfigDir();
std::string  getConfigDir();
std::vector<std::string> split(const std::string_view text, char delim);

#if CF_ANDROID
std::string get_android_property(const std::string_view name);
#endif

#if ANDROID_APP
#include "jni.h"
#include "android/log.h"

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "JNI_TOAST", __VA_ARGS__)

inline struct jni_objects
{
    JNIEnv *env;
    jobject obj;
} jni_objs;

static constexpr const char *APPNAME = "customfetch_android";

template <typename... Args>
static void nativeLog(JNIEnv *env, jobject obj, int log_level, const std::string_view fmt, Args&&... args)
{
    jstring jMessage = env->NewStringUTF(fmt::format(fmt::runtime(fmt), args...).c_str());
    const char *cMessage = env->GetStringUTFChars(jMessage, nullptr);

    // Use Android's native logging
    __android_log_print(log_level, APPNAME, "%s", cMessage);

    env->ReleaseStringUTFChars(jMessage, cMessage);  // Clean up
}

template <typename... Args>
void error(const std::string_view fmt, Args&&... args) noexcept
{
    const std::string& fmt_str = fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...);
    nativeLog(jni_objs.env, jni_objs.obj, ANDROID_LOG_ERROR, "{}", fmt_str);
}

template <typename... Args>
void die(const std::string_view fmt, Args&&... args) noexcept
{
    const std::string& fmt_str = fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...);
    nativeLog(jni_objs.env, jni_objs.obj, ANDROID_LOG_FATAL, "{}", fmt_str);
    abort();
}

template <typename... Args>
void debug(const std::string_view fmt, Args&&... args) noexcept
{
#if DEBUG
    const std::string& fmt_str = fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...);
    nativeLog(jni_objs.env, jni_objs.obj, ANDROID_LOG_DEBUG, "{}", fmt_str);
#endif
}

template <typename... Args>
void warn(const std::string_view fmt, Args&&... args) noexcept
{
    const std::string& fmt_str = fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...);
    nativeLog(jni_objs.env, jni_objs.obj, ANDROID_LOG_WARN, "{}", fmt_str);
}

template <typename... Args>
void info(const std::string_view fmt, Args&&... args) noexcept
{
    const std::string& fmt_str = fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...);
    nativeLog(jni_objs.env, jni_objs.obj, ANDROID_LOG_INFO, "{}", fmt_str);
}

#else
template <typename... Args>
void error(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::print(stderr, BOLD_COLOR(fmt::rgb(fmt::color::red)), "ERROR: {}\n",
                 fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

template <typename... Args>
void die(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::print(stderr, BOLD_COLOR(fmt::rgb(fmt::color::red)), "FATAL: {}\n",
                 fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
    std::exit(1);
}

template <typename... Args>
void debug(const std::string_view fmt, Args&&... args) noexcept
{
#if DEBUG
    fmt::print(BOLD_COLOR((fmt::rgb(fmt::color::hot_pink))), "[DEBUG]: {}\n",
                 fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
#endif
}

template <typename... Args>
void warn(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::print(BOLD_COLOR((fmt::rgb(fmt::color::yellow))), "WARNING: {}\n",
                 fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

template <typename... Args>
void info(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::print(BOLD_COLOR((fmt::rgb(fmt::color::cyan))), "INFO: {}\n",
                 fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}
#endif

/** Ask the user a yes or no question.
 * @param def The default result
 * @param fmt The format string
 * @param args Arguments in the format
 * @returns the result, y = true, n = false, only returns def if the result is def
 */
template <typename... Args>
bool askUserYorN(bool def, const std::string_view fmt, Args&&... args)
{
    const std::string& inputs_str = fmt::format(" [{}]: ", def ? "Y/n" : "y/N");
    std::string result;
    fmt::print(fmt::runtime(fmt), std::forward<Args>(args)...);
    fmt::print("{}", inputs_str);

    while (std::getline(std::cin, result) && (result.length() > 1))
        fmt::print(BOLD_COLOR(fmt::rgb(fmt::color::yellow)), "Please answear y or n,{}", inputs_str);

    ctrl_d_handler(std::cin);

    if (result.empty())
        return def;

    if (def ? std::tolower(result[0]) != 'n' : std::tolower(result[0]) != 'y')
        return def;

    return !def;
}

#endif
