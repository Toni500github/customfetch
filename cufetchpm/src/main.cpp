#include "pluginManager.hpp"
#include "stateManager.hpp"

int main(int argc, char* argv[])
{
    std::filesystem::create_directories({ getHomeCacheDir() / "cufetchpm" / "plugins" });
    StateManager  state;
    PluginManager man(std::move(state));
    man.add_repo_plugins(argv[2]);
    return 0;
}
