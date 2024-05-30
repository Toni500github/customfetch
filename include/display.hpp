#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include "util.hpp"
#include <string>
#include <vector>

namespace Display {

std::vector<std::string> render(systemInfo_t& systemInfo);
void display(std::vector<std::string> renderResult, systemInfo_t& systemInfo);

}

#endif
