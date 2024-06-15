#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include "util.hpp"
#include <string>
#include <magic.h>
#include <vector>

namespace Display {

std::vector<std::string>& render(std::string reset_fgcolor = "");
void display(std::vector<std::string>& renderResult);

}

#endif
