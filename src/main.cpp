#include <query.hpp>
#include <display.hpp>

int main (int argc, char *argv[]) {
    SysInfo sysInfo{};
    sysInfo.systemName = query_sys.SystemName();
    sysInfo.GPUName = query_sys.GPUName();

    DisplaySystem::Display(DisplaySystem::Render(sysInfo));
    return 0;
}
