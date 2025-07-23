#ifndef _STATE_MANAGER_HPP_
#define _STATE_MANAGER_HPP_

#include <filesystem>

#include "manifest.hpp"
#include "toml++/toml.hpp"
#include "util.hpp"

class StateManager
{
public:
    StateManager();
    StateManager(StateManager&&)      = default;
    StateManager(const StateManager&) = default;
    ~StateManager()                   = default;

    void        add_new_plugin(const plugin_t& manifest);
    toml::table get_state() { return m_state; }

private:
    const std::filesystem::path m_path{ getHomeCacheDir() / "cufetchpm" / "state.toml" };
    toml::table                 m_state;
};

#endif
