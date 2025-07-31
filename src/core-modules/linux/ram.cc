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

#include "platform.hpp"
#if CF_LINUX || CF_ANDROID

#include <cstdio>
#include <string>
#include <string_view>

#include "core-modules.hh"
#include "libcufetch/common.hh"

static double read_value(const std::string_view key)
{
    if (!meminfo)
        return 0.0;

    std::string result{ UNKNOWN };
    char*       line = nullptr;
    size_t      len  = 0;

    while (getline(&line, &len, meminfo) != -1)
    {
        if (strncmp(line, key.data(), key.length()) != 0)
            continue;

        // Skip colon and whitespace
        char* value = line + key.length();
        while (isspace(*value))
            value++;

        // Find end of numeric value (stop at first non-digit or '.')
        char* end = value;
        while (*end && (isdigit(*end) || *end == '.'))
            end++;

        if (value != end)
            result.assign(value, end - value);
        break;
    }

    free(line);
    rewind(meminfo);
    return std::stod(result) * 1024.0f;
}

// clang-format off
double ram_free()
{ return read_value("MemAvailable:"); }

double ram_total()
{ return read_value("MemTotal:"); }

double ram_used()
{ return ram_total() - ram_free(); }

double swap_free()
{ return read_value("SwapFree:"); }

double swap_total()
{ return read_value("SwapTotal:"); }

double swap_used()
{ return swap_total() - swap_free(); }

#endif
