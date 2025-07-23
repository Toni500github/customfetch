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

#include "dewm.hh"

#include <cstdlib>
#include <fstream>

#include "libcufetch/common.hh"
#include "rapidxml-1.13/rapidxml.hpp"
#include "switch_fnv1a.hpp"
#include "util.hpp"

// https://github.com/fastfetch-cli/fastfetch/blob/a61765c8b1387777be67d967bc2f69031c8ca399/src/detection/displayserver/linux/wmde.c#L19
std::string parse_de_env(void) noexcept
{
    const char* env;

    env = std::getenv("XDG_CURRENT_DESKTOP");
    if (env != NULL && env[0] != '\0')
        return env;

    // maybe let's try to get the DE envs first, then try with the others
    if (std::getenv("KDE_FULL_SESSION") != NULL || std::getenv("KDE_SESSION_UID") != NULL ||
        std::getenv("KDE_SESSION_VERSION") != NULL)
        return "KDE";

    if (std::getenv("CINNAMON_VERSION") != NULL)
        return "Cinnamon";

    if (std::getenv("GNOME_DESKTOP_SESSION_ID") != NULL || std::getenv("GNOME_SETUP_DISPLAY") != NULL)
        return "GNOME";

    if (std::getenv("MATE_DESKTOP_SESSION_ID") != NULL)
        return "Mate";

    if (std::getenv("TDE_FULL_SESSION") != NULL)
        return "Trinity";

    if (std::getenv("HYPRLAND_CMD") != NULL)
        return "Hyprland";

    env = std::getenv("XDG_SESSION_DESKTOP");
    if (env != NULL && env[0] != '\0')
        return env;

    env = std::getenv("CURRENT_DESKTOP");
    if (env != NULL && env[0] != '\0')
        return env;

    env = std::getenv("SESSION_DESKTOP");
    if (env != NULL && env[0] != '\0')
        return env;

    env = std::getenv("DESKTOP_SESSION");
    if (env != NULL && env[0] != '\0')
        return env;

    return MAGIC_LINE;
}

std::string prettify_wm_name(const std::string_view name) noexcept
{
    if (hasStart(name, "kwin"))
        return "Kwin";

    // taken from this list
    // https://github.com/KittyKatt/screenFetch/blob/master/screenfetch-dev#L93
    // added some missing.
    // ngl this looks beatiful thanks to clang-format :D
    switch (fnv1a16::hash(str_tolower(name.data())))
    {
        case "cinnamon"_fnv1a16: return "Muffin";
        case "hyprland"_fnv1a16: return "Hyprland";
        case "kwin"_fnv1a16:     return "Kwin";
        case "marco"_fnv1a16:    return "Marco";
        case "muffin"_fnv1a16:   return "Muffin";
        case "metacity"_fnv1a16: return "Metacity";
        case "openbox"_fnv1a16:  return "Openbox";
        case "qtile"_fnv1a16:    return "Qtile";
        case "xfwm4"_fnv1a16:    return "Xfwm4";
        case "xmonad"_fnv1a16:   return "XMonad";

        case "gnome-shell"_fnv1a16:
        case "gnome-session-binary"_fnv1a16:
        case "mutter"_fnv1a16:               return "Mutter";

        case "2bwm"_fnv1a16:
        case "9wm"_fnv1a16:
        case "awesome"_fnv1a16:
        case "beryl"_fnv1a16:
        case "blackbox"_fnv1a16:
        case "bspwm"_fnv1a16:
        case "budgie-wm"_fnv1a16:
        case "chromeos-wm"_fnv1a16:
        case "compiz"_fnv1a16:
        case "deepin-wm"_fnv1a16:
        case "dminiwm"_fnv1a16:
        case "dtwm"_fnv1a16:
        case "dwm"_fnv1a16:
        case "e16"_fnv1a16:
        case "echinus"_fnv1a16:
        case "emerald"_fnv1a16:
        case "enlightenment"_fnv1a16:
        case "finder"_fnv1a16:
        case "fluxbox"_fnv1a16:
        case "flwm"_fnv1a16:
        case "flwm_topside"_fnv1a16:
        case "fvwm"_fnv1a16:
        case "herbstluftwm"_fnv1a16:
        case "howm"_fnv1a16:
        case "i3"_fnv1a16:
        case "i3wm"_fnv1a16:
        case "icewm"_fnv1a16:
        case "monsterwm"_fnv1a16:
        case "musca"_fnv1a16:
        case "mwm"_fnv1a16:
        case "notion"_fnv1a16:
        case "pekwm"_fnv1a16:
        case "ratpoison"_fnv1a16:
        case "sawfish"_fnv1a16:
        case "scrotwm"_fnv1a16:
        case "spectrwm"_fnv1a16:
        case "stumpwm"_fnv1a16:
        case "subtle"_fnv1a16:
        case "sway"_fnv1a16:
        case "swm"_fnv1a16:
        case "tinywm"_fnv1a16:
        case "twin"_fnv1a16:
        case "wayfire"_fnv1a16:
        case "weston"_fnv1a16:
        case "wmaker"_fnv1a16:
        case "wmfs"_fnv1a16:
        case "wmii"_fnv1a16:          return name.data();
    }

    return MAGIC_LINE;
}

std::string get_mate_version()
{
    std::ifstream f(get_data_path("mate-about/mate-version.xml"), std::ios::in);
    if (!f.is_open())
    {
        std::string ret;
        read_exec({ "mate-session", "--version" }, ret);

        ret.erase(0, ret.rfind(' ') + 1);
        return ret;
    }

    std::string buffer(std::istreambuf_iterator<char>{ f }, std::istreambuf_iterator<char>{});
    buffer.push_back('\0');

    rapidxml::xml_document<> doc;
    doc.parse<0>(&buffer[0]);
    rapidxml::xml_node<>* root_node = doc.first_node("mate-version");
    if (!root_node)
    {
        error(_("Root node <mate-version> not found"));
        return UNKNOWN;
    }

    const std::string_view major = root_node->first_node("platform")->value();
    const std::string_view minor = root_node->first_node("minor")->value();
    const std::string_view micro = root_node->first_node("micro")->value();

    return fmt::format("{}.{}.{}", major, minor, micro);
}

std::string get_kwin_version()
{
    std::string ret;

    if (std::getenv("WAYLAND_DISPLAY") != NULL)
        read_exec({ "kwin_wayland", "--version" }, ret);
    else
        read_exec({ "kwin_x11", "--version" }, ret);

    ret.erase(0, ret.rfind(' ') + 1);
    return ret;
}

static std::string get_cinnamon_version_binary()
{
    std::ifstream f(which("cinnamon"), std::ios::binary);
    if (!f.is_open())
        return UNKNOWN;

    std::string line, ret;
    while (read_binary_file(f, line))
    {
        // if you run `strings $(which cinnamon)`
        // and then analyze every string, you'll see there is a string with
        // "Cinnamon %s" and above it's version
        // so let's do it
        if (line == "Cinnamon %s")
            return ret;

        // save the above position
        ret = line;
    }

    return UNKNOWN;
}

std::string get_cinnamon_version()
{
    const char* env = std::getenv("CINNAMON_VERSION");
    if (env != nullptr && env[0] != '\0')
        return env;

    std::string   line;
    std::ifstream f(get_data_path("applications/cinnamon.desktop"), std::ios::in);
    if (!f.is_open())
    {
        std::string ret = get_cinnamon_version_binary();
        if (ret != UNKNOWN)
            return ret;

        ret.clear();
        read_exec({ "cinnamon", "--version" }, ret);
        ret.erase(0, "Cinnamon "_len);
        return ret;
    }

    while (std::getline(f, line))
    {
        if (hasStart(line, "X-GNOME-Bugzilla-Version="))
            return line.substr("X-GNOME-Bugzilla-Version="_len);
    }

    return "";
}

static std::string get_xfce4_version_lib()
{
    void* handle = LOAD_LIBRARY("libxfce4util.so");
    if (!handle)
        return UNKNOWN;

    LOAD_LIB_SYMBOL(handle, const char*, xfce_version_string, void)
    const std::string& ret = xfce_version_string();
    UNLOAD_LIBRARY(handle)
    return ret;
}

std::string get_xfce4_version()
{
    std::string ret = get_xfce4_version_lib();
    if (ret != UNKNOWN)
        return ret;

    read_exec({ "xfce4-session", "--version" }, ret);
    ret.erase(0, "xfce4-session"_len + 1);
    ret.erase(ret.find(' '));
    return ret;
}

bool get_fast_xfwm4_version(std::string& ret, const std::string& exec_path)
{
    std::ifstream f(exec_path, std::ios::binary);
    if (!f.is_open())
        return false;

    std::string line;
    while (read_binary_file(f, line))
    {
        if (line == "using GTK+-%d.%d.%d.")
            return true;

        // previous line, which will eventually be the version
        ret = line;
    }

    return false;
}
