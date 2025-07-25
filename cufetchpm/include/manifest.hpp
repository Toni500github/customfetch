#ifndef _MANIFEST_HPP_
#define _MANIFEST_HPP_

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "libcufetch/common.hh"
#include "toml++/toml.hpp"

struct plugin_t
{
    // The plugin name.
    // It must be conform to the function is_valid_name()
    std::string name;

    // The plugin description.
    std::string description;

    // The plugin build directory,
    // where we'll retrive the built plugin
    std::string output_dir;

    // The plugin multiple SPDX License Identifier (MIT, GPL-2.0, ...)
    // NOTE: it doesn't actually check if they are correct or not.
    std::vector<std::string> licenses;

    // Which plugins can be conflicting by name / modules, using the git url.
    std::vector<std::string> conflicts;

    // The plugin authors.
    std::vector<std::string> authors;

    // A list of commands to be executed for building the plugin.
    // Kinda like a Makefile target instructions.
    // Each command will be executed from a different shell session.
    std::vector<std::string> build_steps;

    // A list of registered root modules that the plugin will be used for querying its submodules.
    // For example: 'github.followers' the root module is indeed 'github' and 'followers' is the submodule.
    std::vector<std::string> prefixes;
};

struct manifest_t
{
    // The repository name.
    // It must be conform to the function is_valid_name()
    std::string name;

    // The repository git/homepage url
    std::string url;

    // An array of all the plugins that are declared in the manifest
    std::vector<plugin_t> plugins;
};

constexpr char const MANIFEST_NAME[] = "cufetchpm.toml";

class CManifest
{
public:
    CManifest(const std::string_view path);
    CManifest(toml::table&& tbl) : m_tbl(std::move(tbl)) { parse_manifest(); }
    CManifest(const toml::table& tbl) : m_tbl(tbl) { parse_manifest(); }

    plugin_t get_plugin(const std::string_view name);

    const std::string& get_repo_name() const
    { return m_repo.name; }

    const std::string& get_repo_url() const
    { return m_repo.url; }

    const std::vector<plugin_t>& get_all_plugins() const
    { return m_repo.plugins; }

private:
    toml::table  m_tbl;
    manifest_t   m_repo;

    void parse_manifest();

    std::string getStrValue(const std::string_view name, const std::string_view key) const
    {
        const std::optional<std::string>& ret = m_tbl[name][key].value<std::string>();
        return ret.value_or(UNKNOWN);
    }

    std::string getStrValue(const std::string_view path) const
    {
        const std::optional<std::string>& ret = m_tbl.at_path(path).value<std::string>();
        return ret.value_or(UNKNOWN);
    }

    std::vector<std::string> getStrArrayValue(const std::string_view name, const std::string_view value) const
    {
        std::vector<std::string> ret;

        // https://stackoverflow.com/a/78266628
        if (const toml::array* array_it = m_tbl[name][value].as_array())
        {
            array_it->for_each([&ret](auto&& el) {
                if (const toml::value<std::string>* str_elem = el.as_string())
                    ret.push_back((*str_elem)->data());
            });

            return ret;
        }
        return {};
    }
};

#endif  // !_MANIFEST_HPP_;
