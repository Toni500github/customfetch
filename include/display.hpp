#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include <query.hpp>
#include <string_view>

using std::string_view;

struct SysInfo {
    string systemName;
    string GPUName;
};

namespace Display {
    string render(SysInfo& sysInfo);
    void display(string_view renderResult);
}

#endif
