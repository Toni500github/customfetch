#include <array>
#include <filesystem>
#include "query.hpp"
#include "switch_fnv1a.hpp"
#include "util.hpp"
#include "rapidxml-1.13/rapidxml.hpp"

using namespace Query;

static std::string configDir = getHomeConfigDir();

static Theme::Theme_t get_config_gtk3_theme()
{
    Theme::Theme_t ret;

    std::string path = configDir + "/gtk-3.0/settings.ini";
    
    if (!std::filesystem::exists(path))
    {
        constexpr std::array<std::string_view, 3> paths = {"/etc/gtk-3.0/settings.ini", "/usr/local/share/gtk-3.0/settings.ini", "/usr/share/gtk-3.0/settings.ini"};
        for (std::string_view str : paths)
        {
            if (std::filesystem::exists(str))
            {
                path = str;
                break;
            }
        }
    }

    std::ifstream f(path, std::ios::in);
    std::string line;
    static u_short iter_index = 0;
    while (std::getline(f, line) && iter_index < 2)
    {
        if (hasStart(line, "gtk-theme-name=")) 
        {
            iter_index++;
            ret.gtk3_theme_name = line.substr("gtk-theme-name="_len);
        }

        else if (hasStart(line, "gtk-icon-theme-name="))
        {
            iter_index++;
            ret.gtk3_icon_theme = line.substr("gtk-icon-theme-name="_len);
        }
    }
    
    if (ret.gtk3_theme_name == MAGIC_LINE || ret.gtk3_theme_name.empty())
    {
        char *gtk_theme_env = getenv("GTK_THEME");

        if (gtk_theme_env)
            ret.gtk3_theme_name = gtk_theme_env;
    }

    return ret;
}

static Theme::Theme_t get_de_gtk_theme(const std::string_view de_name)
{
    Theme::Theme_t ret;

    switch (fnv1a32::hash(de_name)) {
        case "xfce"_fnv1a32:
        case "xfce4"_fnv1a32:
            {
                const std::string& path = configDir + "/xfce4/xfconf/xfce-perchannel-xml/xsettings.xml";
                std::ifstream f(path, std::ios::in);
                if (!f.is_open())
                    return get_config_gtk3_theme();
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
                        ret.gtk3_theme_name = theme_node->first_attribute("value")->value();
                        iter_index++;
                    }
                    else if (std::string(theme_node->first_attribute("name")->value()) == "IconThemeName")
                    {
                        ret.gtk3_icon_theme = theme_node->first_attribute("value")->value();
                        iter_index++;
                    }

                    theme_node = theme_node->next_sibling();
                }
            }
        default:
            return get_config_gtk3_theme();
    }

    return ret;
}

static Theme::Theme_t get_gtk_theme(const bool dont_query_dewm, const std::string_view de_name,
                                 const std::string_view wm_name)
{
    if (dont_query_dewm)
        return get_config_gtk3_theme();

    if (((de_name != MAGIC_LINE && wm_name != MAGIC_LINE) ||
         (de_name != UNKNOWN && wm_name != UNKNOWN)) &&
        de_name == wm_name)
        return get_config_gtk3_theme();
    else
        return get_de_gtk_theme(str_tolower(de_name.data()));

}

// clang-format off
Theme::Theme()
{
    debug("Constructing {}", __func__);
    if (!m_bInit)
    {
        // this is so damn shitty just for displaying WM and DE having the same name, smh
        m_theme_infos = get_gtk_theme(m_bDont_query_dewm, 
                                      de_name(m_bDont_query_dewm, term_name(), wm_name(m_bDont_query_dewm, term_name())),
                                      wm_name(m_bDont_query_dewm, term_name()));
        m_bInit = true;
    }
}

std::string Theme::gtk3_theme()
{ return m_theme_infos.gtk3_theme_name; }

std::string Theme::gtk3_icon_theme()
{ return m_theme_infos.gtk3_icon_theme; }
