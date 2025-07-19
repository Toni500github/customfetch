#include <filesystem>

#include "toml++/toml.hpp"
#include "util.hpp"

class StateManager
{
public:
    StateManager();
    StateManager(StateManager&&)      = default;
    StateManager(const StateManager&) = default;
    ~StateManager()                   = default;

    toml::table get_state() { return m_state; }

private:
    const std::filesystem::path m_path{getConfigDir()/"plugins"/"state.toml"};
    toml::table                 m_state;
};
