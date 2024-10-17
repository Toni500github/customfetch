#include <algorithm>
#include <cstdint>

#include "config.hpp"
#include "fmt/format.h"
#include "parse.hpp"
#include "query.hpp"
#include "rapidxml-1.13/rapidxml.hpp"
#include "switch_fnv1a.hpp"
#include "util.hpp"

#if USE_DCONF
#  include <client/dconf-client.h>
#  include <glib/gvariant.h>
#endif

using namespace Query;

const std::string& configDir = getHomeConfigDir();

static bool get_xsettings_xfce4(const std::string_view property, const std::string_view subproperty, std::string& ret)
{
    static bool done = false;
    static rapidxml::xml_document<> doc;

    if (!done)
    {
        const std::string& path = configDir + "/xfce4/xfconf/xfce-perchannel-xml/xsettings.xml";
        std::ifstream      f(path, std::ios::in);
        if (!f.is_open())
            return false;

        std::string buffer((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        buffer.push_back('\0');

        doc.parse<0>(&buffer[0]);
        done = true;
    }

    rapidxml::xml_node<>* node1 = doc.first_node("channel")->first_node("property");
    for (; node1 && std::string_view(node1->first_attribute("name")->value()) != property; node1 = node1->next_sibling("property"));

    rapidxml::xml_node<>* node2 = node1->first_node("property");
    for (; node2; node2 = node2->next_sibling())
    {
        if (std::string_view(node2->first_attribute("name")->value()) == subproperty &&
            node2->first_attribute("value"))
        {
            ret = node2->first_attribute("value")->value();
            return true;
        }
    }

    return false;
}

//
//
// 1. Cursor
//
static bool assert_cursor(Theme::Theme_t& theme)
{
    return
    (theme.cursor != MAGIC_LINE && theme.cursor_size != UNKNOWN) || 
    (!theme.cursor.empty() && !theme.cursor_size.empty());
}

static bool get_cursor_xresources(Theme::Theme_t& theme)
{
    const std::string& path = expandVar("~/.Xresources");
    std::ifstream f(path, std::ios::in);
    if (!f.is_open())
    {
        theme.cursor = MAGIC_LINE;
        theme.cursor_size = UNKNOWN;
        return false;
    }

    std::uint16_t iter_index = 0;
    std::string line;
    while (std::getline(f, line) && iter_index < 2)
    {
        if (hasStart(line, "Xcursor.theme:"))
        {
            getFileValue(iter_index, line, theme.cursor, "Xcursor.theme:"_len);
            strip(theme.cursor);
        }

        else if (hasStart(line, "Xcursor.size:"))
        {
            getFileValue(iter_index, line, theme.cursor_size, "Xcursor.size:"_len);
            strip(theme.cursor_size);
        }
    }

    return assert_cursor(theme);
}

static bool get_cursor_dconf(const std::string_view de_name, Theme::Theme_t& theme)
{
#if USE_DCONF

    LOAD_LIBRARY("libdconf.so", return false);
    LOAD_LIB_SYMBOL(DConfClient *, dconf_client_new, void);
    LOAD_LIB_SYMBOL(GVariant *, dconf_client_read, DConfClient *, const char *);
    LOAD_LIB_SYMBOL(const gchar *, g_variant_get_string, GVariant *, gsize *);
    LOAD_LIB_SYMBOL(gint32, g_variant_get_int32, GVariant *);

    debug("calling {}", __PRETTY_FUNCTION__);
    DConfClient *client = dconf_client_new();
    GVariant *variant;

    std::string interface;
    switch(fnv1a16::hash(str_tolower(de_name.data())))
    {
        case "cinnamon"_fnv1a16: interface = "/org/cinnamon/desktop/interface/"; break;
        case "mate"_fnv1a16: interface = "/org/mate/interface/"; break;

        case "gnome"_fnv1a16:
        case "budgie"_fnv1a16:
        case "unity"_fnv1a16:
        default:
            interface = "/org/gnome/desktop/interface/";
    }

    
    variant = dconf_client_read(client, (interface + "cursor-theme").c_str());
    if (variant)
        theme.cursor = g_variant_get_string(variant, NULL);
    

    
    variant = dconf_client_read(client, (interface + "cursor-size").c_str());
    if (variant)
        theme.cursor_size = fmt::to_string(g_variant_get_int32(variant));

    return assert_cursor(theme);
#else
    return false;
#endif
}

static bool get_cursor_gsettings(const std::string_view de_name, Theme::Theme_t& theme, const Config& config)
{
    debug("calling {}", __PRETTY_FUNCTION__);
    if (get_cursor_dconf(de_name, theme))
        return true;

    if (config.slow_query_warnings)
    {
        warn("cufetch could not detect a gtk configuration file. cufetch will use the much-slower gsettings.");
        warn("If there's a file in a standard location that we aren't detecting, please file an issue on our GitHub.");
        info("You can disable this warning by disabling slow-query-warnings in your config.toml file.");
    }

    const char* interface;
    switch(fnv1a16::hash(str_tolower(de_name.data())))
    {
        case "cinnamon"_fnv1a16: interface = "org.cinnamon.desktop.interface"; break;
        case "mate"_fnv1a16: interface = "org.mate.interface"; break;

        case "gnome"_fnv1a16:
        case "budgie"_fnv1a16:
        case "unity"_fnv1a16:
        default:
            interface = "org.gnome.desktop.interface";
    }

    if (theme.cursor == MAGIC_LINE || theme.cursor.empty())
    {
        theme.cursor.clear();
        read_exec({ "gsettings", "get", interface, "cursor-theme" }, theme.cursor);
        theme.cursor.erase(std::remove(theme.cursor.begin(), theme.cursor.end(), '\''), theme.cursor.end());
    }

    if (theme.cursor_size == UNKNOWN || theme.cursor_size.empty())
    {
        theme.cursor_size.clear();
        read_exec({ "gsettings", "get", interface, "cursor-size" }, theme.cursor_size);
        theme.cursor_size.erase(std::remove(theme.cursor_size.begin(), theme.cursor_size.end(), '\''), theme.cursor_size.end());
    }

    return assert_cursor(theme);
}

static bool get_gtk_cursor_config(const std::string_view path, Theme::Theme_t& theme)
{
    std::ifstream f(path.data(), std::ios::in);
    if (!f.is_open())
        return false;

    std::string    line;
    std::uint16_t  iter_index = 0;
    while (std::getline(f, line) && iter_index < 2)
    {
        if (hasStart(line, "gtk-cursor-theme-name="))
            getFileValue(iter_index, line, theme.cursor, "gtk-cursor-theme-name="_len);

        else if (hasStart(line, "gtk-cursor-theme-size="))
            getFileValue(iter_index, line, theme.cursor_size, "gtk-cursor-theme-size="_len);
    }

    return assert_cursor(theme);
}

static bool get_cursor_from_gtk_configs(const std::uint8_t ver, Theme::Theme_t& theme)
{
    if (get_gtk_cursor_config(fmt::format("{}/gtk-{}.0/settings.ini", configDir, ver), theme))
        return true;

    if (get_gtk_cursor_config(fmt::format("{}/gtk-{}.0/gtkrc", configDir, ver), theme))
        return true;

    if (get_gtk_cursor_config(fmt::format("{}/gtkrc-{}.0", configDir, ver), theme))
        return true;

    if (get_gtk_cursor_config(fmt::format("{}/.gtkrc-{}.0", std::getenv("HOME"), ver), theme))
        return true;

    if (get_gtk_cursor_config(fmt::format("{}/.gtkrc-{}.0-kde", std::getenv("HOME"), ver), theme))
        return true;

    if (get_gtk_cursor_config(fmt::format("{}/.gtkrc-{}.0-kde4", std::getenv("HOME"), ver), theme))
        return true;

    return false;
}

static bool get_de_cursor(const std::string_view de_name, Theme::Theme_t& theme)
{
    switch (fnv1a16::hash(str_tolower(de_name.data())))
    {
        case "xfce"_fnv1a16:
        case "xfce4"_fnv1a16:
        {
            debug("calling {} and getting info on xfce4", __PRETTY_FUNCTION__);
            get_xsettings_xfce4("Gtk", "CursorThemeName", theme.cursor);
            get_xsettings_xfce4("Gtk", "CursorThemeSize", theme.cursor_size);

            return assert_cursor(theme);

        } break;
    }
    
    return false;
}

//
//
// 2. GTK theme
//
static bool assert_gtk_theme(Theme::Theme_t& theme)
{
    return
    (theme.gtk_font != MAGIC_LINE && theme.gtk_icon_theme != MAGIC_LINE && theme.gtk_theme_name != MAGIC_LINE) || 
    (!theme.gtk_font.empty() && !theme.gtk_theme_name.empty() && !theme.gtk_icon_theme.empty());
}

static bool get_gtk_theme_config(const std::string_view path, Theme::Theme_t& theme)
{
    std::ifstream f(path.data(), std::ios::in);
    if (!f.is_open())
        return false;

    std::string    line;
    std::uint16_t  iter_index = 0;
    while (std::getline(f, line) && iter_index < 3)
    {
        if (hasStart(line, "gtk-theme-name="))
            getFileValue(iter_index, line, theme.gtk_theme_name, "gtk-theme-name="_len);

        else if (hasStart(line, "gtk-icon-theme-name="))
            getFileValue(iter_index, line, theme.gtk_icon_theme, "gtk-icon-theme-name="_len);

        else if (hasStart(line, "gtk-font-name="))
            getFileValue(iter_index, line, theme.gtk_font, "gtk-font-name="_len);
    }

    return assert_gtk_theme(theme);
}

static bool get_gtk_theme_dconf(const std::string_view de_name, Theme::Theme_t& theme)
{
#if USE_DCONF

    LOAD_LIBRARY("libdconf.so", return false);
    LOAD_LIB_SYMBOL(DConfClient *, dconf_client_new, void);
    LOAD_LIB_SYMBOL(GVariant *, dconf_client_read, DConfClient *client, const char *);
    LOAD_LIB_SYMBOL(const gchar *, g_variant_get_string, GVariant *value, gsize *lenght);

    debug("calling {}", __PRETTY_FUNCTION__);
    DConfClient *client = dconf_client_new();
    GVariant *variant;

    std::string interface;
    switch(fnv1a16::hash(str_tolower(de_name.data())))
    {
        case "cinnamon"_fnv1a16: interface = "/org/cinnamon/desktop/interface/"; break;
        case "mate"_fnv1a16: interface = "/org/mate/interface/"; break;

        case "gnome"_fnv1a16:
        case "budgie"_fnv1a16:
        case "unity"_fnv1a16:
        default:
            interface = "/org/gnome/desktop/interface/";
    }

    if (theme.gtk_theme_name == MAGIC_LINE || theme.gtk_theme_name.empty())
    {
        variant = dconf_client_read(client, (interface + "gtk-theme").c_str());
        if (variant)
            theme.gtk_theme_name = g_variant_get_string(variant, NULL);
    }

    if (theme.gtk_icon_theme == MAGIC_LINE || theme.gtk_icon_theme.empty())
    {
        variant = dconf_client_read(client, (interface + "icon-theme").c_str());
        if (variant)
            theme.gtk_icon_theme = g_variant_get_string(variant, NULL);
    }

    if (theme.gtk_font == MAGIC_LINE || theme.gtk_font.empty())
    {
        variant = dconf_client_read(client, (interface + "font-name").c_str());
        if (variant)
            theme.gtk_font = g_variant_get_string(variant, NULL);
    }

    return assert_gtk_theme(theme);

#else
    return false;
#endif
}

static void get_gtk_theme_gsettings(const std::string_view de_name, Theme::Theme_t& theme, const Config& config)
{
    debug("calling {}", __PRETTY_FUNCTION__);

    if (theme.gtk_theme_name == MAGIC_LINE || theme.gtk_theme_name.empty())
    {
        const char* gtk_theme_env = std::getenv("GTK_THEME");

        if (gtk_theme_env)
            theme.gtk_theme_name = gtk_theme_env;
    }

    if (get_gtk_theme_dconf(de_name, theme))
        return;

    if (config.slow_query_warnings)
    {
        warn("cufetch could not detect a gtk configuration file. cufetch will use the much-slower gsettings.");
        warn("If there's a file in a standard location that we aren't detecting, please file an issue on our GitHub.");
        info("You can disable this warning by disabling slow-query-warnings in your config.toml file.");
    }

    const char* interface;
    switch(fnv1a16::hash(str_tolower(de_name.data())))
    {
        case "cinnamon"_fnv1a16: interface = "org.cinnamon.desktop.interface"; break;
        case "mate"_fnv1a16: interface = "org.mate.interface"; break;

        case "gnome"_fnv1a16:
        case "budgie"_fnv1a16:
        case "unity"_fnv1a16:
        default:
            interface = "org.gnome.desktop.interface";
    }

    if (theme.gtk_theme_name == MAGIC_LINE || theme.gtk_theme_name.empty())
    {
        theme.gtk_theme_name.clear();
        read_exec({ "gsettings", "get", interface, "gtk-theme" }, theme.gtk_theme_name);
    }

    if (theme.gtk_icon_theme == MAGIC_LINE || theme.gtk_icon_theme.empty())
    {
        theme.gtk_icon_theme.clear();
        read_exec({ "gsettings", "get", interface, "icon-theme" }, theme.gtk_icon_theme);
    }

    if (theme.gtk_font == MAGIC_LINE || theme.gtk_font.empty())
    {
        theme.gtk_font.clear();
        read_exec({ "gsettings", "get", interface, "font-name" }, theme.gtk_font);
    }

    theme.gtk_theme_name.erase(std::remove(theme.gtk_theme_name.begin(), theme.gtk_theme_name.end(), '\''), theme.gtk_theme_name.end());
    theme.gtk_icon_theme.erase(std::remove(theme.gtk_icon_theme.begin(), theme.gtk_icon_theme.end(), '\''), theme.gtk_icon_theme.end());
    theme.gtk_font.erase(std::remove(theme.gtk_font.begin(), theme.gtk_font.end(), '\''), theme.gtk_font.end());
}

static void get_gtk_theme_from_configs(const std::uint8_t ver, const std::string_view de_name, Theme::Theme_t& theme, const Config& config)
{
    if (get_gtk_theme_config(fmt::format("{}/gtk-{}.0/settings.ini", configDir, ver), theme))
        return;

    if (get_gtk_theme_config(fmt::format("{}/gtk-{}.0/gtkrc", configDir, ver), theme))
        return;

    if (get_gtk_theme_config(fmt::format("{}/gtkrc-{}.0", configDir, ver), theme))
        return;

    if (get_gtk_theme_config(fmt::format("{}/.gtkrc-{}.0", std::getenv("HOME"), ver), theme))
        return;

    if (get_gtk_theme_config(fmt::format("{}/.gtkrc-{}.0-kde", std::getenv("HOME"), ver), theme))
        return;

    if (get_gtk_theme_config(fmt::format("{}/.gtkrc-{}.0-kde4", std::getenv("HOME"), ver), theme))
        return;

    get_gtk_theme_gsettings(de_name, theme, config);
}

static void get_de_gtk_theme(const std::string_view de_name, const std::uint8_t ver, Theme::Theme_t& theme, const Config& config)
{
    switch (fnv1a16::hash(str_tolower(de_name.data())))
    {
        case "xfce"_fnv1a16:
        case "xfce4"_fnv1a16:
        {
            debug("calling {} and getting info on xfce4", __PRETTY_FUNCTION__);
            get_xsettings_xfce4("Net", "ThemeName", theme.gtk_theme_name);
            get_xsettings_xfce4("Net", "IconThemeName", theme.gtk_icon_theme);
            get_xsettings_xfce4("Gtk", "FontName",   theme.gtk_font);

            if (!assert_gtk_theme(theme))
                get_gtk_theme_from_configs(ver, de_name, theme, config);

        } break;

        default:
            get_gtk_theme_from_configs(ver, de_name, theme, config);
    }
}

static void get_gtk_theme(const bool dont_query_dewm, const std::uint8_t ver, const std::string_view de_name,
                          Theme::Theme_t& theme, const Config& config, const bool gsettings_only)
{
    if (gsettings_only)
        get_gtk_theme_gsettings(de_name, theme, config);
    else if (dont_query_dewm)
        get_gtk_theme_from_configs(ver, de_name, theme, config);
    else 
        get_de_gtk_theme(de_name, ver, theme, config);
}

// clang-format off
Theme::Theme(const std::uint8_t ver, systemInfo_t& queried_themes, std::vector<std::string>& queried_themes_names,
             const std::string& theme_name_version, const Config& config, const bool gsettings_only)
            : m_queried_themes(queried_themes),
              m_theme_name_version(theme_name_version)
{
    if (std::find(queried_themes_names.begin(), queried_themes_names.end(), m_theme_name_version)
        == queried_themes_names.end())
        queried_themes_names.push_back(m_theme_name_version);
    else
        return;

    const std::string& wm_name = query_user.wm_name(query_user.m_bDont_query_dewm, query_user.term_name());
    const std::string& de_name = query_user.de_name(query_user.m_bDont_query_dewm, query_user.term_name(), wm_name);

    if (((de_name != MAGIC_LINE && wm_name != MAGIC_LINE) &&
         de_name == wm_name) || de_name == MAGIC_LINE)
        m_wmde_name = wm_name;
    else
        m_wmde_name = de_name;

    get_gtk_theme(query_user.m_bDont_query_dewm, ver, m_wmde_name, m_theme_infos, config, gsettings_only);

    if (m_theme_infos.gtk_theme_name.empty())
        m_theme_infos.gtk_theme_name = MAGIC_LINE;

    if (m_theme_infos.gtk_font.empty())
        m_theme_infos.gtk_font = MAGIC_LINE;

    if (m_theme_infos.gtk_icon_theme.empty())
        m_theme_infos.gtk_icon_theme = MAGIC_LINE;

    m_queried_themes.insert(
        {m_theme_name_version, {
            {"theme-name",      variant(m_theme_infos.gtk_theme_name)},
            {"icon-theme-name", variant(m_theme_infos.gtk_icon_theme)},
            {"font-name",       variant(m_theme_infos.gtk_font)},
        }}
    );
}

// only use it for cursor
Theme::Theme(systemInfo_t& queried_themes, const Config& config, const bool gsettings_only) : m_queried_themes(queried_themes)
{
    const std::string& wm_name = query_user.wm_name(query_user.m_bDont_query_dewm, query_user.term_name());
    const std::string& de_name = query_user.de_name(query_user.m_bDont_query_dewm, query_user.term_name(), wm_name);

    if (((de_name != MAGIC_LINE && wm_name != MAGIC_LINE) &&
         de_name == wm_name) || de_name == MAGIC_LINE)
        m_wmde_name = wm_name;
    else
        m_wmde_name = de_name;

    if (gsettings_only) { get_cursor_gsettings(m_wmde_name, m_theme_infos, config); }
    else if (get_de_cursor(m_wmde_name, m_theme_infos)){}
    else if (get_cursor_from_gtk_configs(4, m_theme_infos)){}
    else if (get_cursor_from_gtk_configs(3, m_theme_infos)){}
    else if (get_cursor_from_gtk_configs(2, m_theme_infos)){}
    else if (get_cursor_xresources(m_theme_infos)){}
    else get_cursor_gsettings(m_wmde_name, m_theme_infos, config);

    if (m_theme_infos.cursor.empty())
        m_theme_infos.cursor = MAGIC_LINE;
    else
    {
        size_t pos = 0;
        if ((pos = m_theme_infos.cursor.rfind("cursor")) != std::string::npos)
            m_theme_infos.cursor.erase(pos);

        if ((pos = m_theme_infos.cursor.rfind('_')) != std::string::npos)
            m_theme_infos.cursor.erase(pos - 1);

    }

}

std::string Theme::gtk_theme() noexcept
{ return getInfoFromName(m_queried_themes, m_theme_name_version, "theme-name"); }

std::string Theme::gtk_icon_theme() noexcept
{ return getInfoFromName(m_queried_themes, m_theme_name_version, "icon-theme-name"); }

std::string Theme::gtk_font() noexcept
{ return getInfoFromName(m_queried_themes, m_theme_name_version, "font-name"); }

std::string& Theme::cursor() noexcept
{ return m_theme_infos.cursor; }

std::string& Theme::cursor_size() noexcept
{ return m_theme_infos.cursor_size; }
