#include "pluginManager.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include "fmt/format.h"
#include "libcufetch/common.hh"
#include "manifest.hpp"
#include "tiny-process-library/process.hpp"
// #include "fmt/ranges.h"

using namespace TinyProcessLib;

bool PluginManager::has_deps()
{
    for (const std::string_view bin : dependencies)
    {
        Process proc(fmt::format("command -v {}", bin),
                     "",
                    [](const char*, size_t) {},   // discard stdout
                    [](const char*, size_t) {});  // discard stderr
        if (proc.get_exit_status() != 0)
            return false;
    }

    return true;
}

void PluginManager::add_repo_plugins(const std::string& repo)
{
    if (!has_deps())
        die("Not all dependencies have been installed. You'll need to install git");  // fmt::join(dependencies, ", "));

    static std::random_device              rd;
    static std::mt19937                    gen(rd());
    static std::uniform_int_distribution<> dist(0, 999999);

    // create temponary directory
    const std::filesystem::path& working_dir = m_cache_path / ("plugin_" + std::to_string(dist(gen)));
    std::filesystem::create_directories(working_dir);

    // and lets clone the repository
    status("Cloning repository '{}' at '{}'", repo, working_dir.string());
    if (Process({ "git", "clone", "--recursive", repo, working_dir.string() }).get_exit_status() != 0)
    {
        std::filesystem::remove_all(working_dir);
        die("Failed to clone at directory '{}'", working_dir.string());
    }
    success("Successfully cloned. Changing current directory to '{}'", working_dir.string());

    // cd to the working directory and parse its manifest
    std::filesystem::current_path(working_dir);
    CManifest manifest(MANIFEST_NAME);

    // though lets check if we have already installed the plugin in the cache
    const std::filesystem::path& repo_path = (m_cache_path / manifest.get_repo_name());
    if (std::filesystem::exists(repo_path))
    {
        warn("Repository '{}' already exists in '{}'", manifest.get_repo_name(), repo_path.string());
        return;
    }

    // So we don't have any plugins in the manifest uh
    if (manifest.get_all_plugins().empty())
    {
        std::filesystem::remove_all(working_dir);
        die("Looks like there are no plugins to build with '{}'", manifest.get_repo_name());
    }

    // build each plugin from the manifest
    // and add the infos to the state.toml
    for (const plugin_t& plugin : manifest.get_all_plugins())
    {
        status("Trying to build plugin '{}'", plugin.name);
        for (const std::string& bs : plugin.build_steps)
        {
            if (Process(bs, "").get_exit_status() != 0)
            {
                std::filesystem::remove_all(working_dir);
                die("Failed to build plugin '{}' from '{}'", plugin.name, repo);
            }
        }
        success("Successfully built '{}' into '{}'", plugin.name, plugin.output_dir);
    }
    m_state.add_new_repo(manifest);

    // we built all plugins. let's rename the working directory to its actual manifest name,
    success("Repository plugins successfully built! Moving to '{}'...", repo_path.string());
    std::filesystem::create_directories(repo_path);
    std::filesystem::rename(working_dir, repo_path);

    // and then we symlink each plugin built library from its output-dir
    const std::filesystem::path& plugin_config_path = (m_config_path / manifest.get_repo_name());
    std::filesystem::create_directories(plugin_config_path);
    status("Linking each built plugin to '{}'", plugin_config_path.string());
    for (const plugin_t& plugin : manifest.get_all_plugins())
    {
        if (!std::filesystem::exists(plugin.output_dir))
            die("Plugin '{}' output-dir '{}' doesn't exist", plugin.name, plugin.output_dir);
        for (const auto& library : std::filesystem::directory_iterator{ plugin.output_dir })
        {
            if (library.is_regular_file() || library.is_symlink())
                std::filesystem::create_symlink(std::filesystem::canonical(library),
                                                plugin_config_path / library.path().filename());
            else
                error("Built library '{}' is not a regular file", library.path().string());
        }
    }
    success("Enjoy the new plugins from {}", manifest.get_repo_name());
}
