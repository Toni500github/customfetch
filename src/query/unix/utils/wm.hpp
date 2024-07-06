#ifndef _WM_HPP
#define _WM_HPP

#include <string>
#include <string_view>

std::string parse_wm_env(void);
std::string prettify_wm_name(const std::string_view name);

#endif
