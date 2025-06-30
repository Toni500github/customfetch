#pragma once

#include "fmt/color.h"
#include "fmt/core.h"

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

#define APICALL extern "C"
#define EXPORT __attribute__((visibility("default")))
#define MOD_INIT void start
#define MOD_FINISH void finish

#define BOLD_COLOR(x) (fmt::emphasis::bold | fmt::fg(fmt::rgb(x)))

#if DEBUG
inline bool debug_print = true;
#else
inline bool debug_print = false;
#endif

template <typename... Args>
void error(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::print(stderr, BOLD_COLOR(fmt::color::red), "ERROR: {}\033[0m\n",
               fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

template <typename... Args>
void die(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::print(stderr, BOLD_COLOR(fmt::color::red), "FATAL: {}\033[0m\n",
               fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
    std::exit(1);
}

template <typename... Args>
void debug(const std::string_view fmt, Args&&... args) noexcept
{
    if (debug_print)
        fmt::print(BOLD_COLOR((fmt::color::hot_pink)), "[DEBUG]:\033[0m {}\n",
                   fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

template <typename... Args>
void warn(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::print(BOLD_COLOR((fmt::color::yellow)), "WARNING: {}\033[0m\n",
               fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

template <typename... Args>
void info(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::print(BOLD_COLOR((fmt::color::cyan)), "INFO: {}\033[0m\n",
               fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

#undef BOLD_COLOR
