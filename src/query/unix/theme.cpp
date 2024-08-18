#include <algorithm>
#include <cstdint>

#include "parse.hpp"
#include "query.hpp"
#include "rapidxml-1.13/rapidxml.hpp"
#include "switch_fnv1a.hpp"
#include "util.hpp"

using namespace Query;

const std::string& configDir = getHomeConfigDir();

// 1. Cursor
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

    assert_cursor(theme);
}

static bool get_cursor_gsettings(const std::string_view de_name, Theme::Theme_t& theme)
{
    debug("calling {}", __PRETTY_FUNCTION__);

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
    }

    if (theme.cursor_size == UNKNOWN || theme.cursor_size.empty())
    {
        theme.cursor_size.clear();
        read_exec({ "gsettings", "get", interface, "cursor-size" }, theme.cursor_size);
    }

    theme.cursor.erase(std::remove(theme.cursor.begin(), theme.cursor.end(), '\''), theme.cursor.end());
    theme.cursor_size.erase(std::remove(theme.cursor_size.begin(), theme.cursor_size.end(), '\''), theme.cursor_size.end());

    if (!assert_cursor(theme))
    {
        theme.cursor = MAGIC_LINE;
        return false;
    }

    return true;
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

static bool get_cursor_from_gtk_configs(const std::uint8_t ver, const std::string_view de_name, Theme::Theme_t& theme)
{
    if (get_gtk_cursor_config(fmt::format("{}/gtk-{}.0/settings.ini", configDir, ver), theme))
        return true;

    if (get_gtk_cursor_config(fmt::format("{}/gtk-{}.0/gtkrc", configDir, ver), theme))
        return true;

    if (get_gtk_cursor_config(fmt::format("{}/gtkrc-{}.0", configDir, ver), theme))
        return true;

    if (get_gtk_cursor_config(fmt::format("{}/.gtkrc-{}.0", std::getenv("HOME"), ver), theme))
        return true;

    return get_cursor_gsettings(de_name, theme);
}

static bool get_de_cursor(const std::string_view de_name, Theme::Theme_t& theme)
{
    switch (fnv1a16::hash(str_tolower(de_name.data())))
    {
        case "xfce"_fnv1a16:
        case "xfce4"_fnv1a16:
            {
                debug("calling {} and getting info on xfce4", __PRETTY_FUNCTION__);
                const std::string& path = configDir + "/xfce4/xfconf/xfce-perchannel-xml/xsettings.xml";
                std::ifstream      f(path, std::ios::in);
                if (!f.is_open())
                    return true;

                std::string buffer((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                buffer.push_back('\0');

                rapidxml::xml_document<> doc;
                doc.parse<0>(&buffer[0]);
                rapidxml::xml_node<>* net_node = doc.first_node("channel")->first_node("property");
                for (; net_node && std::string(net_node->first_attribute("name")->value()) != "Gtk"; net_node = net_node->next_sibling("property"));
                
                unsigned short iter_index = 0;
                rapidxml::xml_node<>* theme_node = net_node->first_node("property");
                for (; theme_node && iter_index < 2; theme_node = theme_node->next_sibling())
                {
                    if (std::string(theme_node->first_attribute("name")->value()) == "CursorThemeName" &&
                        theme_node->first_attribute("value"))
                    {
                        theme.cursor = theme_node->first_attribute("value")->value();
                        iter_index++;
                    }
                    else if (std::string(theme_node->first_attribute("name")->value()) == "CursorThemeSize" &&
                             theme_node->first_attribute("value"))
                    {
                        theme.cursor_size = theme_node->first_attribute("value")->value();
                        iter_index++;
                    }
                }

                if (!assert_cursor(theme))
                    return false;
            }
            break;

        default:
            return false;
    }
    
    return true;

}

//
//
// 2. GTK theme
//
static bool get_gtk_theme_config(const std::string_view path, Theme::Theme_t& theme)
{
    std::ifstream f(path.data(), std::ios::in);
    if (!f.is_open())
        return false;

    std::string    line;
    std::uint16_t  iter_index = 0;
    while (std::getline(f, line) && iter_index < 5)
    {
        if (hasStart(line, "gtk-theme-name="))
            getFileValue(iter_index, line, theme.gtk_theme_name, "gtk-theme-name="_len);

        else if (hasStart(line, "gtk-icon-theme-name="))
            getFileValue(iter_index, line, theme.gtk_icon_theme, "gtk-icon-theme-name="_len);

        else if (hasStart(line, "gtk-font-name="))
            getFileValue(iter_index, line, theme.gtk_font, "gtk-font-name="_len);
    }

    if (theme.gtk_font == MAGIC_LINE || theme.gtk_theme_name == MAGIC_LINE ||
        theme.gtk_icon_theme == MAGIC_LINE)
        return false;

    return true;
}

static void get_gtk_theme_settings(const std::string_view de_name, Theme::Theme_t& theme)
{
    debug("calling {}", __PRETTY_FUNCTION__);

    if (theme.gtk_theme_name == MAGIC_LINE || theme.gtk_theme_name.empty())
    {
        const char* gtk_theme_env = std::getenv("GTK_THEME");

        if (gtk_theme_env)
            theme.gtk_theme_name = gtk_theme_env;
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

static void get_gtk_theme_from_configs(const std::uint8_t ver, const std::string_view de_name, Theme::Theme_t& theme)
{
    if (get_gtk_theme_config(fmt::format("{}/gtk-{}.0/settings.ini", configDir, ver), theme))
        return;

    if (get_gtk_theme_config(fmt::format("{}/gtk-{}.0/gtkrc", configDir, ver), theme))
        return;

    if (get_gtk_theme_config(fmt::format("{}/gtkrc-{}.0", configDir, ver), theme))
        return;

    if (get_gtk_theme_config(fmt::format("{}/.gtkrc-{}.0", std::getenv("HOME"), ver), theme))
        return;

    get_gtk_theme_settings(de_name, theme);
}

static void get_de_gtk_theme(const std::string_view de_name, const std::uint8_t ver, Theme::Theme_t& theme)
{
    switch (fnv1a16::hash(str_tolower(de_name.data())))
    {
        case "xfce"_fnv1a16:
        case "xfce4"_fnv1a16:
            {
                debug("calling {} and getting info on xfce4", __PRETTY_FUNCTION__);
                const std::string& path = configDir + "/xfce4/xfconf/xfce-perchannel-xml/xsettings.xml";
                std::ifstream      f(path, std::ios::in);
                if (!f.is_open())
                {
                    get_gtk_theme_from_configs(ver, de_name, theme);
                    return;
                }

                std::string buffer((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                buffer.push_back('\0');

                rapidxml::xml_document<> doc;
                doc.parse<0>(&buffer[0]);
                rapidxml::xml_node<>* net_node = doc.first_node("channel")->first_node("property");
                for (; net_node && std::string(net_node->first_attribute("name")->value()) != "Net"; net_node = net_node->next_sibling("property"));

                rapidxml::xml_node<>* theme_node = net_node->first_node("property");
                unsigned short        iter_index = 0;
                for (; theme_node && iter_index < 2; theme_node = theme_node->next_sibling())
                {
                    if (std::string(theme_node->first_attribute("name")->value()) == "ThemeName" &&
                        theme_node->first_attribute("value"))
                    {
                        theme.gtk_theme_name = theme_node->first_attribute("value")->value();
                        iter_index++;
                    }
                    else if (std::string(theme_node->first_attribute("name")->value()) == "IconThemeName" &&
                             theme_node->first_attribute("value"))
                    {
                        theme.gtk_icon_theme = theme_node->first_attribute("value")->value();
                        iter_index++;
                    }
                }

                for (; net_node && std::string(net_node->first_attribute("name")->value()) != "Gtk"; net_node = net_node->next_sibling("property"));
                theme_node = net_node->first_node("property");

                for (; theme_node; theme_node = theme_node->next_sibling())
                {
                    if (std::string(theme_node->first_attribute("name")->value()) == "FontName" &&
                        theme_node->first_attribute("value"))
                    {
                        theme.gtk_font = theme_node->first_attribute("value")->value();
                        break;
                    }
                }

            }
        break;
        default:
            get_gtk_theme_from_configs(ver, de_name, theme);
    }
}

static void get_gtk_theme(const bool dont_query_dewm, const std::uint8_t ver, const std::string_view de_name,
                          Theme::Theme_t& theme)
{
    if (dont_query_dewm)
    {
        get_gtk_theme_from_configs(ver, de_name, theme);
        return;
    }

    get_de_gtk_theme(de_name, ver, theme);
}

// clang-format off
Theme::Theme(const std::uint8_t ver, systemInfo_t& queried_themes, std::vector<std::string>& queried_themes_names,
             const std::string& theme_name_version)
            : m_queried_themes(queried_themes),
              m_theme_name_version(theme_name_version)
{
    debug("Constructing {}", __func__);

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

    get_gtk_theme(query_user.m_bDont_query_dewm, ver, m_wmde_name, m_theme_infos);

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
Theme::Theme(systemInfo_t& queried_themes) : m_queried_themes(queried_themes)
{
    static bool done = false;
    if (hasStart(query_user.term_name(), "/dev") || done)
        return;

    const std::string& wm_name = query_user.wm_name(query_user.m_bDont_query_dewm, query_user.term_name());
    const std::string& de_name = query_user.de_name(query_user.m_bDont_query_dewm, query_user.term_name(), wm_name);

    if (((de_name != MAGIC_LINE && wm_name != MAGIC_LINE) &&
         de_name == wm_name) || de_name == MAGIC_LINE)
        m_wmde_name = wm_name;
    else
        m_wmde_name = de_name;

    if (get_de_cursor(m_wmde_name, m_theme_infos)){}
    else if (get_cursor_from_gtk_configs(4, m_wmde_name, m_theme_infos)){}
    else if ((get_cursor_from_gtk_configs(3, m_wmde_name, m_theme_infos))){}
    else if (get_cursor_from_gtk_configs(2, m_wmde_name, m_theme_infos)){}
    else if (get_cursor_xresources(m_theme_infos)){}
    else get_cursor_gsettings(m_wmde_name, m_theme_infos);

    done = true;
}

std::string Theme::gtk_theme()
{ return getInfoFromName(m_queried_themes, m_theme_name_version, "theme-name"); }

std::string Theme::gtk_icon_theme()
{ return getInfoFromName(m_queried_themes, m_theme_name_version, "icon-theme-name"); }

std::string Theme::gtk_font()
{ return getInfoFromName(m_queried_themes, m_theme_name_version, "font-name"); }

std::string& Theme::cursor()
{ return m_theme_infos.cursor; }

std::string& Theme::cursor_size()
{ return m_theme_infos.cursor_size; }
