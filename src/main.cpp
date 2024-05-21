#include <query.hpp>
#include <display.hpp>

int main (int argc, char *argv[]) {
    SystemInformation sysInfo{};
    sysInfo.systemName = QuerySystem::QuerySystemName();
    sysInfo.GPUName = QuerySystem::QueryGPUName();

    DisplaySystem::Display(DisplaySystem::Render(sysInfo));
    return 0;
}
