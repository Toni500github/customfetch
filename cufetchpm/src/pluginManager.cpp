#include "pluginManager.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include "fmt/format.h"
#include "fmt/ranges.h"
#include "libcufetch/common.hh"
#include "manifest.hpp"
#include "tiny-process-library/process.hpp"
#include "util.hpp"

using namespace TinyProcessLib;

bool PluginManager::has_deps(const std::vector<std::string>& dependencies)
{
    for (const std::string& bin : dependencies)
    {
        Process proc(
            fmt::format("command -v {}", bin), "", [](const char*, size_t) {},  // discard stdout
            [](const char*, size_t) {});                                        // discard stderr
        if (proc.get_exit_status() != 0)
            return false;
    }

    return true;
}

void PluginManager::add_repo_plugins(const std::string& repo)
{
    if (!has_deps(core_dependencies))
        die("Some core dependencies are not installed. You'll need to install: {}", fmt::join(core_dependencies, ", "));

    static std::random_device              rd;
    static std::mt19937                    gen(rd());
    static std::uniform_int_distribution<> dist(0, 999999);

    // create temponary directory
    const fs::path& working_dir = m_cache_path / ("plugin_" + std::to_string(dist(gen)));
    fs::create_directories(working_dir);

    // and lets clone the repository
    status("Cloning repository '{}' at '{}'", repo, working_dir.string());
    if (Process({ "git", "clone", "--recursive", repo, working_dir.string() }).get_exit_status() != 0)
    {
        fs::remove_all(working_dir);
        die("Failed to clone at directory '{}'", working_dir.string());
    }
    success("Successfully cloned. Changing current directory to '{}'", working_dir.string());
    build_plugins(working_dir);
}

void PluginManager::build_plugins(const fs::path& working_dir)
{
    std::vector<std::string> non_supported_plugins;

    // cd to the working directory and parse its manifest
    fs::current_path(working_dir);
    CManifest manifest(MANIFEST_NAME);

    // though lets check if we have already installed the plugin in the cache
    const fs::path& repo_cache_path     = (m_cache_path / manifest.get_repo_name());
    const fs::path& plugins_config_path = (m_config_path / manifest.get_repo_name());
    if (fs::exists(repo_cache_path))
    {
        if (!options.install_force)
        {
            warn("Repository '{}' already exists in '{}'", manifest.get_repo_name(), repo_cache_path.string());
            fs::remove_all(working_dir);
            return;
        }

        fs::remove_all(repo_cache_path);
    }

    // So we don't have any plugins in the manifest uh
    if (manifest.get_all_plugins().empty())
    {
        fs::remove_all(working_dir);
        die("Looks like there are no plugins to build with '{}'", manifest.get_repo_name());
    }

    if (!has_deps(manifest.get_dependencies()))
    {
        fs::remove_all(working_dir);
        die("Some dependencies for repository '{}' are not installed. The repository requires the following: {}",
            manifest.get_repo_name(), fmt::join(manifest.get_dependencies(), ", "));
    }

    // build each plugin from the manifest
    // and add the infos to the state.toml
    for (const plugin_t& plugin : manifest.get_all_plugins())
    {
        bool found_platform = false;

        if (plugin.platforms[0] != "all")
        {
            for (const std::string& plugin_platform : plugin.platforms)
                if (plugin_platform == PLATFORM)
                    found_platform = true;

            if (!found_platform)
            {
                warn("Plugin '{}' doesn't support the platform '{}'. Skipping", plugin.name, PLATFORM);
                non_supported_plugins.push_back(plugin.name);
                continue;
            }
        }

        status("Trying to build plugin '{}'", plugin.name);
        for (const std::string& bs : plugin.build_steps)
        {
            if (Process(bs, "").get_exit_status() != 0)
            {
                fs::remove_all(working_dir);
                die("Failed to build plugin '{}'", plugin.name);
            }
        }
        success("Successfully built '{}' into '{}'", plugin.name, plugin.output_dir);
    }
    m_state_manager.add_new_repo(manifest);

    // we built all plugins. let's rename the working directory to its actual manifest name,
    success("Repository plugins successfully built! Renaming to '{}'...", repo_cache_path.string());
    fs::create_directories(repo_cache_path);
    fs::rename(working_dir, repo_cache_path);

    // and then we move each plugin built library from its output-dir
    // and we'll declare all plugins we have moved.
    fs::create_directories(plugins_config_path);
    status("Moving each built plugin to '{}'", plugins_config_path.string());
    for (const plugin_t& plugin : manifest.get_all_plugins())
    {
        // already told before
        if (std::find(non_supported_plugins.begin(), non_supported_plugins.end(), plugin.name) !=
            non_supported_plugins.end())
            continue;

        // ugh, devs fault. Report this error to them
        if (!fs::exists(plugin.output_dir))
        {
            error("Plugin '{}' output-dir '{}' doesn't exist", plugin.name, plugin.output_dir);
            continue;
        }

        toml::array built_libraries;
        for (const auto& library : fs::directory_iterator{ plugin.output_dir })
        {
            // ~/.config/customfetch/plugins/<plugin-directory>/<plugin-filename>
            const fs::path& library_config_path = plugins_config_path / library.path().filename();
            if (fs::exists(library_config_path) && !options.install_force)
            {
                if (askUserYorN(false, "Plugin '{}' already exists. Replace it?", library_config_path.string()))
                    fs::remove_all(library_config_path);
                else
                    continue;
            }

            if (library.is_regular_file() || library.is_symlink())
            {
                std::error_code er;
                fs::rename(fs::canonical(library), library_config_path, er);
                if (er)
                {
                    error("Failed to move '{}' to '{}': {}", fs::canonical(library).string(),
                          library_config_path.string(), er.message());
                    continue;
                }
                built_libraries.push_back(library_config_path.string());
            }
            else
            {
                error("Built library '{}' is not a regular file", library.path().string());
            }
        }
        m_state_manager.insert_or_assign_at_plugin(manifest.get_repo_name(), plugin.name, "libraries",
                                                   std::move(built_libraries));
    }
    success("Enjoy the new plugins from {}", manifest.get_repo_name());
}
