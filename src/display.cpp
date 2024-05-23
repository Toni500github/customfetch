/* Implementation of the system behind displaying/rendering the information */
#include <display.hpp>
#include <fmt/core.h>

using namespace Display;

string Display::render(SysInfo& sysInfo) {
    return fmt::format("System: {}\nGPU: {}\nOS Pretty name = {}\n", sysInfo.systemName, sysInfo.GPUName, query_system.OS_Name());
}

void Display::display(string_view renderResult) {
    fmt::print("{}", renderResult);
}
