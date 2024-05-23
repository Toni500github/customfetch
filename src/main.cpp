#include <query.hpp>
#include <display.hpp>

int main (int argc, char *argv[]) {
    SysInfo sysInfo{};
    sysInfo.systemName = query_system.name();
    sysInfo.GPUName = query_system.GPUName();

    Display::display(Display::render(sysInfo));
    return 0;
}
