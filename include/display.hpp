#ifndef _DISPLAY_HPP
#define _DISPLAY_HPP

#include <string>
#include <vector>

#include "config.hpp"

namespace Display
{

std::vector<std::string>& render(Config& config, colors_t& colors, const bool already_analyzed_path,
                                 const std::string_view path);

void        display(const std::vector<std::string>& renderResult);
std::string detect_distro(const Config& config);

}  // namespace Display

#endif
