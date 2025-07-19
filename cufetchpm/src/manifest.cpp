#include "manifest.hpp"

#include "libcufetch/common.hh"
#include "util.hpp"

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
            path, err.description(),
            err.source().begin.line, err.source().begin.column);
    }
}

manifest_t CManifest::get_plugin(const std::string_view name)
{
    if (!m_tbl[name].is_table())
        die("Couldn't find such plugin '{}' in manifest", name);

    return {
        name.data(),
        getValue<std::string>(name, "license"),
        getValue<std::string>(name, "description"),
        getValue<std::string>(name, "output-dir"),
        getValueArrayStr(name, "authors"),
        getValueArrayStr(name, "build"),
    };
}
