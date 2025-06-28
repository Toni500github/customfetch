#pragma once

#include "fmt/color.h"
#include "fmt/core.h"

#include <functional>
#include <string>
#include <vector>

constexpr const char NOCOLOR[]      = "\033[0m";
constexpr const char NOCOLOR_BOLD[] = "\033[0m\033[1m";
constexpr const char UNKNOWN[]      = "(unknown)";

// Usually in neofetch/fastfetch when some infos couldn't be queried,
// they remove it from the display. With customfetch is kinda difficult to know when to remove
// the info to display, since it's all modular with tags, so I have created
// magic line to be sure that I don't cut the wrong line.
//
// Every instance of this string in a layout line, the whole line will be erased.
constexpr const char MAGIC_LINE[] = "(cut this line NOW!! RAHHH)";

/* handler  = the library handle
 * ret_type = type of what the function returns
 * func     = the function name
 * ...      = the arguments in a function if any
 */
#define LOAD_LIB_SYMBOL(handler, ret_type, func, ...) \
    typedef ret_type (*func##_t)(__VA_ARGS__);        \
    func##_t func = reinterpret_cast<func##_t>(dlsym(handler, #func));

#define UNLOAD_LIBRARY(handle) dlclose(handle);

#define APICALL extern "C"
// {fmt} library already has __attribute__((visibility(value))) fallback so let's use that maybeAdd commentMore actions
#define EXPORT FMT_VISIBILITY("default")
#define MOD_INIT void start

#define BOLD_COLOR(x) (fmt::emphasis::bold | fmt::fg(x))

template <typename... Args>
void error(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::print(stderr, BOLD_COLOR(fmt::rgb(fmt::color::red)), "ERROR: {}\033[0m\n",
               fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

template <typename... Args>
void die(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::print(stderr, BOLD_COLOR(fmt::rgb(fmt::color::red)), "FATAL: {}\033[0m\n",
               fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
    std::exit(1);
}

#if DEBUG
inline bool debug_print = true;
#else
inline bool debug_print = false;
#endif

template <typename... Args>
void debug(const std::string_view fmt, Args&&... args) noexcept
{
    if (debug_print)
        fmt::print(BOLD_COLOR((fmt::rgb(fmt::color::hot_pink))), "[DEBUG]:\033[0m {}\n",
                   fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

template <typename... Args>
void warn(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::print(BOLD_COLOR((fmt::rgb(fmt::color::yellow))), "WARNING: {}\033[0m\n",
               fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

template <typename... Args>
void info(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::print(BOLD_COLOR((fmt::rgb(fmt::color::cyan))), "INFO: {}\033[0m\n",
               fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

#undef BOLD_COLOR

/* A linked list including module arguments. An argument may be specified for any part of the module path (e.g.
 * `disk(/).used(GiB)`, `test(a).hi`) */
struct moduleArgs_t
{
    struct moduleArgs_t* prev = nullptr;

    std::string name;
    std::string value;

    struct moduleArgs_t* next = nullptr;
};

struct callbackInfo_t;
struct module_t
{
    std::string           name;
    std::string           description;
    std::vector<module_t> submodules; /* For best performance, use std::move() when adding modules in here. */
    std::function<const std::string(const callbackInfo_t*)> handler;
};

// Map from a modules name to its pointer.
using moduleMap_t = std::unordered_map<std::string, const module_t&>;
class Config;

const std::string parse(const std::string& input, const moduleMap_t& modulesInfo, const Config& config);
const std::string get_and_color_percentage(const float n1, const float n2, const callbackInfo_t* callback, const bool invert);

struct callbackInfo_t
{
    const moduleArgs_t* moduleArgs;
    const moduleMap_t&  modulesInfo;
    const Config&       config;
};
