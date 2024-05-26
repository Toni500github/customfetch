/* Implementation of the system behind displaying/rendering the information */
#include "fmt/color.h"
#include "query.hpp"
#include "display.hpp"
#include <iostream>
#include <fmt/core.h>
#include <fmt/ranges.h>

std::string Display::render() {
    std::string vendor_id = query_gpu.vendor_id();
    return fmt::format("System: {}\n"
                       "Uptime: {}\n"
                       "GPU: {}\n"
                       "GPU vendor: {}\n"
                       "CPU model name: {}\n"
                       "RAM usage: {}Mib / {}Mib\n"
                       "RAM free amount: {}Mib\n"
                       "Kernel version: {}\n"
                       "OS Pretty name: {}\n"
                       "Arch: {}\n"
                       "Hostname: {}\n",
                       query_system.kernel_name(),
                       query_system.uptime(),
                       query_gpu.name(vendor_id),
                       query_gpu.vendor(vendor_id),
                       query_cpu.name(),
                       query_ram.used_amount(), query_ram.total_amount(),
                       query_ram.free_amount(),
                       query_system.kernel_version(),
                       query_system.OS_pretty_name(),
                       query_system.arch(),
                       query_system.hostname()
                       );
}

void Display::display(std::string renderResult) {
    std::string path = "/tmp/test.txt";
    std::ifstream file(path);
    if (!file.is_open())
        error("Could not open {}", path);
    
    std::string line;
    std::vector<std::string> sys_info = split(renderResult, '\n');

    for (int i = 0; std::getline(file, line); i++) {
        fmt::print("{}", line);
        if (i < sys_info.size())
            fmt::println("{}", sys_info[i]);
        else
            fmt::print("\n");
    }
}
