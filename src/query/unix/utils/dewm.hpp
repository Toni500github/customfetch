#ifndef _WM_HPP
#define _WM_HPP

#include <string>
#include <string_view>

std::string parse_de_env(void) noexcept;
std::string prettify_wm_name(const std::string_view name) noexcept;
std::string get_mate_version();
std::string get_xfce4_version();
std::string get_cinnamon_version();
std::string get_kwin_version();


#endif
