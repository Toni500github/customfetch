/* Implementation of the system behind displaying/rendering the information */
#include "query.hpp"
#include "display.hpp"
#include <fmt/core.h>

std::string Display::render() {
    return fmt::format("System: {}\n"
                       "GPU: {}\n"
                       "GPU vendor: {}\n"
                       "Kernel version: {}\n"
                       "OS Pretty name: {}\n"
                       "Arch: {}\n"
                       "Hostname: {}\n",
                       query_system.kernel_name(), 
                       query_gpu.name(),
                       query_gpu.vendor(),
                       query_system.kernel_version(),
                       query_system.OS_pretty_name(),
                       query_system.arch(),
                       query_system.hostname()
                       );
}

void Display::display(std::string renderResult) {
    fmt::print("{}", renderResult);
}
