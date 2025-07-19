#include "pluginManager.hpp"
#include <cstdio>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
#include "libcufetch/common.hh"
#include "libcufetch/fmt/ranges.h"
#include "util.hpp"

bool PluginManager::has_deps()
{
    for (const std::string_view bin : dependencies) // expand in the future
    {
        if (which(bin) == UNKNOWN)
            return false;
    }

    return true;
}

void PluginManager::create_cache_dir()
{
    if (!std::filesystem::exists(m_cache_path))
        std::filesystem::create_directories(m_cache_path);
}

void PluginManager::add_repo_plugins(const std::string& repo)
{
    if (!has_deps())
        die("Not all dependencies have been installed. You'll need to install {}", fmt::join(dependencies, ", "));

    const std::filesystem::path& working_dir = (m_cache_path/std::tmpnam(nullptr));
    std::filesystem::create_directories(working_dir);

    if (!taur_exec({"git", "clone", "--recursive", repo, working_dir.string()}, false))
        die("Failed to clone at directory '{}'", working_dir.string());
}
