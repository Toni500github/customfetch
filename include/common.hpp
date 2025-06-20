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
