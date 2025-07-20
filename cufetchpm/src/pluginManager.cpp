#include "pluginManager.hpp"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include "tiny-process-library/process.hpp"
#include "fmt/format.h"
#include "manifest.hpp"
#include "libcufetch/common.hh"
//#include "fmt/ranges.h"

using namespace TinyProcessLib;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dist(0, 999999);

bool PluginManager::has_deps()
{
    for (const std::string_view bin : dependencies)
    {
        Process proc(fmt::format("command -v {}", bin),
                     "",
                    [](const char*, size_t) {}, // discard stdout
                    [](const char*, size_t) {});  // discard stderr
        if (proc.get_exit_status() != 0)
            return false;
    }

    return true;
}

void PluginManager::add_repo_plugins(const std::string& repo)
{
    if (!has_deps())
        die("Not all dependencies have been installed. You'll need to install git"); // fmt::join(dependencies, ", "));

    std::string repo_name;
    {
        size_t pos = repo.rfind('/');
        if (pos == repo.npos)
            die("Is the url valid? Please give us a valid repository url");
        repo_name = repo.substr(pos + 1);
        if (hasEnding(repo_name, ".git"))
            repo_name.erase(repo_name.length() - ".git"_len);
    }

    if (std::filesystem::exists(m_cache_path / repo_name))
    {
        warn("Repository '{}' already exists in '{}'", repo_name, (m_cache_path / repo_name).string());
        return;
    }

    const std::filesystem::path& working_dir = m_cache_path / ("plugin_" + std::to_string(dist(gen)));
    std::filesystem::create_directories(working_dir);

    status("Cloning repository '{}' at '{}'", repo, working_dir.string());
    if (Process({"git", "clone", "--recursive", repo, working_dir.string()}).get_exit_status() != 0)
    {
        std::filesystem::remove_all(working_dir);
        die("Failed to clone at directory '{}'", working_dir.string());
    }
    success("Successfully cloned. Changing current directory");

    std::filesystem::current_path(working_dir);
    CManifest manifest(MANIFEST_NAME);

    status("Querying all plugins declared from the manifest");
    const std::vector<manifest_t>& plugins = manifest.get_all_plugins();
    if (plugins.empty())
    {
        std::filesystem::remove_all(working_dir);
        die("Looks like there are no plugins to build with '{}'", repo_name);
    }
    success("Queried the plugins");

    for (const manifest_t& plugin : plugins)
    {
        status("Trying to build plugin '{}'", plugin.name);
        for (const std::string& bs : plugin.build_steps)
        {
            Process process(bs, "", nullptr);
            if (process.get_exit_status() != 0)
            {   
                std::filesystem::remove_all(working_dir);
                die("Failed to build plugin '{}' from '{}'", plugin.name, repo);
            }
        }
        success("Successfully built '{}'", plugin.name);
    }
}
