/* Implementation of the system behind displaying/rendering the information */
#include <display.hpp>
#include <fmt/core.h>

using namespace DisplaySystem;

string DisplaySystem::Render(SystemInformation &systemInformation) {
    return fmt::format("System: {}\nGPU: {}\n", systemInformation.systemName, systemInformation.GPUName);
}

void DisplaySystem::Display(string_view renderResult) {
    fmt::print("{}", renderResult);
}