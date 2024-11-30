/*
 * Copyright 2024 Toni500git
 * 
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

// Implementation of the system behind displaying/rendering the information

#include "display.hpp"

#ifndef GUI_MODE
# define STB_IMAGE_IMPLEMENTATION
#endif

#include <pty.h>
#include <termios.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "config.hpp"
#include "fmt/core.h"
#include "fmt/format.h"
#include "parse.hpp"
#include "query.hpp"
#include "stb_image.h"
#include "utf8/checked.h"
#include "util.hpp"

std::string Display::detect_distro(const Config& config)
{
    debug("/etc/os-release = \n{}", read_shell_exec("cat /etc/os-release"));

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

        return config.data_dir + "/ascii/linux.txt";
    }
}

#if !ANDROID_APP
static std::vector<std::string> render_with_image(systemInfo_t& systemInfo, std::vector<std::string>& layout,
                                                  const Config& config, const colors_t& colors,
                                                  const std::string_view path, const std::uint16_t font_width,
                                                  const std::uint16_t font_height)
{
    int image_width, image_height, channels;

    // load the image and get its width and height
    unsigned char* img = stbi_load(path.data(), &image_width, &image_height, &channels, 0);

    if (!img)
        die("Unable to load image '{}'", path);
    
    stbi_image_free(img);

    std::string _;
    parse_args_t parse_args{ systemInfo, _, config, colors, true, true };
    for (std::string& line : layout)
    {
        line = parse(line, parse_args);
        if (!config.m_disable_colors)
            line.insert(0, NOCOLOR);
    }

    // erase each element for each instance of MAGIC_LINE
    layout.erase(std::remove_if(layout.begin(), layout.end(),
                                [](const std::string_view str) { return str.find(MAGIC_LINE) != std::string::npos; }),
                 layout.end());
        
    // took math from neofetch in get_term_size() and get_image_size(). seems to work nice
    const size_t width  = image_width / font_width;
    const size_t height = image_height / font_height;

    if (config.m_image_backend == "kitty")
        taur_exec({ "kitty", "+kitten", "icat", "--align", (config.logo_position == "top" ? "center" : config.logo_position), "--place", fmt::format("{}x{}@0x0", width, height),
                    path });
    else if (config.m_image_backend == "viu")
        taur_exec({ "viu", "-t", "-w", fmt::to_string(width), "-h", fmt::to_string(height), path });
    else
        die("The image backend '{}' isn't supported, only kitty and viu.\n"
            "Please currently use the GUI mode for rendering the image/gif (use -h for more details)",
            config.m_image_backend);

    if (config.logo_position == "top")
    {
        for (size_t _ = 0; _ < height + config.layout_padding_top; ++_)
            layout.insert(layout.begin(), "");

        return layout;
    }

    for (size_t i = 0; i < layout.size(); ++i)
        for (size_t _ = 0; _ < width + config.offset; ++_)
            layout.at(i).insert(0, " ");

    return layout;
}

// https://stackoverflow.com/a/50888457
// with a little C++ modernizing
static bool get_pos(int& y, int& x)
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
            die("getpos: error reading response!");
        }
        buf.at(i) = ch;
    }

    if (i < 2)
    {
        tcsetattr(0, TCSANOW, &restore);
        return false;
    }

    for (i -= 2, pow = 1; buf.at(i) != ';'; i--, pow *= 10)
        x = x + (buf.at(i) - '0') * pow;

    for (i--, pow = 1; buf.at(i) != '['; i--, pow *= 10)
        y = y + (buf.at(i) - '0') * pow;

    tcsetattr(0, TCSANOW, &restore);
    return true;
}
#endif

std::vector<std::string> Display::render(const Config& config, const colors_t& colors, const bool already_analyzed_file,
                                         const std::string_view path)
{
    systemInfo_t             systemInfo{};
    std::vector<std::string> asciiArt{}, layout{ config.m_args_layout.empty() ? config.layout : config.m_args_layout };

    debug("Display::render path = {}", path);

    bool          isImage = false;
    std::ifstream file;
    std::ifstream fileToAnalyze;  // both have same path
    if (!config.m_disable_source)
    {
        file.open(path.data(), std::ios::binary);
        fileToAnalyze.open(path.data(), std::ios::binary);
        //if (!file.is_open() || !fileToAnalyze.is_open())
          //  die("Could not open logo file \"{}\"", path);

        // first check if the file is an image
        // without even using the same library that "file" uses
        // No extra bloatware nice
        if (!already_analyzed_file)
        {
            debug("Display::render() analyzing file");
            std::array<unsigned char, 32> buffer;
            fileToAnalyze.read(reinterpret_cast<char*>(&buffer.at(0)), buffer.size());
            isImage = is_file_image(buffer.data());
        }
    }

    if (!config.m_display_distro && isImage)
    {
        std::string distro_path{ Display::detect_distro(config) };
        if (!config.ascii_logo_type.empty())
        {
            const size_t pos = distro_path.rfind('.');

            if (pos != std::string::npos)
                distro_path.insert(pos, "_" + config.ascii_logo_type);
            else
                distro_path += "_" + config.ascii_logo_type;
        }

        debug("{} distro_path = {}", __FUNCTION__, distro_path);

        // this is just for parse() to auto add the distro colors
        std::ifstream distro_file(distro_path);
        std::string   line, _;

        parse_args_t parse_args{ systemInfo, _, config, colors, false, true };

        while (std::getline(distro_file, line))
            parse(line, _, parse_args);
    }

    std::vector<size_t> pureAsciiArtLens;
    int                 maxLineLength = -1;

#if !ANDROID_APP
    if (isImage)
    {
        // clear screen
        write(1, "\33[H\33[2J", 7);

        struct winsize win;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &win);

        const std::uint16_t font_width  = win.ws_xpixel / win.ws_col;
        const std::uint16_t font_height = win.ws_ypixel / win.ws_row;

        // why... why reverse the cardinal coordinates..
        int y = 0, x = 0;
        get_pos(y, x);
        fmt::print("\033[{};{}H", y, x);

        return render_with_image(systemInfo, layout, config, colors, path, font_width, font_height);
    }
#else
    if (isImage)
    {
        die("images not allowed in the android widget at the moment");
    }
#endif

    for (int i = 0; i < config.logo_padding_top; i++)
    {
        pureAsciiArtLens.push_back(0);
        asciiArt.push_back("");
    }

    for (int i = 0; i < config.layout_padding_top; i++)
    {
        layout.insert(layout.begin(), "");
    }

    std::string line;
    while (std::getline(file, line))
    {
        std::string pureOutput;
        parse_args_t parse_args{ systemInfo, pureOutput, config, colors, false, true };

        std::string asciiArt_s = parse(line, parse_args);
        if (!config.m_disable_colors)
            asciiArt_s += config.gui ? "" : NOCOLOR;

        if (config.gui)
        {
            // check parse.cpp
            const size_t pos = asciiArt_s.rfind("$ </");
            if (pos != std::string::npos)
                asciiArt_s.replace(pos, 2, "$");
        }

        asciiArt.push_back(asciiArt_s);
        const size_t pureOutputLen = utf8::distance(pureOutput.begin(), pureOutput.end());

        if (static_cast<int>(pureOutputLen) > maxLineLength)
            maxLineLength = static_cast<int>(pureOutputLen);

        pureAsciiArtLens.push_back(pureOutputLen);
        debug("asciiArt_s = {}", asciiArt_s);
    }

    if (config.m_print_logo_only)
        return asciiArt;

    std::string _;
    parse_args_t parse_args{ systemInfo, _, config, colors, true, true };
    for (std::string& line : layout)
    {
        line = parse(line, parse_args);
        if (!config.gui && !config.m_disable_colors)
            line.insert(0, NOCOLOR);
    }

    // erase each element for each instance of MAGIC_LINE
    layout.erase(std::remove_if(layout.begin(), layout.end(),
                                [](const std::string_view str) { return str.find(MAGIC_LINE) != std::string::npos; }),
                 layout.end());

    if (config.logo_position == "top")
    {
        Display::display(asciiArt);
        return layout;
    }

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

        const size_t spaces = (maxLineLength + (config.m_disable_source ? 1 : config.offset)) -
                              (i < asciiArt.size() ? pureAsciiArtLens.at(i) : 0);

        debug("spaces: {}", spaces);

        for (size_t j = 0; j < spaces; j++)
            layout.at(i).insert(origin, " ");

        if (!config.m_disable_colors)
            layout.at(i) += config.gui ? "" : NOCOLOR;
    }

    for (; i < asciiArt.size(); i++)
    {
        std::string line;
        line.reserve(config.logo_padding_left + asciiArt.at(i).length());

        for (size_t j = 0; j < config.logo_padding_left; j++)
            line += ' ';

        line += asciiArt.at(i);

        layout.push_back(line);
    }

    return layout;
}

void Display::display(const std::vector<std::string>& renderResult)
{
    for (const std::string& str : renderResult)
        fmt::println("{}", str);
}
