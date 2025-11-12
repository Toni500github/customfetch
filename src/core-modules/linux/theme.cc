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
#if CF_LINUX

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <functional>

#include "core-modules.hh"
#include "fmt/format.h"
#include "rapidxml-1.13/rapidxml.hpp"
#include "switch_fnv1a.hpp"
#include "util.hpp"

#if USE_DCONF
#include <client/dconf-client.h>
#include <glib/gvariant.h>
#endif

using ThemeInfo  = std::array<std::string, 3>;  // [theme, icon_theme, font]
using CursorInfo = std::array<std::string, 2>;  // [name, size]

enum
{
    THEME_NAME,
    THEME_ICON,
    THEME_FONT
};

enum
{
    CURSOR_NAME,
    CURSOR_SIZE
};

const std::string& configDir          = getHomeConfigDir();
const std::string  gsetting_interface = (user_de_name(NULL) == "cinnamon") ? "org.cinnamon.desktop.interface"
                                        : (de_name == "mate")              ? "org.mate.interface"
                                                                           : "org.gnome.desktop.interface";

const std::string dconf_interface = (user_de_name(NULL) == "cinnamon") ? "/org/cinnamon/desktop/interface"
                                    : (de_name == "mate")              ? "/org/mate/interface"
                                                                       : "/org/gnome/desktop/interface";

static std::string get_xsettings_xfce4(const std::string_view property, const std::string_view subproperty)
{
    static bool                     done = false;
    static rapidxml::xml_document<> doc;
    static std::string              buffer;

    if (!done)
    {
        const std::string& path = configDir + "/xfce4/xfconf/xfce-perchannel-xml/xsettings.xml";
        std::ifstream      f(path, std::ios::in);
        if (!f.is_open())
            return MAGIC_LINE;

        buffer.assign(std::istreambuf_iterator<char>{ f }, std::istreambuf_iterator<char>());
        buffer.push_back('\0');

        doc.parse<0>(&buffer[0]);
        done = true;
    }

    rapidxml::xml_node<>* node1 = doc.first_node("channel")->first_node("property");
    for (; node1 && std::string_view(node1->first_attribute("name")->value()) != property;
         node1 = node1->next_sibling("property"))
        ;

    rapidxml::xml_node<>* node2 = node1->first_node("property");
    for (; node2; node2 = node2->next_sibling())
    {
        if (std::string_view(node2->first_attribute("name")->value()) == subproperty && node2->first_attribute("value"))
            return node2->first_attribute("value")->value();
    }

    return MAGIC_LINE;
}

static std::string get_auto_gtk_format(const std::string_view gtk2, const std::string_view gtk3,
                                       const std::string_view gtk4)
{
    if ((gtk2 != MAGIC_LINE && gtk3 != MAGIC_LINE && gtk4 != MAGIC_LINE))
    {
        if (gtk2 == gtk3 && gtk2 == gtk4)
            return fmt::format("{} [GTK2/3/4]", gtk4);
        else if (gtk2 == gtk3)
            return fmt::format("{} [GTK2/3], {} [GTK4]", gtk2, gtk4);
        else if (gtk4 == gtk3)
            return fmt::format("{} [GTK2], {} [GTK3/4]", gtk2, gtk4);
        else
            return fmt::format("{} [GTK2], {} [GTK3], {} [GTK4]", gtk2, gtk3, gtk4);
    }

    else if (gtk3 != MAGIC_LINE && gtk4 != MAGIC_LINE)
    {
        if (gtk3 == gtk4)
            return fmt::format("{} [GTK3/4]", gtk4);
        else
            return fmt::format("{} [GTK3], {} [GTK4]", gtk3, gtk4);
    }

    else if (gtk2 != MAGIC_LINE && gtk3 != MAGIC_LINE)
    {
        if (gtk2 == gtk3)
            return fmt::format("{} [GTK2/3]", gtk3);
        else
            return fmt::format("{} [GTK2], {} [GTK3]", gtk2, gtk3);
    }

    else if (gtk4 != MAGIC_LINE)
        return fmt::format("{} [GTK4]", gtk4);
    else if (gtk3 != MAGIC_LINE)
        return fmt::format("{} [GTK3]", gtk3);
    else if (gtk2 != MAGIC_LINE)
        return fmt::format("{} [GTK2]", gtk2);

    return MAGIC_LINE;
}

//
//
// 1. Cursor
//
static CursorInfo get_cursor_xresources()
{
    std::string        cursor_name{ MAGIC_LINE }, cursor_size{ MAGIC_LINE };
    const std::string& path = expandVar("~/.Xresources");
    std::ifstream      f(path, std::ios::in);
    if (!f.is_open())
        return { cursor_name, cursor_size };

    std::uint16_t iter_index = 0;
    std::string   line;
    while (std::getline(f, line) && iter_index < 2)
    {
        if (hasStart(line, "Xcursor.theme:"))
        {
            getFileValue(iter_index, line, cursor_name, "Xcursor.theme:"_len);
            strip(cursor_name);
        }

        else if (hasStart(line, "Xcursor.size:"))
        {
            getFileValue(iter_index, line, cursor_size, "Xcursor.size:"_len);
            strip(cursor_size);
        }
    }

    return { cursor_name, cursor_size };
}

static CursorInfo get_cursor_dconf()
{
    std::string cursor{ MAGIC_LINE }, cursor_size{ MAGIC_LINE };
#if USE_DCONF
    void* handle = LOAD_LIBRARY("libdconf.so");
    if (!handle)
        return { MAGIC_LINE, MAGIC_LINE };

    LOAD_LIB_SYMBOL(handle, DConfClient*, dconf_client_new, void);
    LOAD_LIB_SYMBOL(handle, GVariant*, dconf_client_read, DConfClient*, const char*);
    LOAD_LIB_SYMBOL(handle, const gchar*, g_variant_get_string, GVariant*, gsize*);
    LOAD_LIB_SYMBOL(handle, gint32, g_variant_get_int32, GVariant*);

    debug("calling {}", __PRETTY_FUNCTION__);
    DConfClient* client = dconf_client_new();
    GVariant*    variant;

    variant = dconf_client_read(client, (dconf_interface + "cursor-theme").c_str());
    if (variant)
        cursor = g_variant_get_string(variant, NULL);

    variant = dconf_client_read(client, (dconf_interface + "cursor-size").c_str());
    if (variant)
        cursor_size = fmt::to_string(g_variant_get_int32(variant));
#endif
    return { cursor, cursor_size };
}

static CursorInfo get_cursor_gsettings()
{
    const CursorInfo& dconf = get_cursor_dconf();
    if (dconf != CursorInfo{ MAGIC_LINE, MAGIC_LINE })
        return dconf;

    std::string cursor;
    read_exec({ "gsettings", "get", gsetting_interface.c_str(), "cursor-theme" }, cursor);
    cursor.erase(std::remove(cursor.begin(), cursor.end(), '\''), cursor.end());

    std::string cursor_size;
    read_exec({ "gsettings", "get", gsetting_interface.c_str(), "cursor-size" }, cursor_size);
    cursor_size.erase(std::remove(cursor_size.begin(), cursor_size.end(), '\''), cursor_size.end());

    if (cursor.empty())
        cursor = MAGIC_LINE;
    if (cursor_size.empty())
        cursor_size = MAGIC_LINE;
    return { cursor, cursor_size };
}

static CursorInfo get_gtk_cursor_config(const std::string_view path)
{
    std::string   cursor{ MAGIC_LINE }, cursor_size{ MAGIC_LINE };
    std::ifstream f(path.data(), std::ios::in);
    if (!f.is_open())
        return { cursor, cursor_size };

    std::string   line;
    std::uint16_t iter_index = 0;
    while (std::getline(f, line) && iter_index < 2)
    {
        if (hasStart(line, "gtk-cursor-theme-name="))
            getFileValue(iter_index, line, cursor, "gtk-cursor-theme-name="_len);

        else if (hasStart(line, "gtk-cursor-theme-size="))
            getFileValue(iter_index, line, cursor_size, "gtk-cursor-theme-size="_len);
    }

    return { cursor, cursor_size };
}

static CursorInfo get_cursor_from_gtk_configs(const std::uint8_t ver)
{
    const std::array<std::string, 6> paths = { fmt::format("{}/gtk-{}.0/settings.ini", configDir, ver),
                                               fmt::format("{}/gtk-{}.0/gtkrc", configDir, ver),
                                               fmt::format("{}/gtkrc-{}.0", configDir, ver),
                                               fmt::format("{}/.gtkrc-{}.0", std::getenv("HOME"), ver),
                                               fmt::format("{}/.gtkrc-{}.0-kde", std::getenv("HOME"), ver),
                                               fmt::format("{}/.gtkrc-{}.0-kde4", std::getenv("HOME"), ver) };

    for (const std::string& path : paths)
    {
        const CursorInfo& result = get_gtk_cursor_config(path);
        if (result != CursorInfo{ MAGIC_LINE, MAGIC_LINE })
            return result;
    }
    return { MAGIC_LINE, MAGIC_LINE };
}

static CursorInfo get_de_cursor(const std::string_view de_name)
{
    switch (fnv1a16::hash(str_tolower(de_name.data())))
    {
        case "xfce"_fnv1a16:
        case "xfce4"_fnv1a16:
        {
            debug("getting info on xfce4");
            return { get_xsettings_xfce4("Gtk", "CursorThemeName"), get_xsettings_xfce4("Gtk", "CursorThemeSize") };
        }
    }
    return { MAGIC_LINE, MAGIC_LINE };
}

//
//
// 2. GTK theme
//
static ThemeInfo get_gtk_theme_config(const std::string_view path)
{
    std::string   theme{ MAGIC_LINE }, icon_theme{ MAGIC_LINE }, font{ MAGIC_LINE };
    std::ifstream f(path.data(), std::ios::in);
    if (!f.is_open())
        return { theme, icon_theme, font };

    std::string   line;
    std::uint16_t iter_index = 0;
    while (std::getline(f, line) && iter_index < 3)
    {
        if (hasStart(line, "gtk-theme-name="))
            getFileValue(iter_index, line, theme, "gtk-theme-name="_len);

        else if (hasStart(line, "gtk-icon-theme-name="))
            getFileValue(iter_index, line, icon_theme, "gtk-icon-theme-name="_len);

        else if (hasStart(line, "gtk-font-name="))
            getFileValue(iter_index, line, font, "gtk-font-name="_len);
    }

    return { theme, icon_theme, font };
}

static ThemeInfo get_gtk_theme_dconf()
{
    std::string theme{ MAGIC_LINE }, icon_theme{ MAGIC_LINE }, font{ MAGIC_LINE };
#if USE_DCONF
    void* handle = LOAD_LIBRARY("libdconf.so");
    if (!handle)
        return { theme, icon_theme, font };

    LOAD_LIB_SYMBOL(handle, DConfClient*, dconf_client_new, void);
    LOAD_LIB_SYMBOL(handle, GVariant*, dconf_client_read, DConfClient * client, const char*);
    LOAD_LIB_SYMBOL(handle, const gchar*, g_variant_get_string, GVariant* value, gsize* lenght);

    debug("calling {}", __PRETTY_FUNCTION__);
    DConfClient* client = dconf_client_new();
    GVariant*    variant;

    variant = dconf_client_read(client, (dconf_interface + "gtk-theme").c_str());
    if (variant)
        theme = g_variant_get_string(variant, NULL);

    variant = dconf_client_read(client, (dconf_interface + "icon-theme").c_str());
    if (variant)
        icon_theme = g_variant_get_string(variant, NULL);

    variant = dconf_client_read(client, (dconf_interface + "font-name").c_str());
    if (variant)
        font = g_variant_get_string(variant, NULL);

#endif
    return { theme, icon_theme, font };
}

static ThemeInfo get_gtk_theme_gsettings()
{
    const ThemeInfo& dconf = get_gtk_theme_dconf();
    if (dconf != ThemeInfo{ MAGIC_LINE, MAGIC_LINE, MAGIC_LINE })
        return dconf;

    std::string theme, icon_theme, font;

    read_exec({ "gsettings", "get", gsetting_interface.c_str(), "gtk-theme" }, theme);
    theme.erase(std::remove(theme.begin(), theme.end(), '\''), theme.end());

    read_exec({ "gsettings", "get", gsetting_interface.c_str(), "icon-theme" }, icon_theme);
    icon_theme.erase(std::remove(icon_theme.begin(), icon_theme.end(), '\''), icon_theme.end());

    read_exec({ "gsettings", "get", gsetting_interface.c_str(), "font-name" }, font);
    font.erase(std::remove(font.begin(), font.end(), '\''), font.end());

    if (theme.empty())
        theme = MAGIC_LINE;
    if (icon_theme.empty())
        icon_theme = MAGIC_LINE;
    if (font.empty())
        font = MAGIC_LINE;
    return { theme, icon_theme, font };
}

static ThemeInfo get_gtk_theme_from_configs(const std::uint8_t ver)
{
    const std::array<std::string, 6> paths = { fmt::format("{}/gtk-{}.0/settings.ini", configDir, ver),
                                               fmt::format("{}/gtk-{}.0/gtkrc", configDir, ver),
                                               fmt::format("{}/gtkrc-{}.0", configDir, ver),
                                               fmt::format("{}/.gtkrc-{}.0", std::getenv("HOME"), ver),
                                               fmt::format("{}/.gtkrc-{}.0-kde", std::getenv("HOME"), ver),
                                               fmt::format("{}/.gtkrc-{}.0-kde4", std::getenv("HOME"), ver) };

    for (const auto& path : paths)
    {
        const ThemeInfo& result = get_gtk_theme_config(path);
        if (result != ThemeInfo{ MAGIC_LINE, MAGIC_LINE, MAGIC_LINE })
            return result;
    }
    return get_gtk_theme_gsettings();
}

static ThemeInfo get_de_gtk_theme(const std::string_view de_name, const std::uint8_t ver)
{
    switch (fnv1a16::hash(str_tolower(de_name.data())))
    {
        case "xfce"_fnv1a16:
        case "xfce4"_fnv1a16:
        {
            debug("getting info on xfce4");
            return { get_xsettings_xfce4("Net", "ThemeName"), get_xsettings_xfce4("Net", "IconThemeName"),
                     get_xsettings_xfce4("Gtk", "FontName") };
        }
    }
    return get_gtk_theme_from_configs(ver);
}

const std::string& wmde_name =
    (de_name != MAGIC_LINE && de_name == wm_name) || de_name == MAGIC_LINE ? wm_name : de_name;

MODFUNC(theme_gtk_name)
{
    const moduleArgs_t* moduleArg = callbackInfo->module_args;
    for (; moduleArg && moduleArg->name != "gtk"; moduleArg = moduleArg->next)
        ;
    if (!moduleArg)
        die("GTK version not provided");
    int ver = std::stoi(moduleArg->value);

    const ThemeInfo& result = is_tty ? get_gtk_theme_from_configs(ver) : get_de_gtk_theme(wmde_name, ver);

    return result[THEME_NAME];
}

MODFUNC(theme_gtk_icon)
{
    const moduleArgs_t* moduleArg = callbackInfo->module_args;
    for (; moduleArg && moduleArg->name != "gtk"; moduleArg = moduleArg->next)
        ;
    if (!moduleArg)
        die("GTK version not provided");
    int ver = std::stoi(moduleArg->value);

    const ThemeInfo& result = is_tty ? get_gtk_theme_from_configs(ver) : get_de_gtk_theme(wmde_name, ver);

    return result[THEME_ICON];
}

MODFUNC(theme_gtk_font)
{
    const moduleArgs_t* moduleArg = callbackInfo->module_args;
    for (; moduleArg && moduleArg->name != "gtk"; moduleArg = moduleArg->next)
        ;
    if (!moduleArg)
        die("GTK version not provided");
    int ver = std::stoi(moduleArg->value);

    const ThemeInfo& result = is_tty ? get_gtk_theme_from_configs(ver) : get_de_gtk_theme(wmde_name, ver);

    return result[THEME_FONT];
}

MODFUNC(theme_gtk_all_name)
{
    const ThemeInfo& result_gtk2 = is_tty ? get_gtk_theme_from_configs(2) : get_de_gtk_theme(wmde_name, 2);
    const ThemeInfo& result_gtk3 = is_tty ? get_gtk_theme_from_configs(3) : get_de_gtk_theme(wmde_name, 3);
    const ThemeInfo& result_gtk4 = is_tty ? get_gtk_theme_from_configs(4) : get_de_gtk_theme(wmde_name, 4);

    return get_auto_gtk_format(result_gtk2[THEME_NAME], result_gtk3[THEME_NAME], result_gtk4[THEME_NAME]);
}

MODFUNC(theme_gtk_all_icon)
{
    const ThemeInfo& result_gtk2 = is_tty ? get_gtk_theme_from_configs(2) : get_de_gtk_theme(wmde_name, 2);
    const ThemeInfo& result_gtk3 = is_tty ? get_gtk_theme_from_configs(3) : get_de_gtk_theme(wmde_name, 3);
    const ThemeInfo& result_gtk4 = is_tty ? get_gtk_theme_from_configs(4) : get_de_gtk_theme(wmde_name, 4);

    return get_auto_gtk_format(result_gtk2[THEME_ICON], result_gtk3[THEME_ICON], result_gtk4[THEME_ICON]);
}

MODFUNC(theme_gtk_all_font)
{
    const ThemeInfo& result_gtk2 = is_tty ? get_gtk_theme_from_configs(2) : get_de_gtk_theme(wmde_name, 2);
    const ThemeInfo& result_gtk3 = is_tty ? get_gtk_theme_from_configs(3) : get_de_gtk_theme(wmde_name, 3);
    const ThemeInfo& result_gtk4 = is_tty ? get_gtk_theme_from_configs(4) : get_de_gtk_theme(wmde_name, 4);

    return get_auto_gtk_format(result_gtk2[THEME_FONT], result_gtk3[THEME_FONT], result_gtk4[THEME_FONT]);
}

MODFUNC(theme_gsettings_name)
{
    const ThemeInfo& result = get_gtk_theme_gsettings();
    return result[THEME_NAME];
}

MODFUNC(theme_gsettings_icon)
{
    const ThemeInfo& result = get_gtk_theme_gsettings();
    return result[THEME_ICON];
}

MODFUNC(theme_gsettings_font)
{
    const ThemeInfo& result = get_gtk_theme_gsettings();
    return result[THEME_FONT];
}

MODFUNC(theme_gsettings_cursor_name)
{
    const CursorInfo& result = get_cursor_gsettings();
    return result[CURSOR_NAME];
}

MODFUNC(theme_gsettings_cursor_size)
{
    const CursorInfo& result = get_cursor_gsettings();
    return result[CURSOR_SIZE];
}

const std::array<std::function<CursorInfo()>, 6> funcs{
    std::function<CursorInfo()>{ []() { return get_de_cursor(wmde_name); } },
    std::function<CursorInfo()>{ []() { return get_cursor_from_gtk_configs(4); } },
    std::function<CursorInfo()>{ []() { return get_cursor_from_gtk_configs(3); } },
    std::function<CursorInfo()>{ []() { return get_cursor_from_gtk_configs(2); } },
    std::function<CursorInfo()>{ []() { return get_cursor_xresources(); } },
    std::function<CursorInfo()>{ []() { return get_cursor_gsettings(); } }
};

MODFUNC(theme_cursor_name)
{
    CursorInfo result;

    for (const auto& method : funcs)
    {
        result = method();
        if (result != CursorInfo{ MAGIC_LINE, MAGIC_LINE })
            break;
    }

    if (result[CURSOR_NAME] == MAGIC_LINE)
        return MAGIC_LINE;

    std::string& cursor_name = result[CURSOR_NAME];
    size_t       pos         = 0;
    if ((pos = cursor_name.rfind("cursor")) != std::string::npos)
        cursor_name.erase(pos);
    if ((pos = cursor_name.rfind('_')) != std::string::npos)
        cursor_name.erase(pos - 1);

    return cursor_name;
}

MODFUNC(theme_cursor_size)
{
    CursorInfo result;

    for (const auto& method : funcs)
    {
        result = method();
        if (result != CursorInfo{ MAGIC_LINE, MAGIC_LINE })
            break;
    }

    if (result[CURSOR_SIZE] == MAGIC_LINE)
        return MAGIC_LINE;

    std::string& cursor_size = result[CURSOR_SIZE];
    size_t       pos         = 0;
    if ((pos = cursor_size.rfind("cursor")) != std::string::npos)
        cursor_size.erase(pos);
    if ((pos = cursor_size.rfind('_')) != std::string::npos)
        cursor_size.erase(pos - 1);

    return cursor_size;
}

#endif  // CF_LINUX
