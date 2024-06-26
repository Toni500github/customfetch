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
#include <magic.h>
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

std::vector<std::string>& Display::render(Config& config, colors_t& colors) {
    systemInfo_t systemInfo{};

    // first check if the file is an image
    // using the same library that "file" uses
    // No extra bloatware nice
    if (!config.m_display_distro && !config.m_disable_source && config.m_custom_distro.empty()) {
        magic_t myt = magic_open(MAGIC_CONTINUE|MAGIC_ERROR|MAGIC_MIME);
        magic_load(myt, NULL);
        std::string file_type = magic_file(myt, config.source_path.c_str());
        if ((file_type.find("text") == std::string::npos) && !config.m_disable_source)
            die("The source file '{}' is a binary file. Please currently use the GUI mode for rendering the image (use -h for more details)", config.source_path);
        magic_close(myt);
    }

    for (std::string& include : config.includes) {
        std::vector<std::string> include_nodes = split(include, '.');

        switch (std::count(include.begin(), include.end(), '.')) 
        {   
            // only 1 element
            case 0:
                addModuleValues(systemInfo, include);
                break;
            case 1:
                addValueFromModule(systemInfo, include_nodes[0], include_nodes[1]);
                break;
            default:
                die("Include has too many namespaces!");
        }
    }

    for (std::string& layout : config.layouts) {
        layout = parse(layout, systemInfo, config, colors);
    }
    
    std::string path = config.m_display_distro ? detect_distro(config) : config.source_path;
    std::ifstream file(path, std::ios_base::binary);
    if (!file.is_open())
        if (!config.m_disable_source)
            die("Could not open ascii art file \"{}\"", path);

    if (config.m_disable_source)
        file.close();

    std::string line;
    std::vector<std::string> asciiArt;
    std::vector<std::string> pureAsciiArt;
    int maxLineLength = -1;
    
    while (std::getline(file, line)) {
        std::string pureOutput;
        std::string asciiArt_s = parse(line, systemInfo, pureOutput, config, colors);
        asciiArt_s += config.gui ? "" : NOCOLOR;

        asciiArt.push_back(asciiArt_s);

        if (static_cast<int>(pureOutput.length()) > maxLineLength)
            maxLineLength = pureOutput.length();

        pureAsciiArt.push_back(pureOutput);
    }
    
    debug("SkeletonAsciiArt = \n{}", fmt::join(pureAsciiArt, "\n"));
    debug("asciiArt = \n{}", fmt::join(asciiArt, "\n"));

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
