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
#if CF_ANDROID

#include <linux/limits.h>

#include <string>

#include "core-modules.hh"
#include "tiny-process-library/process.hpp"
#include "util.hpp"

using namespace TinyProcessLib;

MODFUNC(user_shell_path)
{
    char buf[PATH_MAX];
    return realpath(fmt::format("/proc/{}/exe", getppid()).c_str(), buf);
}

MODFUNC(user_shell_name)
{
    return user_shell_path(callbackInfo).substr(user_shell_path(callbackInfo).rfind('/') + 1);
}

MODFUNC(user_shell_version)
{
    const std::string& shell_name = user_shell_name(callbackInfo);
    std::string        ret;

    if (shell_name == "nu")
        Process("nu -c \"version | get version\"", "", [&](const char* bytes, size_t n) { ret.assign(bytes, n); });
    else
        Process(fmt::format("{} -c 'echo \"${}_VERSION\"'", shell_name, str_toupper(shell_name.data())), "",
                [&](const char* bytes, size_t n) { ret.assign(bytes, n); });

    strip(ret);
    return ret;
}

// clang-format off
MMODFUNC(user_name)
{ return g_pwd->pw_name; }

ODFUNC(user_term_name)
{ return "Termux"; }

MODFUNC(user_term_version)
{ return getenv("TERMUX_VERSION"); }

MODFUNC(user_wm_name)
{ return MAGIC_LINE; }

MODFUNC(user_wm_version)
{ return MAGIC_LINE; }

MODFUNC(user_de_name)
{ return MAGIC_LINE; }

MODFUNC(user_de_version)
{ return MAGIC_LINE; }

#endif
