/* Implementation of the system behind displaying/rendering the information */

#include "display.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "config.hpp"
#include "fmt/core.h"
#include "parse.hpp"
#include "query.hpp"
#include "util.hpp"

std::string Display::detect_distro(const Config& config)
{
    debug("/etc/os-release = \n{}", shell_exec("cat /etc/os-release"));

    if (!config.m_custom_distro.empty())
    {
        return fmt::format("{}/ascii/{}.txt", config.data_dir, config.m_custom_distro);
    }
    else
    {
        Query::System system;
        std::string   format;

        format = fmt::format("{}/ascii/{}.txt", config.data_dir, str_tolower(system.os_id()));
        if (std::filesystem::exists(format))
            return format;

        format = fmt::format("{}/ascii/{}.txt", config.data_dir, str_tolower(system.os_name()));
        if (std::filesystem::exists(format))
            return format;

        return fmt::format("{}/ascii/linux.txt", config.data_dir);
    }
}

std::vector<std::string> Display::render(const Config& config, const colors_t& colors, const bool already_analyzed_file,
                                         const std::string_view path)
{
    systemInfo_t             systemInfo{};
    std::vector<std::string> asciiArt{}, layout{ config.layout };

    if (!config.m_display_distro && !config.m_disable_source && !config.source_path.empty())
    {
        if (!config.m_custom_distro.empty() && !config.gui)
            die("You need to specify if either using a custom distro ascii art OR a custom source path");
    }

    debug("path = {}", path);

    std::ifstream file;
    std::ifstream fileToAnalyze;  // both have same path
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
        std::array<unsigned char, 16> buffer;
        fileToAnalyze.read(reinterpret_cast<char*>(&buffer.at(0)), buffer.size());
        if (is_file_image(buffer.data()))
            die("The source file '{}' is a binary file.\n"
                "Please currently use the GUI mode for rendering the image/gif (use -h for more details)",
                path);
    }

    for (int i = 0; i < config.logo_padding_top; i++)
    {
        pureAsciiArtLens.push_back(0);
        asciiArt.push_back("");
    }

    for (int i = 0; i < config.layout_padding_top; i++)
    {
        layout.insert(layout.begin(), "");
    }

    while (std::getline(file, line))
    {
        std::string pureOutput;
        std::string asciiArt_s = parse(line, systemInfo, pureOutput, config, colors, false);
        asciiArt_s += config.gui ? "" : NOCOLOR;

        if (config.gui)
        {
            // check parse.cpp
            const size_t pos = asciiArt_s.rfind("$ </");
            if (pos != std::string::npos)
                asciiArt_s.replace(pos, 2, "$");
        }

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
        debug("asciiArt_s = {}", asciiArt_s);
    }

    if (config.m_print_logo_only)
        return asciiArt;

    std::string _;
    for (std::string& layout : layout)
        layout = parse(layout, systemInfo, _, config, colors, true);

    // erase each element for each instance of MAGIC_LINE
    layout.erase(std::remove_if(layout.begin(), layout.end(),
                                [](const std::string_view str) { return str.find(MAGIC_LINE) != std::string::npos; }),
                 layout.end());

    size_t i;
    for (i = 0; i < layout.size(); i++)
    {
        size_t origin = config.logo_padding_left;

        // The user-specified offset to be put before the logo
        for (size_t j = 0; j < config.logo_padding_left; j++)
            layout.at(i).insert(0, " ");

        if (i < asciiArt.size())
        {
            layout.at(i).insert(origin, asciiArt.at(i));
            origin += asciiArt.at(i).length();
        }

        const size_t& spaces = (maxLineLength + (config.m_disable_source ? 1 : config.offset)) -
                               (i < asciiArt.size() ? pureAsciiArtLens.at(i) : 0);

        debug("spaces: {}", spaces);

        for (size_t j = 0; j < spaces; j++)
            layout.at(i).insert(origin, " ");

        layout.at(i) += config.gui ? "" : NOCOLOR;
    }

    for (; i < asciiArt.size(); i++)
    {
        std::string line;

        for (size_t j = 0; j < config.logo_padding_left; j++)
            line += " ";

        line += asciiArt[i];

        layout.push_back(line);
    }

    return layout;
}

void Display::display(const std::vector<std::string>& renderResult)
{
    for (const std::string_view str : renderResult)
        fmt::println("{}", str);
}
