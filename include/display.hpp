#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include "util.hpp"
#include <string>
#include <vector>

namespace Display {

std::vector<std::string>& render();
void display(std::vector<std::string>& renderResult);

}

#endif
