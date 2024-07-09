#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include "config.hpp"

#include <string>
#include <vector>

namespace Display {

std::vector<std::string> render(Config& config, colors_t& colors, bool already_analyzed_path);
void display(std::vector<std::string>& renderResult);
std::string detect_distro(Config& config);

}

#endif
