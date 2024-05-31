/* Implementation of the system behind displaying/rendering the information */
#include "display.hpp"
#include "config.hpp"
#include "util.hpp"
#include <fstream>
#include <fmt/core.h>
#include <fmt/ranges.h>

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
    
    for (std::string& layout : config.layouts)
        parse(layout, systemInfo);

    return config.layouts;
}

void Display::display(std::vector<std::string>& renderResult, systemInfo_t& systemInfo) {
    std::ifstream file(ascii_art_path, std::ios_base::binary);
    if (!file.is_open()) {
        error("Could not open {}", ascii_art_path);
        return;
    }
    
    std::string line;
    std::vector<std::string> ascii_art;
    
    while (std::getline(file, line))
        ascii_art.push_back(line);
    
    size_t art_width = 0, art_width_full = 0;
    for (auto& str : ascii_art) {
        parse(str, systemInfo);
        if (string_length(str.c_str()) > art_width)
            art_width = string_length(str.c_str());
        if (str.size() > art_width_full)
            art_width_full = str.size();
    }
    
    size_t max_lines = std::max(ascii_art.size(), renderResult.size());
    for (size_t i = 0; i < max_lines; ++i) {
        
        if (i < ascii_art.size())
            fmt::print("{:<{}}\t", ascii_art[i], art_width_full);

        if (i < renderResult.size()) {
            if (i >= ascii_art.size())
                fmt::print("{:<{}}\t{}", "", art_width, renderResult[i]);
            else
                fmt::print("{}", renderResult[i]);
        }

        fmt::println("");
    }    

}
