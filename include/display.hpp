#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include "util.hpp"
#include <string>
#include <magic.h>
#include <vector>

#ifndef CUSTOMFETCH_DATA_DIR
#define CUSTOMFETCH_DATA_DIR "/usr/share/customfetch"
#endif

namespace Display {

std::vector<std::string>& render(std::string reset_fgcolor = "");
void display(std::vector<std::string>& renderResult);
std::string detect_distro();

}

#endif
