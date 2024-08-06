#include "platform.hpp"

#ifdef CF_WINDOWS

#include "query.hpp"
#include "util.hpp"

using namespace Query;

Theme::Theme(const std::uint8_t ver, std::vector<std::string>& queried_themes,
             const std::string_view theme_name_version)
{
    debug("Constructing Theme");
}

std::string& Theme::gtk_theme() 
{ return m_theme_infos.gtk_theme_name; }

std::string& Theme::gtk_icon_theme()
{ return m_theme_infos.gtk_icon_theme; }

std::string& Theme::gtk_font()
{ return m_theme_infos.gtk_font; }

std::string& Theme::gtk_cursor()
{ return m_theme_infos.gtk_cursor; }

#endif
