#pragma once

#include <functional>
#include <string>
#include <vector>
#include <fmt/core.h>
#include <fmt/color.h>

#if ENABLE_NLS && !CF_MACOS
/* here so it doesn't need to be included elsewhere */
#include <libintl.h>
#include <locale.h>
#define _(str) gettext(str)
#else
#define _(s) (char*)s
#endif

#define BOLD_COLOR(x) (fmt::emphasis::bold | fmt::fg(x))

constexpr const char NOCOLOR[] = "\033[0m";
constexpr const char NOCOLOR_BOLD[] = "\033[0m\033[1m";
constexpr const char UNKNOWN[] = "(unknown)";

// Usually in neofetch/fastfetch when some infos couldn't be queried,
// they remove it from the display. With customfetch is kinda difficult to know when to remove
// the info to display, since it's all modular with tags, so I have created
// magic line to be sure that I don't cut the wrong line.
//
// Every instance of this string in a layout line, the whole line will be erased.
constexpr const char MAGIC_LINE[] = "(cut this line NOW!! RAHHH)";

/* handle   = the library handle
 * ret_type = type of what the function returns
 * func     = the function name
 * ...      = the arguments in a function if any
 */
#define LOAD_LIB_SYMBOL(handle, ret_type, func, ...)   \
    typedef ret_type (*func##_t)(__VA_ARGS__); \
    func##_t func = reinterpret_cast<func##_t>(dlsym(handle, #func));

#define UNLOAD_LIBRARY(handle) dlclose(handle);

#define APICALL extern "C"
// {fmt} library already has __attribute__((visibility(value))) fallback so let's use that maybeAdd commentMore actions
#define EXPORT FMT_VISIBILITY("default") void
#define MOD_INIT start

using modfunc = const std::string;

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

struct module_t
{
    std::string           name;
    std::vector<module_t> submodules; /* For best performance, use std::move() when adding modules in here. */
    std::function<const std::string(void)> handler;

    module_t(const std::string& name, const std::vector<module_t>& submodules,
             const std::function<const std::string(void)> handler)
        : name(name), submodules(submodules), handler(handler)
    {
    }
};
