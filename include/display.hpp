#ifndef _DISPLAY_HPP
#define _DISPLAY_HPP

#include <string>
#include <vector>

#include "config.hpp"

namespace Display
{

/*
 * Render the layout along side the ASCII art and return the vector
 * @param config The config class
 * @param colors The colors
 * @param already_analyzed_path If already checked that the source path is not a binary file
 * @param path Path to source file
 */
std::vector<std::string> render(const Config& config, const colors_t& colors, const bool already_analyzed_path,
                                const std::string_view path);

/*
 * Display the ascii art and layout
 * @param renderResult The rendered vector usually by Display::render()
 */
void display(const std::vector<std::string>& renderResult);

/*
 * Detect the distro you are using and return the path to the ASCII art
 * @param config The config class
 */
std::string detect_distro(const Config& config);

}  // namespace Display

#endif
