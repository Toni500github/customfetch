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
    std::string name;

    // The plugin description.
    std::string description;

    // The plugin build directory,
    // where we'll retrive the built plugin
    std::string output_dir;

    // The plugin multiple SPDX License Identifier (MIT, GPL-2.0, ...)
    // NOTE: it doesn't actually check if they are correct or not.
    std::vector<std::string> licenses;

    // Which plugins can be conflicting by name / modules.
    // TODO: choose if check either name or git url.
    std::vector<std::string> conflicts;

    // The plugin authors.
    std::vector<std::string> authors;

    // A list of commands to be executed for building the plugin.
    // Kinda like a Makefile target instructions.
    // Each command will be executed from a different shell session.
    std::vector<std::string> build_steps;

    // A list of root modules that the plugin will be used for querying its modules
    std::vector<std::string> prefixes;
};

const char* const MANIFEST_NAME = "cufetchpm.toml";

class CManifest
{
public:
    CManifest(const std::string_view path);
    CManifest(toml::table&& tbl) : m_tbl(std::move(tbl)) {}
    CManifest(const toml::table& tbl) : m_tbl(tbl) {}

    plugin_t              get_plugin(const std::string_view name);
    std::vector<plugin_t> get_all_plugins();

private:
    toml::table m_tbl;
    bool        m_is_state = true;

    template <typename T>
    T getValue(const std::string_view name, const std::string_view value) const
    {
        const std::optional<T>& ret = m_tbl[name][value].value<T>();
        return ret.value_or(UNKNOWN);
    }

    std::vector<std::string> getValueArrayStr(const std::string_view name, const std::string_view value) const
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
