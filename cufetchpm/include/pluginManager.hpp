#ifndef _PLUGIN_MANAGER_HPP_
#define _PLUGIN_MANAGER_HPP_

#include <array>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

#include "stateManager.hpp"
#include "toml++/toml.hpp"
#include "util.hpp"

constexpr std::array<std::string_view, 1> dependencies = {"git"}; // expand in the future, maybe

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
    PluginManager(const StateManager& state) : m_state(state){}
    PluginManager(StateManager&& state)      : m_state(std::move(state)){}

    void add_repo_plugins(const std::string& repo);
    bool add_plugin(const std::string&);
    bool has_deps();

private:
    const StateManager& m_state;
    const std::filesystem::path m_cache_path{getHomeCacheDir()/"cufetchpm"/"plugins"};
    toml::table m_manifest;
};


#endif
