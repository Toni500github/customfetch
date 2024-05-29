/* Implementation of the system behind displaying/rendering the information */
#include "query.hpp"
#include "display.hpp"
#include "config.hpp"
#include "util.hpp"
#include <chrono>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <unordered_map>

std::string Display::render() {
    std::string vendor_id = query_gpu.vendor_id();
    
    std::chrono::seconds uptime_secs(query_system.uptime());
    auto uptime_mins = std::chrono::duration_cast<std::chrono::minutes>(uptime_secs);

    systemInfo_t systemInfo = {
        {"cpu", {
            {"name", std::variant<std::string, size_t>(query_cpu.name())},
            },
        }, 
        {"gpu", {
            {"name", std::variant<std::string, size_t>(query_gpu.name(vendor_id))},
            {"vendor", std::variant<std::string, size_t>(query_gpu.vendor(vendor_id))}
            },
        },
        {"ram", {
            {"used", std::variant<std::string, size_t>(query_ram.used_amount())},
            {"total", std::variant<std::string, size_t>(query_ram.total_amount())},
            {"free", std::variant<std::string, size_t>(query_ram.free_amount())}
            }
        }};
    
    for (std::string &layout : config.layouts)
        parse(layout, systemInfo);

    return fmt::format("{}@{}\n\n"
                       "{}\n"
                       "System: {}\n"
                       "Uptime: {} minutes\n"
                       "GPU: {}\n"
                       "GPU vendor: {}\n"
                       "CPU model name: {}\n"
                       "RAM usage: {}MiB / {}MiB\n"
                       "RAM free amount: {}MiB\n"
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
                       );
}

void Display::display(std::string renderResult) {
    std::string path = "/tmp/test.txt";
    std::ifstream file(path);
    if (!file.is_open()) {
        error("Could not open {}", path);
        return;
    }
    
    std::string line;
    std::vector<std::string> sys_info = split(renderResult, '\n');
    std::vector<std::string> ascii_art;
    
    while (std::getline(file, line))
        ascii_art.push_back(line);

    size_t art_width = 0;
    for (const auto& line : ascii_art)
        if (line.size() > art_width)
            art_width = line.size();

    size_t max_lines = std::max(ascii_art.size(), sys_info.size());
    for (size_t i = 0; i < max_lines; ++i) {
        
        if (i < art_width)
            fmt::print("{:<{}}\t", ascii_art[i], art_width);

        if (i < sys_info.size())
            fmt::print("{}",sys_info[i]);

        fmt::print("\n");
    }    

}
