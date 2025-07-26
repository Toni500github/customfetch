#ifndef _STATE_MANAGER_HPP_
#define _STATE_MANAGER_HPP_

#include <filesystem>

#include "manifest.hpp"
#include "toml++/toml.hpp"
#include "util.hpp"

namespace fs = std::filesystem;

class StateManager
{
public:
    StateManager();
    StateManager(StateManager&&)      = default;
    StateManager(const StateManager&) = default;
    ~StateManager()                   = default;

    void add_new_repo(const CManifest& manifest);
    void add_new_plugin(const plugin_t& manifest);
    std::vector<manifest_t> get_all_repos();

    const toml::table& get_state() const
    { return m_state; }

private:
    const fs::path m_path{ getConfigDir() / "plugins" / "state.toml" };
    toml::table    m_state;
};

#endif
