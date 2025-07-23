#include "manifest.hpp"

#include <algorithm>
#include <cctype>
#include <vector>

#include "libcufetch/common.hh"
#include "util.hpp"

static bool validManifestName(const std::string_view n)
{
    return std::ranges::all_of(n,
                               [](const unsigned char c) { return (isalnum(c) || c == '-' || c == '_' || c == '='); });
}

CManifest::CManifest(const std::string_view path) : m_is_state(false)
{
    try
    {
        this->m_tbl = toml::parse_file(path);
    }
    catch (const toml::parse_error& err)
    {
        die(_("Failed to parse state file at '{}':\n"
              "{}\n"
              "\t(error occurred at line {} column {})"),
            path, err.description(), err.source().begin.line, err.source().begin.column);
    }
}

std::vector<plugin_t> CManifest::get_all_plugins()
{
    std::vector<plugin_t> plugins;
    for (auto const& [name, _] : m_tbl)
    {
        if (name.str() == "repository")
            continue;

        if (!validManifestName(name.str()))
        {
            warn("Plugin '{}' has an invalid name. Only alphanumeric and '-', '_', '=' are allowed in the name",
                 name.str());
            continue;
        }

        plugins.push_back({ .name        = name.data(),
                            .description = getValue<std::string>(name, "description"),
                            .output_dir  = getValue<std::string>(name, "license"),
                            .licenses    = getValueArrayStr(name, "licenses"),
                            .conflicts   = getValueArrayStr(name, "conflicts"),
                            .authors     = getValueArrayStr(name, "authors"),
                            .build_steps = getValueArrayStr(name, "build-steps"),
                            .prefixes    = getValueArrayStr(name, "prefixes") });
    }
    return plugins;
}

plugin_t CManifest::get_plugin(const std::string_view name)
{
    if (!m_tbl[name].is_table())
        die("Couldn't find such plugin '{}' in manifest", name);

    return { .name        = name.data(),
             .description = getValue<std::string>(name, "description"),
             .output_dir  = getValue<std::string>(name, "license"),
             .licenses    = getValueArrayStr(name, "licenses"),
             .conflicts   = getValueArrayStr(name, "conflicts"),
             .authors     = getValueArrayStr(name, "authors"),
             .build_steps = getValueArrayStr(name, "build-steps"),
             .prefixes    = getValueArrayStr(name, "prefixes") };
}
