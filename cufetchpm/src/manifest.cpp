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

    for (const auto& [name, _] : m_tbl)
    {
        if (name.str() == "repository")
            continue;

        if (!is_valid_name(name.str()))
        {
            warn("Plugin '{}' has an invalid name. Only alphanumeric and '-', '_', '=' are allowed in the name",
                 name.str());
            continue;
        }

        m_repo.plugins.push_back({ .name        = name.data(),
                                   .description = getStrValue(name, "description"),
                                   .output_dir  = getStrValue(name, "output-dir"),
                                   .licenses    = getStrArrayValue(name, "licenses"),
                                   .conflicts   = getStrArrayValue(name, "conflicts"),
                                   .authors     = getStrArrayValue(name, "authors"),
                                   .build_steps = getStrArrayValue(name, "build-steps"),
                                   .prefixes    = getStrArrayValue(name, "prefixes") });
    }
}

plugin_t CManifest::get_plugin(const std::string_view name)
{
    if (!m_tbl[name].is_table())
        die("Couldn't find such plugin '{}' in manifest", name);

    return { .name        = name.data(),
             .description = getStrValue(name, "description"),
             .output_dir  = getStrValue(name, "output-dir"),
             .licenses    = getStrArrayValue(name, "licenses"),
             .conflicts   = getStrArrayValue(name, "conflicts"),
             .authors     = getStrArrayValue(name, "authors"),
             .build_steps = getStrArrayValue(name, "build-steps"),
             .prefixes    = getStrArrayValue(name, "prefixes") };
}
