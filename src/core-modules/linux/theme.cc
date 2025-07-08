#include "platform.hpp"
#if CF_LINUX

#include <fstream>
#include <functional>
#include <array>
#include <algorithm>
#include <cstdint>

#include "core-modules.hh"
#include "fmt/format.h"
#include "rapidxml-1.13/rapidxml.hpp"
#include "switch_fnv1a.hpp"
#include "util.hpp"

#if USE_DCONF
#include <client/dconf-client.h>
#include <glib/gvariant.h>
#endif

using ThemeInfo = std::array<std::string, 3>; // [theme, icon_theme, font]
using CursorInfo = std::array<std::string, 2>; // [name, size]

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

const std::string& configDir = getHomeConfigDir();

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
            return UNKNOWN;

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

    return UNKNOWN;
}

//
//
// 1. Cursor
//
static CursorInfo get_cursor_xresources()
{
    const std::string& path = expandVar("~/.Xresources");
    std::ifstream      f(path, std::ios::in);
    if (!f.is_open())
        return {UNKNOWN, UNKNOWN};

    std::string cursor_name, cursor_size;
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

    return {cursor_name, cursor_size};
}

static CursorInfo get_cursor_dconf(const std::string_view de_name)
{
    std::string cursor{UNKNOWN}, cursor_size{UNKNOWN};
#if USE_DCONF
    void* handle = LOAD_LIBRARY("libdconf.so");
    if (!handle)
        return {UNKNOWN, UNKNOWN};

    LOAD_LIB_SYMBOL(handle, DConfClient*, dconf_client_new, void);
    LOAD_LIB_SYMBOL(handle, GVariant*, dconf_client_read, DConfClient*, const char*);
    LOAD_LIB_SYMBOL(handle, const gchar*, g_variant_get_string, GVariant*, gsize*);
    LOAD_LIB_SYMBOL(handle, gint32, g_variant_get_int32, GVariant*);

    debug("calling {}", __PRETTY_FUNCTION__);
    DConfClient* client = dconf_client_new();
    GVariant*    variant;

    std::string interface;
    switch (fnv1a16::hash(str_tolower(de_name.data())))
    {
        case "cinnamon"_fnv1a16: interface = "/org/cinnamon/desktop/interface/"; break;
        case "mate"_fnv1a16:     interface = "/org/mate/interface/"; break;
        default:               interface = "/org/gnome/desktop/interface/";
    }

    variant = dconf_client_read(client, (interface + "cursor-theme").c_str());
    if (variant)
        cursor = g_variant_get_string(variant, NULL);

    variant = dconf_client_read(client, (interface + "cursor-size").c_str());
    if (variant)
        cursor_size = fmt::to_string(g_variant_get_int32(variant));
#endif
    return {cursor, cursor_size};
}

static CursorInfo get_cursor_gsettings(const std::string_view de_name)
{
    const CursorInfo& dconf = get_cursor_dconf(de_name);
    if (dconf != CursorInfo{UNKNOWN, UNKNOWN})
        return dconf;
    const char* interface;
    switch (fnv1a16::hash(str_tolower(de_name.data())))
    {
        case "cinnamon"_fnv1a16: interface = "org.cinnamon.desktop.interface"; break;
        case "mate"_fnv1a16:     interface = "org.mate.interface"; break;
        default:                 interface = "org.gnome.desktop.interface";
    }

    std::string cursor{UNKNOWN};
    read_exec({ "gsettings", "get", interface, "cursor-theme" }, cursor);
    cursor.erase(std::remove(cursor.begin(), cursor.end(), '\''), cursor.end());

    std::string cursor_size{UNKNOWN};
    read_exec({ "gsettings", "get", interface, "cursor-size" }, cursor_size);
    cursor_size.erase(std::remove(cursor_size.begin(), cursor_size.end(), '\''), cursor_size.end());

    return {cursor, cursor_size};
}

static CursorInfo get_gtk_cursor_config(const std::string_view path)
{
    std::string cursor{UNKNOWN}, cursor_size{UNKNOWN};
    std::ifstream f(path.data(), std::ios::in);
    if (!f.is_open())
        return {cursor, cursor_size};

    std::string   line;
    std::uint16_t iter_index = 0;
    while (std::getline(f, line) && iter_index < 2)
    {
        if (hasStart(line, "gtk-cursor-theme-name="))
            getFileValue(iter_index, line, cursor, "gtk-cursor-theme-name="_len);

        else if (hasStart(line, "gtk-cursor-theme-size="))
            getFileValue(iter_index, line, cursor_size, "gtk-cursor-theme-size="_len);
    }

    return {cursor, cursor_size};
}

static CursorInfo get_cursor_from_gtk_configs(const std::uint8_t ver)
{
    const std::array<std::string, 6> paths = {
        fmt::format("{}/gtk-{}.0/settings.ini", configDir, ver),
        fmt::format("{}/gtk-{}.0/gtkrc", configDir, ver),
        fmt::format("{}/gtkrc-{}.0", configDir, ver),
        fmt::format("{}/.gtkrc-{}.0", std::getenv("HOME"), ver),
        fmt::format("{}/.gtkrc-{}.0-kde", std::getenv("HOME"), ver),
        fmt::format("{}/.gtkrc-{}.0-kde4", std::getenv("HOME"), ver)
    };

    for (const std::string& path : paths)
    {
        const CursorInfo& result = get_gtk_cursor_config(path);
        if (result != CursorInfo{UNKNOWN, UNKNOWN})
            return result;
    }
    return {UNKNOWN, UNKNOWN};
}

static CursorInfo get_de_cursor(const std::string_view de_name)
{
    switch (fnv1a16::hash(str_tolower(de_name.data())))
    {
        case "xfce"_fnv1a16:
        case "xfce4"_fnv1a16:
        {
            debug("getting info on xfce4");
            return {get_xsettings_xfce4("Gtk", "CursorThemeName"), get_xsettings_xfce4("Gtk", "CursorThemeSize")};
        }
    }
    return {UNKNOWN, UNKNOWN};
}

//
//
// 2. GTK theme
//
static ThemeInfo get_gtk_theme_config(const std::string_view path)
{
    std::string theme{UNKNOWN}, icon_theme{UNKNOWN}, font{UNKNOWN};
    std::ifstream f(path.data(), std::ios::in);
    if (!f.is_open())
        return {theme, icon_theme, font};

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

    return {theme, icon_theme, font};
}

static ThemeInfo get_gtk_theme_dconf(const std::string_view de_name)
{
    std::string theme{UNKNOWN}, icon_theme{UNKNOWN}, font{UNKNOWN};
#if USE_DCONF
    void* handle = LOAD_LIBRARY("libdconf.so");
    if (!handle)
        return {theme, icon_theme, font};

    LOAD_LIB_SYMBOL(handle, DConfClient*, dconf_client_new, void);
    LOAD_LIB_SYMBOL(handle, GVariant*, dconf_client_read, DConfClient * client, const char*);
    LOAD_LIB_SYMBOL(handle, const gchar*, g_variant_get_string, GVariant* value, gsize* lenght);

    debug("calling {}", __PRETTY_FUNCTION__);
    DConfClient* client = dconf_client_new();
    GVariant*    variant;

    std::string interface;
    switch (fnv1a16::hash(str_tolower(de_name.data())))
    {
        case "cinnamon"_fnv1a16: interface = "/org/cinnamon/desktop/interface/"; break;
        case "mate"_fnv1a16:     interface = "/org/mate/interface/"; break;
        default:                 interface = "/org/gnome/desktop/interface/";
    }

    variant = dconf_client_read(client, (interface + "gtk-theme").c_str());
    if (variant)
        theme = g_variant_get_string(variant, NULL);

    variant = dconf_client_read(client, (interface + "icon-theme").c_str());
    if (variant)
        icon_theme = g_variant_get_string(variant, NULL);

    variant = dconf_client_read(client, (interface + "font-name").c_str());
    if (variant)
        font = g_variant_get_string(variant, NULL);

#endif
    return {theme, icon_theme, font};
}

static ThemeInfo get_gtk_theme_gsettings(const std::string_view de_name)
{
    const ThemeInfo& dconf = get_gtk_theme_dconf(de_name);
    if (dconf != ThemeInfo{UNKNOWN, UNKNOWN, UNKNOWN})
        return dconf;

    std::string theme{UNKNOWN}, icon_theme{UNKNOWN}, font{UNKNOWN};
    const char* interface;
    switch (fnv1a16::hash(str_tolower(de_name.data())))
    {
        case "cinnamon"_fnv1a16: interface = "org.cinnamon.desktop.interface"; break;
        case "mate"_fnv1a16:     interface = "org.mate.interface"; break;
        default:                 interface = "org.gnome.desktop.interface";
    }

    read_exec({ "gsettings", "get", interface, "gtk-theme" }, theme);
    theme.erase(std::remove(theme.begin(), theme.end(), '\''), theme.end());

    read_exec({ "gsettings", "get", interface, "icon-theme" }, icon_theme);
    icon_theme.erase(std::remove(icon_theme.begin(), icon_theme.end(), '\''), icon_theme.end());

    read_exec({ "gsettings", "get", interface, "font-name" }, font);
    font.erase(std::remove(font.begin(), font.end(), '\''), font.end());

    return {theme, icon_theme, font};
}

static ThemeInfo get_gtk_theme_from_configs(const std::uint8_t ver, const std::string_view de_name)
{
    const std::array<std::string, 6> paths = {
        fmt::format("{}/gtk-{}.0/settings.ini", configDir, ver),
        fmt::format("{}/gtk-{}.0/gtkrc", configDir, ver),
        fmt::format("{}/gtkrc-{}.0", configDir, ver),
        fmt::format("{}/.gtkrc-{}.0", std::getenv("HOME"), ver),
        fmt::format("{}/.gtkrc-{}.0-kde", std::getenv("HOME"), ver),
        fmt::format("{}/.gtkrc-{}.0-kde4", std::getenv("HOME"), ver)
    };

    for (const auto& path : paths)
    {
        const ThemeInfo& result = get_gtk_theme_config(path);
        if (result != ThemeInfo{UNKNOWN, UNKNOWN, UNKNOWN})
            return result;
    }
    return get_gtk_theme_gsettings(de_name);
}

static ThemeInfo get_de_gtk_theme(const std::string_view de_name, const std::uint8_t ver)
{
    switch (fnv1a16::hash(str_tolower(de_name.data())))
    {
        case "xfce"_fnv1a16:
        case "xfce4"_fnv1a16:
        {
            debug("getting info on xfce4");
            return {
                    get_xsettings_xfce4("Net", "ThemeName"),
                    get_xsettings_xfce4("Net", "IconThemeName"),
                    get_xsettings_xfce4("Gtk", "FontName")
                };
        }
    }
    return get_gtk_theme_from_configs(ver, de_name);
}

const std::string& wmde_name = (de_name != MAGIC_LINE && de_name == wm_name) || de_name == MAGIC_LINE 
        ? wm_name 
        : de_name;

MODFUNC(theme_gtk_name)
{
    const ThemeInfo& result = is_tty
        ? get_gtk_theme_from_configs(3, wmde_name)
        : get_de_gtk_theme(wmde_name, 3);

    return result[THEME_NAME] == UNKNOWN ? MAGIC_LINE : result[THEME_NAME];
}

MODFUNC(theme_gtk_icon)
{
    const ThemeInfo& result = is_tty
        ? get_gtk_theme_from_configs(3, wmde_name)
        : get_de_gtk_theme(wmde_name, 3);

    return result[THEME_ICON] == UNKNOWN ? MAGIC_LINE : result[THEME_ICON];
}

MODFUNC(theme_gtk_font)
{
    const ThemeInfo& result = is_tty
        ? get_gtk_theme_from_configs(3, wmde_name)
        : get_de_gtk_theme(wmde_name, 3);

    return result[THEME_FONT] == UNKNOWN ? MAGIC_LINE : result[THEME_FONT];
}

MODFUNC(theme_gsettings_name)
{
    const ThemeInfo& result = get_gtk_theme_gsettings(de_name);
    return result[THEME_NAME] == UNKNOWN ? MAGIC_LINE : result[THEME_NAME];
}

MODFUNC(theme_gsettings_icon)
{
    const ThemeInfo& result = get_gtk_theme_gsettings(de_name);
    return result[THEME_ICON] == UNKNOWN ? MAGIC_LINE : result[THEME_ICON];
}

MODFUNC(theme_gsettings_font)
{
    const ThemeInfo& result = get_gtk_theme_gsettings(de_name);
    return result[THEME_FONT] == UNKNOWN ? MAGIC_LINE : result[THEME_FONT];
}

const std::array<std::function<CursorInfo()>, 6> funcs{
        std::function<CursorInfo()>{[]() { return get_de_cursor(wmde_name); }},
        std::function<CursorInfo()>{[]() { return get_cursor_from_gtk_configs(4); }},
        std::function<CursorInfo()>{[]() { return get_cursor_from_gtk_configs(3); }},
        std::function<CursorInfo()>{[]() { return get_cursor_from_gtk_configs(2); }},
        std::function<CursorInfo()>{[]() { return get_cursor_xresources(); }},
        std::function<CursorInfo()>{[]() { return get_cursor_gsettings(wmde_name); }}
    };

MODFUNC(theme_cursor_name)
{
    CursorInfo result;

    for (const auto& method : funcs)
    {
        result = method();
        if (result != CursorInfo{UNKNOWN, UNKNOWN})
            break;
    }

    if (result[CURSOR_NAME] == UNKNOWN)
        return MAGIC_LINE;

    std::string& cursor_name = result[CURSOR_NAME];
    size_t pos = 0;
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
        if (result != CursorInfo{UNKNOWN, UNKNOWN})
            break;
    }

    if (result[CURSOR_SIZE] == UNKNOWN)
        return MAGIC_LINE;

    std::string& cursor_size = result[CURSOR_SIZE];
    size_t pos = 0;
    if ((pos = cursor_size.rfind("cursor")) != std::string::npos)
        cursor_size.erase(pos);
    if ((pos = cursor_size.rfind('_')) != std::string::npos)
        cursor_size.erase(pos - 1);

    return cursor_size;
}

#endif // CF_LINUX
