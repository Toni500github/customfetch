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

#ifndef _MANIFEST_HPP_
#define _MANIFEST_HPP_

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "platform.hpp"
#include "toml++/toml.hpp"

#if CF_LINUX
constexpr char const PLATFORM[] = "linux";
#elif CF_MACOS
constexpr char const PLATFORM[] = "macos";
#elif CF_ANDROID
constexpr char const PLATFORM[] = "android";
#endif

struct plugin_t
{
    // The plugin name.
    // It must be conform to the function is_valid_name()
    std::string name;

    // The plugin description.
    std::string description;

    // The plugin build directory,
    // where we'll retrive the built plugin libraries.
    std::string output_dir;

    // The plugin multiple SPDX License Identifier (MIT, GPL-2.0, ...)
    // NOTE: it doesn't actually check if they are correct or not.
    std::vector<std::string> licenses;

    // The plugin authors.
    std::vector<std::string> authors;

    // A list of commands to be executed for building the plugin.
    // Kinda like a Makefile target instructions.
    // Each command will be executed from a different shell session.
    std::vector<std::string> build_steps;

    // A list of registered root modules that the plugin will be used for querying its submodules.
    // For example: 'github.followers' the root module is indeed 'github' and 'followers' is the submodule.
    std::vector<std::string> prefixes;

    // Platforms that are supported by the plugin.
    // Make it a string and put 'all' for being cross-platform.
    std::vector<std::string> platforms;
};

struct manifest_t
{
    // The repository name.
    // It must be conform to the function is_valid_name()
    std::string name;

    // The repository git url
    std::string url;

    // NOTE: INTERNAL ONLY
    // The repository latest commit hash.
    std::string git_hash;

    // An array of all the plugins that are declared in the manifest
    std::vector<plugin_t> plugins;

    // An array for storing the dependencies for 'all' and current platforms.
    // first -> platform string name
    // seconds -> platform dependencies vector names
    std::vector<std::string> dependencies;
};

constexpr char const MANIFEST_NAME[] = "cufetchpm.toml";

namespace ManifestSpace
{
std::string              getStrValue(const toml::table& tbl, const std::string_view name, const std::string_view key);
std::string              getStrValue(const toml::table& tbl, const std::string_view path);
std::vector<std::string> getStrArrayValue(const toml::table& tbl, const std::string_view name,
                                          const std::string_view value);
std::vector<std::string> getStrArrayValue(const toml::table& tbl, const std::string_view path);
}  // namespace ManifestSpace

class CManifest
{
public:
    CManifest(const std::filesystem::path& path);

    plugin_t get_plugin(const std::string_view name);

    const std::string& get_repo_name() const
    { return m_repo.name; }

    const std::string& get_repo_url() const
    { return m_repo.url; }

    const std::string& get_repo_hash() const
    { return m_repo.git_hash; }

    const std::vector<plugin_t>& get_all_plugins() const
    { return m_repo.plugins; }

    const std::vector<std::string>& get_dependencies() const
    { return m_repo.dependencies; }

private:
    toml::table m_tbl;
    manifest_t  m_repo;

    void parse_manifest(const std::filesystem::path& path);

    std::string getStrValue(const std::string_view name, const std::string_view key) const
    {
        return ManifestSpace::getStrValue(m_tbl, name, key);
    }

    std::string getStrValue(const std::string_view path) const
    {
        return ManifestSpace::getStrValue(m_tbl, path);
    }

    std::vector<std::string> getStrArrayValue(const std::string_view name, const std::string_view value) const
    {
        return ManifestSpace::getStrArrayValue(m_tbl, name, value);
    }
};

#endif  // !_MANIFEST_HPP_;
