/* Implementation of the system behind displaying/rendering the information */
#include "display.hpp"
#include "config.hpp"
#include "util.hpp"
#include <algorithm>
#include <fstream>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <memory>

std::vector<std::string>& Display::render() {
    systemInfo_t systemInfo{};

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
                // die("Specific includes are not supported at this time.");
            default:
                die("Include has too many namespaces!");
        }
    }

    for (std::string& layout : config.layouts) {
        std::unique_ptr<std::string> _;
        layout = parse(layout, systemInfo, _);
    }

    std::ifstream file(config.ascii_art_path, std::ios_base::binary);
    if (!file.is_open())
        if (!config.disable_ascii_art)
            die("Could not open ascii art file \"{}\"", config.ascii_art_path);
    
    std::string line;
    std::vector<std::string> asciiArt;
    std::vector<std::unique_ptr<std::string>> pureAsciiArt;
    int maxLineLength = -1;
    
    while (std::getline(file, line)) {
        std::unique_ptr<std::string> pureOutput = std::make_unique<std::string>();
        std::string asciiArt_s = parse(line, systemInfo, pureOutput);
        asciiArt_s += NOCOLOR;

        asciiArt.push_back(asciiArt_s);

        if ((int)pureOutput->length() > maxLineLength)
            maxLineLength = pureOutput->length();

        pureAsciiArt.push_back(std::move(pureOutput));
    }

    size_t i;
    for (i = 0; i < config.layouts.size(); i++) {
        size_t origin = 0;

        if (i < asciiArt.size()) {
            config.layouts.at(i).insert(0, asciiArt.at(i));
            origin = asciiArt.at(i).length();
        }

        size_t spaces = (maxLineLength + (config.disable_ascii_art ? 1 : config.offset)) - (i < asciiArt.size() ? pureAsciiArt.at(i)->length() : 0);
        for (size_t j = 0; j < spaces; j++)
            config.layouts.at(i).insert(origin, " ");
        
        config.layouts.at(i) += NOCOLOR;
    }

    if (i < asciiArt.size())
        config.layouts.insert(config.layouts.end(), asciiArt.begin() + i, asciiArt.end());

    return config.layouts;
}

void Display::display(std::vector<std::string>& renderResult) {
    fmt::println("{}", fmt::join(renderResult, "\n"));
}
