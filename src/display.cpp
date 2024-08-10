/* Implementation of the system behind displaying/rendering the information */

#include "display.hpp"

#include <fstream>
#include <iostream>
#include <vector>

#include "config.hpp"
#include "fmt/core.h"
#include "fmt/ranges.h"
#include "parse.hpp"
#include "query.hpp"
#include "fmt/core.h"
#include "fmt/ranges.h"
#include "util.hpp"

// listen, it supposed to be only in Display::render, since only there is used.
// But at the same time, I like returning everything by reference if possible
// :)
std::vector<std::string> asciiArt;

std::string Display::detect_distro(Config& config)
{
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

std::vector<std::string>& Display::render(Config& config, colors_t& colors, const bool already_analyzed_file,
                                          const std::string_view path)
{
    systemInfo_t systemInfo{};

//#ifdef CF_UNIX
    if (!config.m_display_distro && 
        !config.m_disable_source && 
        !config.source_path.empty()) 
    {
        if (!config.m_custom_distro.empty())
            die("You need to specify if either using a custom distro ascii art OR a custom source path");
    }
//#endif

    debug("path = {}", path);

    std::ifstream file;
    std::ifstream fileToAnalyze;  // Input iterators are invalidated when you advance them. both have same path
    if (!config.m_disable_source)
    {
        file.open(path.data(), std::ios::binary);
        fileToAnalyze.open(path.data(), std::ios::binary);
        if (!file.is_open() || !fileToAnalyze.is_open())
            die("Could not open ascii art file \"{}\"", path);
    }

    std::string         line;
    std::vector<size_t> pureAsciiArtLens;
    int                 maxLineLength = -1;

    // first check if the file is an image
    // without even using the same library that "file" uses
    // No extra bloatware nice
    if (!already_analyzed_file)
    {
        debug("Display::render() analyzing file");
        unsigned char buffer[16];
        fileToAnalyze.read((char*)(&buffer[0]), sizeof(buffer));
        if (is_file_image(buffer))
            die("The source file '{}' is a binary file.\n"
                "Please currently use the GUI mode for rendering the image/gif (use -h for more details)",
                path);
    }

    for (int i = 0; i < config.logo_padding_top; i++)
        asciiArt.push_back("");

    while (std::getline(file, line))
    {
        std::string pureOutput;

#ifdef CF_WINDOWS
        if (hasEnding(line, "\r")) {
            line.pop_back();
        }
#endif
        std::string asciiArt_s = parse(line, systemInfo, pureOutput, config, colors, false);
        asciiArt_s += config.gui ? "" : NOCOLOR;

        asciiArt.push_back(asciiArt_s);
        size_t pureOutputLen = pureOutput.length();

        // shootout to my bf BurntRanch that helped me with this whole project
        // with the parsing and addValueFromModule()
        // and also fixing the problem with calculating the aligniment
        // with unicode characters
        size_t remainingUnicodeChars = 0;
        for (unsigned char c : pureOutput)
        {
            if (remainingUnicodeChars > 0)
            {
                remainingUnicodeChars--;
                continue;
            }
            // debug("c = {:c}", c);
            if (c > 127)
            {
                remainingUnicodeChars = 2;
                pureOutputLen -= 2;
            }
        }

        if (static_cast<int>(pureOutputLen) > maxLineLength)
            maxLineLength = static_cast<int>(pureOutputLen);

        pureAsciiArtLens.push_back(pureOutputLen);
    }

    if (config.m_print_logo_only)
        return asciiArt;

    for (std::string& layout : config.layouts)
    {
        std::string _;
        layout = parse(layout, systemInfo, _, config, colors, true);
    }

    // erase each element for each instance of MAGIC_LINE
    std::erase_if(config.layouts, [](const std::string_view str) { return str.find(MAGIC_LINE) != std::string::npos; });

    size_t i;
    for (i = 0; i < config.layouts.size(); i++)
    {
        size_t origin = 0;

        if (i < asciiArt.size())
        {
            config.layouts.at(i).insert(0, asciiArt.at(i));
            origin = asciiArt.at(i).length();
        }

        size_t spaces = (maxLineLength + (config.m_disable_source ? 1 : config.offset)) -
                        (i < asciiArt.size() ? pureAsciiArtLens.at(i) : 0);

        debug("spaces: {}", spaces);

        for (size_t j = 0; j < spaces; j++)
            config.layouts.at(i).insert(origin, " ");

        config.layouts.at(i) += config.gui ? "" : NOCOLOR;
    }

    if (i < asciiArt.size())
        config.layouts.insert(config.layouts.end(), asciiArt.begin() + i, asciiArt.end());

    return config.layouts;
}

void Display::display(std::vector<std::string>& renderResult)
{
    // for loops hell nah
    fmt::println("{}", fmt::join(renderResult, "\n"));
}
