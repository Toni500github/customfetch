#pragma once

#include <string>
#include <vector>
#include "config.hpp"

// Flexible Unicode box-drawing helper.
// Given a parsed layout (one string per row) and config options,
// expands $fill macros and adds borders/column padding.
// Returns the final list of strings ready to be printed.
std::vector<std::string> apply_box_drawing(std::vector<std::string> layout,
                                           const Config& config) noexcept;
