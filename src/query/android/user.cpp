/*
 * Copyright 2024 Toni500git
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

#include "query.hpp"
#include "util.hpp"
#include "fmt/format.h"

#include <string>
#include <linux/limits.h>
#include <unistd.h>
#include <cstdlib>

using namespace Query;

static std::string get_shell_version(const std::string_view shell_name)
{
    std::string ret;

    if (shell_name == "nu")
        ret = read_shell_exec("nu -c \"version | get version\"");
    else
        ret = read_shell_exec(fmt::format("{} -c 'echo \"${}_VERSION\"'", shell_name, str_toupper(shell_name.data())));

    strip(ret);
    return ret;
}

static std::string get_shell_name(const std::string_view shell_path)
{
    return shell_path.substr(shell_path.rfind('/') + 1).data();
}

User::User() noexcept
{
    if (!m_bInit)
    {
        const uid_t uid = getuid();

        if (m_pPwd = getpwuid(uid), !m_pPwd)
            die("getpwent failed: {}\nCould not get user infos", std::strerror(errno));

        char buf[PATH_MAX];
        if (getenv("TERMUX_VERSION") || getenv("TERMUX_MAIN_PACKAGE_FORMAT"))
        {
            m_users_infos.shell_path = realpath(fmt::format("/proc/{}/exe", getppid()).c_str(), buf);
            m_users_infos.shell_name = get_shell_name(m_users_infos.shell_path);
            m_users_infos.shell_version = get_shell_version(m_users_infos.shell_name);
            m_users_infos.term_name = "Termux";
            m_users_infos.term_version = getenv("TERMUX_VERSION");
        }
        else
        {
            m_users_infos.shell_path = m_pPwd->pw_shell;
        }

        m_users_infos.wm_name = m_users_infos.wm_version = m_users_infos.de_name = m_users_infos.de_version = m_users_infos.m_wm_path = MAGIC_LINE;
    }
    m_bInit = true;

}

// clang-format off
std::string User::name() noexcept
{ return m_pPwd->pw_name; }

std::string User::shell_path() noexcept
{ return m_users_infos.shell_path; }

std::string& User::shell_name() noexcept
{ return m_users_infos.shell_name; }

std::string& User::shell_version(const std::string_view shell_name)
{ return m_users_infos.shell_version; }

std::string& User::term_name()
{ return m_users_infos.term_name; }

std::string& User::term_version(const std::string_view term_name)
{ return m_users_infos.term_version; }

std::string& User::wm_name(bool dont_query_dewm, const std::string_view term_name)
{ return m_users_infos.wm_name; }

std::string& User::wm_version(bool dont_query_dewm, const std::string_view term_name)
{ return m_users_infos.wm_version; }

std::string& User::de_name(bool dont_query_dewm, const std::string_view term_name, const std::string_view wm_name)
{ return m_users_infos.de_name; }

std::string& User::de_version(const std::string_view de_name)
{ return m_users_infos.de_version; }

#endif
