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

#include "manifest.hpp"

#include <algorithm>
#include <cctype>
#include <vector>

#include "libcufetch/common.hh"
#include "util.hpp"

static bool is_valid_name(const std::string_view n)
{
    return std::ranges::all_of(n,
                               [](const unsigned char c) { return (isalnum(c) || c == '-' || c == '_' || c == '='); });
}

std::string ManifestSpace::getStrValue(const toml::table& tbl, const std::string_view name, const std::string_view key)
{
    const std::optional<std::string>& ret = tbl[name][key].value<std::string>();
    return ret.value_or(UNKNOWN);
}

std::string ManifestSpace::getStrValue(const toml::table& tbl, const std::string_view path)
{
    const std::optional<std::string>& ret = tbl.at_path(path).value<std::string>();
    return ret.value_or(UNKNOWN);
}

std::vector<std::string> ManifestSpace::getStrArrayValue(const toml::table& tbl, const std::string_view path)
{
    std::vector<std::string> ret;

    // https://stackoverflow.com/a/78266628
    if (const toml::array* array_it = tbl.at_path(path).as_array())
    {
        array_it->for_each([&ret](auto&& el) {
            if (const toml::value<std::string>* str_elem = el.as_string())
                ret.push_back((*str_elem)->data());
        });

        return ret;
    }
    return {};
}

std::vector<std::string> ManifestSpace::getStrArrayValue(const toml::table& tbl, const std::string_view name,
                                                         const std::string_view value)
{
    std::vector<std::string> ret;

    // https://stackoverflow.com/a/78266628
    if (const toml::array* array_it = tbl[name][value].as_array())
    {
        array_it->for_each([&ret](auto&& el) {
            if (const toml::value<std::string>* str_elem = el.as_string())
                ret.push_back((*str_elem)->data());
        });

        return ret;
    }
    return {};
}

CManifest::CManifest(const std::string_view path)
{
    try
    {
        this->m_tbl = toml::parse_file(path);
    }
    catch (const toml::parse_error& err)
    {
        die(_("Failed to parse manifest file at '{}':\n"
              "{}\n"
              "\t(error occurred at line {} column {})"),
            path, err.description(), err.source().begin.line, err.source().begin.column);
    }

    parse_manifest();
}

void CManifest::parse_manifest()
{
    m_repo.name = getStrValue("repository", "name");
    m_repo.url  = getStrValue("repository", "url");
    if (m_repo.name == UNKNOWN)
        die("Couldn't find manifest repository name");
    if (!is_valid_name(m_repo.name))
        die("Manifest repository name '{}' is invalid. Only alphanumeric and '-', '_', '=' are allowed in the name",
            m_repo.name);

    if (auto* deps = m_tbl["dependencies"].as_table())
    {
        // Collect "all" dependencies
        if (auto arr = (*deps)["all"].as_array())
        {
            for (auto&& pkg : *arr)
                if (auto s = pkg.value<std::string>())
                    m_repo.dependencies.push_back(*s);
        }

        // Collect platform-specific dependencies
        if (auto arr = (*deps)[PLATFORM].as_array())
        {
            for (auto&& pkg : *arr)
                if (auto s = pkg.value<std::string>())
                    m_repo.dependencies.push_back(*s);
        }
    }

    for (const auto& [name, _] : m_tbl)
    {
        if (name.str() == "repository" || name.str() == "dependencies")
            continue;

        if (!is_valid_name(name.str()))
        {
            warn("Plugin '{}' has an invalid name. Only alphanumeric and '-', '_', '=' are allowed in the name",
                 name.str());
            continue;
        }

        m_repo.plugins.push_back(get_plugin(name));
    }
}

plugin_t CManifest::get_plugin(const std::string_view name)
{
    // if (!m_tbl[name].is_table())
    //   die("Couldn't find such plugin '{}' in manifest", name);

    return { .name        = name.data(),
             .description = getStrValue(name, "description"),
             .output_dir  = getStrValue(name, "output-dir"),
             .licenses    = getStrArrayValue(name, "licenses"),
             .conflicts   = getStrArrayValue(name, "conflicts"),
             .authors     = getStrArrayValue(name, "authors"),
             .build_steps = getStrArrayValue(name, "build-steps"),
             .prefixes    = getStrArrayValue(name, "prefixes"),
             .platforms   = getStrArrayValue(name, "platforms") };
}
