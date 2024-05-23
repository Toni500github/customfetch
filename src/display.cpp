/* Implementation of the system behind displaying/rendering the information */
#include <display.hpp>
#include <fmt/core.h>

using namespace DisplaySystem;

string DisplaySystem::Render(SysInfo& sysInfo) {
    return fmt::format("System: {}\nGPU: {}\nOS Pretty name = {}\n", sysInfo.systemName, sysInfo.GPUName, query_sys.OS_Name());
}

void DisplaySystem::Display(string_view renderResult) {
    fmt::print("{}", renderResult);
}
