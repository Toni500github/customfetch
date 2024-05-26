/* Implementation of the system behind displaying/rendering the information */
#include "query.hpp"
#include "display.hpp"
#include <fmt/core.h>

std::string Display::render() {
    std::string vendor_id = query_gpu.vendor_id();
    return fmt::format("System: {}\n"
                       "GPU: {}\n"
                       "GPU vendor: {}\n"
                       "Kernel version: {}\n"
                       "OS Pretty name: {}\n"
                       "Arch: {}\n"
                       "Hostname: {}\n",
                       query_system.kernel_name(), 
                       query_gpu.name(vendor_id),
                       query_gpu.vendor(vendor_id),
                       query_system.kernel_version(),
                       query_system.OS_pretty_name(),
                       query_system.arch(),
                       query_system.hostname()
                       );
}

void Display::display(std::string renderResult) {
    fmt::print("{}", renderResult);
}
