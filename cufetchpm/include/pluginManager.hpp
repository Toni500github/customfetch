#include <array>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

#include "stateManager.hpp"
#include "toml++/toml.hpp"
#include "util.hpp"

constexpr std::array<std::string_view, 1> dependencies = {"git"};

class PluginManager
{
public:
    PluginManager(const StateManager& state) : m_state(state){}
    PluginManager(StateManager&& state)      : m_state(std::move(state)){}

    void create_cache_dir();
    void add_repo_plugins(const std::string& repo);
    bool add_plugin(const std::string&);
    bool has_deps();

private:
    const StateManager& m_state;
    const std::filesystem::path m_cache_path{getHomeCacheDir()/"cufetchpm"/"plugins"};
    toml::table m_manifest;
};
