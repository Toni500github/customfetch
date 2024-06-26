#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include "util.hpp"
#include "config.hpp"

#include <string>
#include <magic.h>
#include <vector>

namespace Display {

std::vector<std::string>& render(Config& config, colors_t& colors);
void display(std::vector<std::string>& renderResult);
std::string detect_distro(Config& config);

}

#endif
