/*
 * Copyright (c) 2021-2023 Linus Dierheimer
 * Copyright (c) 2022-2024 Carter Li
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "term.hh"

#include <fstream>

#include "fmt/format.h"
#include "util.hpp"

void get_term_version_exec(const std::string_view term, std::string& ret, bool _short, bool _stderr)
{
    ret.clear();
    read_exec({ term.data(), _short ? "-v" : "--version" }, ret, _stderr);
}

bool fast_detect_st_ver(std::string& ret)
{
    std::string   line;
    std::ifstream f(which("st"), std::ios::binary);

    while (read_binary_file(f, line))
    {
        if (line == "WINDOWID" && hasStart(ret, "%s "))
        {
            ret.erase(0, 3);
            return true;
        }

        ret = line;
    }
    debug("failed to fast detect st version");

    get_term_version_exec("st", ret, true, true);
    return false;
}

// https://github.com/fastfetch-cli/fastfetch/blob/dev/src/detection/terminalshell/terminalshell.c#L345
bool fast_detect_konsole_ver(std::string& ret)
{
    const char* env = std::getenv("KONSOLE_VERSION");
    if (env)
    {
        long major = strtol(env, NULL, 10);
        if (major >= 0)
        {
            long patch = major % 100;
            major /= 100;
            long minor = major % 100;
            major /= 100;
            ret = fmt::format("{}.{}.{}", major, minor, patch);
            return true;
        }
    }

    debug("failed to fast detect konsole version");
    get_term_version_exec("konsole", ret);

    return false;
}
