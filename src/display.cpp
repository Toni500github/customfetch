/* Implementation of the system behind displaying/rendering the information */

#include "display.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <termios.h>

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
#include "stb_image.h"
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

static std::vector<std::string> render_with_image(const Config& config, const colors_t& colors,
                                                  const std::string_view path)
{
    std::string              distro_path{ Display::detect_distro(config) };
    systemInfo_t             systemInfo{};
    std::vector<std::string> layout{ config.layout };

    int image_width, image_height, channels;

    // load the image and get its width and height
    unsigned char* img = stbi_load(path.data(), &image_width, &image_height, &channels, 0);

    if (img)
        stbi_image_free(img);
    else
        die("Unable to load image '{}'", config.source_path);

    if (!config.ascii_logo_type.empty())
    {
        const size_t& pos = distro_path.rfind('.');

        if (pos != std::string::npos)
            distro_path.insert(pos, "_" + config.ascii_logo_type);
        else
            distro_path += "_" + config.ascii_logo_type;
    }

    // this is just for parse() to auto add the distro colors
    std::ifstream file(distro_path, std::ios::binary);
    std::string line, _;
    
    while (std::getline(file, line))
        parse(line, systemInfo, _, config, colors, false);

    for (std::string& layout : layout)
        layout = parse(layout, systemInfo, _, config, colors, true);

    // erase each element for each instance of MAGIC_LINE
    layout.erase(std::remove_if(layout.begin(), layout.end(),
                                [](const std::string_view str) { return str.find(MAGIC_LINE) != std::string::npos; }),
                 layout.end());

    for (size_t i = 0; i < layout.size(); i++)
        for (size_t _ = 0; _ < config.offset + 40; _++)  // I use _ because we don't need it
            layout.at(i).insert(0, " ");

    return layout;
}

// https://stackoverflow.com/a/50888457
// with a little C++ modernizing
bool get_pos(std::uint32_t& y, std::uint32_t& x)
{
    std::array<char, 32> buf;
    int  ret, i, pow;
    char ch;

    y = 0;
    x = 0;

    struct termios term, restore;

    tcgetattr(0, &term);
    tcgetattr(0, &restore);
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(0, TCSANOW, &term);

    write(1, "\033[6n", 4);

    for (i = 0, ch = 0; ch != 'R'; i++)
    {
        ret = read(0, &ch, 1);
        if (!ret)
        {
            tcsetattr(0, TCSANOW, &restore);
            fprintf(stderr, "getpos: error reading response!\n");
            return false;
        }
        buf[i] = ch;
        debug("buf[{}]: \t{} \t{}", i, ch, ch);
    }

    if (i < 2)
    {
        tcsetattr(0, TCSANOW, &restore);
        return false;
    }

    for (i -= 2, pow = 1; buf[i] != ';'; i--, pow *= 10)
        x = x + (buf[i] - '0') * pow;

    for (i--, pow = 1; buf[i] != '['; i--, pow *= 10)
        y = y + (buf[i] - '0') * pow;

    tcsetattr(0, TCSANOW, &restore);
    return true;
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

    debug("Display::render path = {}", path);

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
        std::array<unsigned char, 32> buffer;
        fileToAnalyze.read(reinterpret_cast<char*>(&buffer.at(0)), buffer.size());
        if (is_file_image(buffer.data()))
        {
            std::uint32_t x = 0, y = 0;
            get_pos(x, y);
            fmt::print("\033[{};{}H", x, y);
            if (config.m_image_backend == "kitty")
                taur_exec({ "kitty", "+kitten", "icat", "--align", "left", path.data() });

            return render_with_image(config, colors, path);
        }
        /*    die("The source file '{}' is a binary file.\n"
                "Please currently use the GUI mode for rendering the image/gif (use -h for more details)",
                path);*/
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
