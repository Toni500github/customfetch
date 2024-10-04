/* Implementation of the system behind displaying/rendering the information */

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

        return fmt::format("{}/ascii/linux.txt", config.data_dir);
    }
}

static std::vector<std::string> render_with_image(systemInfo_t& systemInfo, std::vector<std::string>& layout,
                                                  const Config& config, const colors_t& colors, const std::string_view path,
                                                  const std::uint16_t font_width, const std::uint16_t font_height)
{
    int image_width, image_height, channels;

    // load the image and get its width and height
    unsigned char* img = stbi_load(path.data(), &image_width, &image_height, &channels, 0);

    if (img)
        stbi_image_free(img);
    else
        die("Unable to load image '{}'", path);

    std::string _;
    for (std::string& layout : layout)
        layout = parse(layout, systemInfo, _, config, colors, true);

    // erase each element for each instance of MAGIC_LINE
    layout.erase(std::remove_if(layout.begin(), layout.end(),
                                [](const std::string_view str) { return str.find(MAGIC_LINE) != std::string::npos; }),
                 layout.end());

    const size_t width  = image_width / font_width;
    const size_t height = image_height / font_height;

    if (config.m_image_backend == "kitty")
        taur_exec({ "kitty", "+kitten", "icat", "--align", "left", "--place", fmt::format("{}x{}@0x0", width, height),
                    path });
    else if (config.m_image_backend == "viu")
        taur_exec({ "viu", "-t", "-w", fmt::to_string(width), "-h", fmt::to_string(height), path });
    else
        die("The image backend '{}' isn't supported, only kitty and viu.\n"
            "Please currently use the GUI mode for rendering the image/gif (use -h for more details)",
            config.m_image_backend);

    for (size_t i = 0; i < layout.size(); i++)
        // took math from neofetch in get_term_size() and get_image_size(). seems to work nice
        for (size_t _ = 0; _ < width + config.offset; _++)
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

std::vector<std::string> Display::render(const Config& config, const colors_t& colors, const bool already_analyzed_file,
                                         const std::string_view path)
{
    systemInfo_t             systemInfo{};
    std::vector<std::string> asciiArt{}, layout{ config.m_args_layout.empty() ? config.layout : config.m_args_layout };

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

    if (!config.m_custom_distro.empty() && !config.m_display_distro)
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

        while (std::getline(distro_file, line))
            parse(line, systemInfo, _, config, colors, false);
    }

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

    std::string line;
    while (std::getline(file, line))
    {
        std::string pureOutput;
        std::string asciiArt_s = parse(line, systemInfo, pureOutput, config, colors, false);
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
    for (const std::string_view str : renderResult)
        fmt::println("{}", str);
}
