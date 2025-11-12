/*
 * Copyright 2025 Toni500git
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 * following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote
 * products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#pragma once

#include <cstdlib>

#include "fmt/core.h"

constexpr const char NOCOLOR[]      = "\033[0m";
constexpr const char NOCOLOR_BOLD[] = "\033[0m\033[1m";

// Didn't find what you were looking for.
constexpr const char UNKNOWN[]      = "(unknown)";

// Usually in neofetch/fastfetch when some infos couldn't be queried, they remove it from the display.
// With customfetch is kinda difficult to know when to remove the info to display,
// since it's all modular with tags, so I have created a "magic line" to be sure that I don't cut the wrong line.
//
// Every instance of this string found in a layout line, the whole line will be erased.
constexpr const char MAGIC_LINE[] = "(cut this line NOW!! RAHHH)";

#define APICALL extern "C"
#define EXPORT __attribute__((visibility("default")))
#define PLUGIN_INIT void start
#define PLUGIN_FINISH void finish

#if DEBUG
inline bool debug_print = true;
#else
inline bool debug_print = false;
#endif

// std::format function arguments
// Print to stderr an error with header 'ERROR:' in red
template <typename... Args>
void error(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::print(stderr, "\033[1;31mERROR: {}\033[0m\n",
               fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

// std::format function arguments
// Print to stderr an error with header 'FATAL:' in red and exit with failure code
template <typename... Args>
void die(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::print(stderr, "\033[1;31mFATAL: {}\033[0m\n",
               fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
    std::exit(EXIT_FAILURE);
}

// std::format function arguments
// Print to stdout a debug msg with header '[DEBUG]' in pink color
// only if customfetch is run with --debug=1
template <typename... Args>
void debug(const std::string_view fmt, Args&&... args) noexcept
{
    if (debug_print)
        fmt::print(stdout, "\033[1;35m[DEBUG]:\033[0m {}\n",
                   fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

// std::format function arguments
// Print to stderr a warning with header 'WARNING:' in yellow
template <typename... Args>
void warn(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::print(stderr, "\033[1;33mWARNING: {}\033[0m\n",
               fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

// std::format function arguments
// Print to stdout an info msg with header 'INFO:' in cyan
template <typename... Args>
void info(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::print(stdout, "\033[1;36mINFO: {}\033[0m\n",
               fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}
