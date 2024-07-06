/* Implementation of the system behind displaying/rendering the information */
#include "display.hpp"
#include "config.hpp"
#include "util.hpp"
#include "parse.hpp"
#include "query.hpp"

#include <algorithm>
#include <fstream>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <iostream>

std::string Display::detect_distro(Config& config) {
    std::string file_path;
    
    debug("/etc/os-release = \n{}", shell_exec("cat /etc/os-release"));
    if (!config.m_custom_distro.empty()) 
    {
        file_path = fmt::format("{}/ascii/{}.txt", config.data_dir, config.m_custom_distro);
    } 
    else 
    {
        Query::System system;
        file_path = fmt::format("{}/ascii/{}.txt", config.data_dir, str_tolower(system.os_id()));
    }
    return file_path;
}

std::vector<std::string>& Display::render(Config& config, colors_t& colors, bool already_analyzed_file) {
    systemInfo_t systemInfo{};
    std::string path = config.m_display_distro ? detect_distro(config) : config.source_path;

    if (!config.m_display_distro && 
        !config.m_disable_source && 
        !config.source_path.empty()) 
    {
        path = config.source_path;
        if (!config.m_custom_distro.empty())
            die("You need to specify if either using a custom distro ascii art OR a custom source path");
    }

    debug("path = {}", path);

    for (std::string& include : config.includes) {
        addModuleValues(systemInfo, include);
    }

    for (std::string& layout : config.layouts) {
        layout = parse(layout, systemInfo, config, colors, true);
    }
    
    std::ifstream file;
    std::ifstream fileToAnalyze; // Input iterators are invalidated when you advance them. both have same path
    if (!config.m_disable_source) {
        file.open(path, std::ios::binary);
        fileToAnalyze.open(path, std::ios::binary);
        if (!file.is_open() || !fileToAnalyze.is_open())
            die("Could not open ascii art file \"{}\"", path);
    }

    std::string line;
    std::vector<std::string> asciiArt;
    std::vector<std::string> pureAsciiArt;
    int maxLineLength = -1;
    
    // first check if the file is an image
    // without even using the same library that "file" uses
    // No extra bloatware nice
    if (!already_analyzed_file)
    {
        debug("Display::render() analyzing file");
        unsigned char buffer[16];
        fileToAnalyze.read((char*) (&buffer[0]), sizeof(buffer));
        if (is_file_image(buffer))
                die("The source file '{}' is a binary file.\n"
                    "Please currently use the GUI mode for rendering the image/gif (use -h for more details)", path);
    }

    while (std::getline(file, line)) {
        std::string pureOutput;
        std::string asciiArt_s = parse(line, systemInfo, pureOutput, config, colors, false);
        asciiArt_s += config.gui ? "" : NOCOLOR;

        asciiArt.push_back(asciiArt_s);

        if (static_cast<int>(pureOutput.length()) > maxLineLength)
            maxLineLength = static_cast<int>(pureOutput.length());

        pureAsciiArt.push_back(pureOutput);
    }
    
    debug("SkeletonAsciiArt = \n{}", fmt::join(pureAsciiArt, "\n"));
    debug("asciiArt = \n{}", fmt::join(asciiArt, "\n"));
    
    // erase each element for each instance of MAGIC_LINE
    config.layouts.erase(std::remove_if(config.layouts.begin(), config.layouts.end(), 
                                        [](const std::string_view str) { return str.find(MAGIC_LINE) != std::string::npos; }
                                        ), config.layouts.end());

    size_t i;
    for (i = 0; i < config.layouts.size(); i++) {
        size_t origin = 0;

        if (i < asciiArt.size()) {
            config.layouts.at(i).insert(0, asciiArt.at(i));
            origin = asciiArt.at(i).length();
        }

        size_t spaces = (maxLineLength + (config.m_disable_source ? 1 : config.offset)) - (i < asciiArt.size() ? pureAsciiArt.at(i).length() : 0);

        debug("spaces: {}", spaces);

        for (size_t j = 0; j < spaces; j++)
            config.layouts.at(i).insert(origin, " ");
        
        config.layouts.at(i) += config.gui ? "" : NOCOLOR;
    }

    if (i < asciiArt.size())
        config.layouts.insert(config.layouts.end(), asciiArt.begin() + i, asciiArt.end());
    
    return config.layouts;
}

void Display::display(std::vector<std::string>& renderResult) {
    // for loops hell nah
    fmt::println("{}", fmt::join(renderResult, "\n"));
}
