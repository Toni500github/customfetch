#ifndef _BOXD_HPP
#define _BOXD_HPP

#include <string>
#include <vector>

#include "config.hpp"

std::vector<std::string> apply_box_drawing(std::vector<std::string> layout,
                                           const Config& config) noexcept;

#endif // _BOXD_HPP
