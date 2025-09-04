/*
 * Copyright 2025 Toni500git
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 * following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote
 * products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _PLUGIN_MANAGER_HPP_
#define _PLUGIN_MANAGER_HPP_

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

#include "stateManager.hpp"
#include "util.hpp"

namespace fs = std::filesystem;

inline struct operations_t
{
    bool                     install_force   = false;
    bool                     install_no_warn = false;
    bool                     list_verbose    = false;
    std::vector<std::string> arguments;
} options;

template <typename... Args>
void success(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::print("\033[1;32m==> {}\033[0m\n", fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

template <typename... Args>
void status(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::print("\033[1;34m==> {} ...\033[0m\n", fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

class PluginManager
{
public:
    PluginManager(const StateManager& state_manager) : m_state_manager(state_manager) {}
    PluginManager(StateManager&& state_manager) : m_state_manager(std::move(state_manager)) {}

    void add_plugins_repo(const std::string& repo);
    void build_plugins(const fs::path& working_dir);
    bool add_plugin(const std::string&);
    void update_repos();
    bool is_plugin_conflicting(const plugin_t& plugin);
    void remove_repo(const std::string& repo_name);

private:
    StateManager m_state_manager;
    fs::path     m_config_path{ getConfigDir() / "plugins" };
    fs::path     m_cache_path{ getHomeCacheDir() / "cufetchpm" };
};

#endif
