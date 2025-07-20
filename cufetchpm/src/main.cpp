#include "stateManager.hpp"
#include "pluginManager.hpp"

int main (int argc, char *argv[])
{
    std::filesystem::create_directories({getHomeCacheDir()/"cufetchpm"/"plugins"});
    StateManager state;
    PluginManager man(state);
    man.add_repo_plugins(argv[2]);
    return 0;
}
