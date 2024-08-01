#include <algorithm>
#include <cstdint>
#include "query.hpp"
#include "switch_fnv1a.hpp"
#include "util.hpp"
#include "rapidxml-1.13/rapidxml.hpp"

using namespace Query;

std::string configDir = getHomeConfigDir();

static void get_gtk_theme_config(const std::string_view path, Theme::Theme_t& theme)
{
    std::ifstream f(path.data(), std::ios::in);
    if (!f.is_open())
    {
        theme.gtk_icon_theme = UNKNOWN;
        theme.gtk_theme_name = UNKNOWN;
        return;
    }

    std::string line;
    static unsigned short iter_index = 0;
    while (std::getline(f, line) && iter_index < 2)
    {
        if (hasStart(line, "gtk-theme-name="))
        {
            theme.gtk_theme_name = line.substr("gtk-theme-name="_len);
            iter_index++;
        }
        else if (hasStart(line, "gtk-icon-theme-name="))
        {
            theme.gtk_icon_theme = line.substr("gtk-icon-theme-name="_len);
            iter_index++;
        }
    }

    // remove double quotes (if any)
    // man gadamn if this is verbose as hell
    theme.gtk_theme_name.erase(std::remove(theme.gtk_theme_name.begin(), theme.gtk_theme_name.end(), '\"'), theme.gtk_theme_name.end());
    theme.gtk_icon_theme.erase(std::remove(theme.gtk_icon_theme.begin(), theme.gtk_icon_theme.end(), '\"'), theme.gtk_icon_theme.end());

}

static bool things_are_set(Theme::Theme_t& theme)
{
    return theme.gtk_icon_theme != UNKNOWN && theme.gtk_theme_name != UNKNOWN;
}

static void get_gtk_theme_settings(const std::string_view de_name, Theme::Theme_t& theme)
{
    debug("calling {}", __PRETTY_FUNCTION__);

    if (theme.gtk_theme_name == MAGIC_LINE || theme.gtk_theme_name == UNKNOWN)
    {
        char *gtk_theme_env = getenv("GTK_THEME");

        if (gtk_theme_env)
            theme.gtk_theme_name = gtk_theme_env;
    }

    auto hash = fnv1a32::hash(str_tolower(de_name.data()));
    
    if (theme.gtk_theme_name == MAGIC_LINE || theme.gtk_theme_name == UNKNOWN)
    {
        theme.gtk_theme_name.clear();
        switch (hash)
        {           
            case "cinnamon"_fnv1a32:
                read_exec({"gsettings", "get", "org.cinnamon.desktop.interface", "gtk-theme"}, theme.gtk_theme_name); break;
            case "mate"_fnv1a32:
                read_exec({"gsettings", "get", "org.mate.interface", "gtk-theme"}, theme.gtk_theme_name); break;

            case "gnome"_fnv1a32:
            case "budgie"_fnv1a32:
            case "unity"_fnv1a32:
            default:
                read_exec({"gsettings", "get", "org.gnome.desktop.interface", "gtk-theme"}, theme.gtk_theme_name);
        }
    }

    if (theme.gtk_icon_theme == MAGIC_LINE || theme.gtk_icon_theme == UNKNOWN)
    {
        theme.gtk_icon_theme.clear();
        switch (hash)
        {           
            case "cinnamon"_fnv1a32:
                read_exec({"gsettings", "get", "org.cinnamon.desktop.interface", "icon-theme"}, theme.gtk_icon_theme); break;
            case "mate"_fnv1a32:
                read_exec({"gsettings", "get", "org.mate.interface", "icon-theme"}, theme.gtk_icon_theme); break;

            case "gnome"_fnv1a32:
            case "budgie"_fnv1a32:
            case "unity"_fnv1a32:
            default:
                read_exec({"gsettings", "get", "org.gnome.desktop.interface", "icon-theme"}, theme.gtk_icon_theme);
        }
    }

    theme.gtk_theme_name.erase(std::remove(theme.gtk_theme_name.begin(), theme.gtk_theme_name.end(), '\''), theme.gtk_theme_name.end());
    theme.gtk_icon_theme.erase(std::remove(theme.gtk_icon_theme.begin(), theme.gtk_icon_theme.end(), '\''), theme.gtk_icon_theme.end());
}

static void get_gtk_theme_from_configs(const std::uint8_t ver, const std::string_view de_name, Theme::Theme_t& theme)
{
    get_gtk_theme_config(fmt::format("{}/gtk-{}.0/settings.ini", configDir, ver), theme);
    if (things_are_set(theme))
        return;

    get_gtk_theme_config(fmt::format("{}/gtk-{}.0/gtkrc", configDir, ver), theme);
    if (things_are_set(theme))
        return;

    get_gtk_theme_config(fmt::format("{}/.gtkrc-{}.0", std::getenv("HOME"), ver), theme);
    if (things_are_set(theme))
        return;

    get_gtk_theme_settings(de_name, theme);
}

static void get_de_gtk_theme(const std::string_view de_name, const std::uint8_t ver, Theme::Theme_t& theme)
{
    switch (fnv1a32::hash(str_tolower(de_name.data()))) {
        case "xfce"_fnv1a32:
        case "xfce4"_fnv1a32:
            {
                debug("calling {} and getting info on xfce4", __PRETTY_FUNCTION__);
                const std::string& path = configDir + "/xfce4/xfconf/xfce-perchannel-xml/xsettings.xml";
                std::ifstream f(path, std::ios::in);
                if (!f.is_open())
                { get_gtk_theme_from_configs(ver, de_name, theme); return; }

                std::string buffer((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                buffer.push_back('\0');

                rapidxml::xml_document<> doc;
                doc.parse<0>(&buffer[0]);
                rapidxml::xml_node<> *net_node = doc.first_node("channel")->first_node("property");
                while (net_node && std::string(net_node->first_attribute("name")->value()) != "Net")
                    net_node = net_node->next_sibling("property");

                rapidxml::xml_node<> *theme_node = net_node->first_node("property");
                unsigned short iter_index = 0;
                while (theme_node && iter_index < 2)
                {
                    if (std::string(theme_node->first_attribute("name")->value()) == "ThemeName")
                    {
                        theme.gtk_theme_name = theme_node->first_attribute("value")->value();
                        iter_index++;
                    }
                    else if (std::string(theme_node->first_attribute("name")->value()) == "IconThemeName")
                    {
                        theme.gtk_icon_theme = theme_node->first_attribute("value")->value();
                        iter_index++;
                    }

                    theme_node = theme_node->next_sibling();
                }
            } break;
        default:
            get_gtk_theme_from_configs(ver, de_name, theme);
    }
}

static void get_gtk_theme(const bool dont_query_dewm, const std::uint8_t ver, const std::string_view de_name, Theme::Theme_t& theme)
{
    if (dont_query_dewm)
    { 
        get_gtk_theme_from_configs(ver, de_name, theme);
        return;
    }

    get_de_gtk_theme(de_name, ver, theme);
}

// clang-format off
Theme::Theme(const std::uint8_t ver)
{
    debug("Constructing {}", __func__);
    if (!m_bInit)
    {
        const std::string& _wm_name = wm_name(m_bDont_query_dewm, term_name());
        const std::string& _de_name = de_name(m_bDont_query_dewm, term_name(), _wm_name);

        if ((_de_name != MAGIC_LINE && _wm_name != MAGIC_LINE) &&
             _de_name == _wm_name)
        {
            get_gtk_theme(m_bDont_query_dewm, ver, _wm_name, m_theme_infos);
        }
        else
        {
            get_gtk_theme(m_bDont_query_dewm, ver, _de_name, m_theme_infos);
        }

        m_bInit = true;
    }
}

std::string& Theme::gtk_theme()
{ return m_theme_infos.gtk_theme_name; }

std::string& Theme::gtk_icon_theme()
{ return m_theme_infos.gtk_icon_theme; }
