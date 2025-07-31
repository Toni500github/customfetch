#ifndef _PLUGIN_MANAGER_HPP_
#define _PLUGIN_MANAGER_HPP_

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

#include "stateManager.hpp"
#include "util.hpp"

namespace fs = std::filesystem;

inline const std::vector<std::string> core_dependencies = { "git" };  // expand in the future, maybe

inline struct operations_t
{
    bool                     install_force = false;
    bool                     list_verbose  = false;
    std::vector<std::string> arguments;
} options;

#define BOLD_COLOR(x) (fmt::emphasis::bold | fmt::fg(fmt::rgb(x)))

template <typename... Args>
void success(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::print(BOLD_COLOR((fmt::color::green)), "SUCCESS:\033[0m {}\n",
               fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

template <typename... Args>
void status(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::print(BOLD_COLOR((fmt::color::cadet_blue)), "status:\033[0m {} ...\n",
               fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

#undef BOLD_COLOR

class PluginManager
{
public:
    PluginManager(const StateManager& state_manager) : m_state_manager(state_manager) {}
    PluginManager(StateManager&& state_manager) : m_state_manager(std::move(state_manager)) {}

    void add_repo_plugins(const std::string& repo);
    void build_plugins(const fs::path& working_dir);
    bool add_plugin(const std::string&);
    bool has_deps(const std::vector<std::string>& dependencies);

private:
    StateManager m_state_manager;
    fs::path     m_config_path{ getConfigDir() / "plugins" };
    fs::path     m_cache_path{ getHomeCacheDir() / "cufetchpm" / "plugins" };
};

#endif
