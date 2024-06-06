/* Implementation of the system behind displaying/rendering the information */
#include "display.hpp"
#include "config.hpp"
#include "util.hpp"
#include <fstream>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <memory>

std::string ascii_art_path = "/tmp/test.txt";

// function taken from archlinux pacman utils
static size_t string_length(const char *s) {
    int      len;
    wchar_t *wcstr;

    if (!s || s[0] == '\0') {
        return 0;
    }
    if (strstr(s, "\033")) {
        char *replaced = (char *)malloc(sizeof(char) * strlen(s));
        int   iter     = 0;
        for (; *s; s++) {
            if (*s == '\033') {
                while (*s != 'm') {
                    s++;
                }
            } else {
                replaced[iter] = *s;
                iter++;
            }
        }
        replaced[iter] = '\0';
        len            = iter;
        wcstr          = (wchar_t *)calloc(len, sizeof(wchar_t));
        len            = mbstowcs(wcstr, replaced, len);
        len            = wcswidth(wcstr, len);
        free(wcstr);
        free(replaced);
    } else {
        /* len goes from # bytes -> # chars -> # cols */
        len   = strlen(s) + 1;
        wcstr = (wchar_t *)calloc(len, sizeof(wchar_t));
        len   = mbstowcs(wcstr, s, len);
        len   = wcswidth(wcstr, len);
        free(wcstr);
    }

    return len;
}

std::vector<std::string>& Display::render(systemInfo_t& systemInfo) {
    for (std::string& layout : config.layouts) {
        std::unique_ptr<std::string> _;
        layout = parse(layout, systemInfo, _);
    }

    std::ifstream file(ascii_art_path, std::ios_base::binary);
    if (!file.is_open())
        die("Could not open {}", ascii_art_path);
    
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
            config.layouts[i].insert(0, asciiArt[i]);
            origin = asciiArt[i].length();
        }

        size_t spaces = (maxLineLength + 5) - (i < asciiArt.size() ? pureAsciiArt[i]->length() : 0);
        for (size_t j = 0; j < spaces; j++)
            config.layouts[i].insert(origin, " ");
        
        config.layouts[i] += NOCOLOR;
    }

    if (i < asciiArt.size())
        config.layouts.insert(config.layouts.end(), asciiArt.begin() + i, asciiArt.end());

    return config.layouts;
}

void Display::display(std::vector<std::string>& renderResult, systemInfo_t& systemInfo) {
    fmt::println("{}", fmt::join(renderResult, "\n"));
}
