/* Implementation of the system behind displaying/rendering the information */
#include "display.hpp"
#include "config.hpp"
#include "util.hpp"
#include <fstream>
#include <fmt/core.h>
#include <fmt/ranges.h>

std::string ascii_art_path = "/tmp/test.txt";

std::vector<std::string> Display::render(systemInfo_t& systemInfo) {
    
    for (std::string &layout : config.layouts)
        parse(layout, systemInfo);

    /*return fmt::format("{}@{}\n\n"
                       "{}\n"
                       "System: {}\n"
                       "Uptime: {} minutes\n"
                       "GPU: {}\n"
                       "GPU vendor: {}\n"
                       "CPU model name: {}\n"
                       "RAM usage: {}MB / {}MB\n"
                       "RAM free amount: {}MB\n"
                       "Kernel version: {}\n"
                       "OS Pretty name: {}\n"
                       "Arch: {}\n",
                       query_system.username(), query_system.hostname(),
                       fmt::join(config.layouts, "\n"),
                       query_system.kernel_name(),
                       uptime_mins.count(),
                       std::get<std::string>(systemInfo["gpu"]["name"]),
                       std::get<std::string>(systemInfo["gpu"]["vendor"]),
                       std::get<std::string>(systemInfo["cpu"]["name"]),
                       std::get<size_t>(systemInfo["ram"]["used"]), 
                       std::get<size_t>(systemInfo["ram"]["total"]), 
                       std::get<size_t>(systemInfo["ram"]["free"]), 
                       query_system.kernel_version(),
                       query_system.OS_pretty_name(),
                       query_system.arch()
                       );*/
    return config.layouts;
}

void Display::display(std::vector<std::string> renderResult, systemInfo_t& systemInfo) {
    std::ifstream file(ascii_art_path);
    if (!file.is_open()) {
        error("Could not open {}", ascii_art_path);
        return;
    }
    
    std::string line;
    std::vector<std::string> sys_info = renderResult;
    std::vector<std::string> ascii_art;
    
    while (std::getline(file, line))
        ascii_art.push_back(line);

    size_t art_width = 0;
    for (auto& line : ascii_art) {
        //parse(line, systemInfo_t());
        if (line.size() > art_width)
            art_width = line.size();
    }

    size_t max_lines = std::max(ascii_art.size(), sys_info.size());
    for (size_t i = 0; i < max_lines; ++i) {
        
        if (i < art_width)
            fmt::print("{:<{}}\t", ascii_art[i], art_width);

        if (i < sys_info.size())
            fmt::println("{}",sys_info[i]);

        //fmt::print("\n");
    }    

}
