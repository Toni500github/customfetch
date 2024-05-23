#ifndef DISPLAY_HPP
#define DISPLAY_HPP
#include <query.hpp>
#include <string_view>

using std::string_view;

struct SysInfo {
    string systemName;
    string GPUName;
};

namespace DisplaySystem {
    string Render(SysInfo& sysInfo);
    void Display(string_view renderResult);
}

#endif
