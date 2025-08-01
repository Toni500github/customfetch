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

#include <cstdlib>
#include <filesystem>

#include "fmt/compile.h"
#include "fmt/os.h"
#include "fmt/ranges.h"
#include "libcufetch/common.hh"
#include "manifest.hpp"
#include "pluginManager.hpp"
#include "stateManager.hpp"
#include "texts.hpp"

#if (!__has_include("version.h"))
#error "version.h not found, please generate it with ./scripts/generateVersion.sh"
#else
#include "version.h"
#endif

#include "getopt_port/getopt.h"

enum Ops
{
    NONE,
    INSTALL,
    LIST,
    ENABLE,
    DISABLE,
    GEN_MANIFEST,
} op = NONE;

void version()
{
    fmt::print(
        "cufetchpm {} built from branch '{}' at {} commit '{}' ({}).\n"
        "Date: {}\n"
        "Tag: {}\n",
        VERSION, GIT_BRANCH, GIT_DIRTY, GIT_COMMIT_HASH, GIT_COMMIT_MESSAGE, GIT_COMMIT_DATE, GIT_TAG);

    // if only everyone would not return error when querying the program version :(
    std::exit(EXIT_SUCCESS);
}

void help(int invalid_opt = false)
{
    fmt::print(FMT_COMPILE("{}"), cufetchpm_help);

    std::exit(invalid_opt);
}

void help_install(int invalid_opt = false)
{
    fmt::print(FMT_COMPILE("{}"), cufetchpm_help_install);

    std::exit(invalid_opt);
}

void help_list(int invalid_opt = false)
{
    fmt::print(FMT_COMPILE("{}"), cufetchpm_help_list);

    std::exit(invalid_opt);
}

bool parse_install_args(int argc, char* argv[])
{
    // clang-format off
    const struct option long_opts[] = {
        {"force", no_argument, nullptr, 'f'},
        {"help",  no_argument, nullptr, 'h'},
        {0, 0, 0, 0}
    };
    // clang-format on

    int opt;
    while ((opt = getopt_long(argc, argv, "-fh", long_opts, nullptr)) != -1)
    {
        switch (opt)
        {
            case 'f': options.install_force = true; break;
            case 'h': help_install(EXIT_SUCCESS); break;
            case '?': help_install(EXIT_FAILURE); break;
        }
    }

    for (int i = optind; i < argc; ++i)
        options.arguments.emplace_back(argv[i]);

    if (options.arguments.empty())
        die("install: no repositories/paths given");

    return true;
}

bool parse_list_args(int argc, char* argv[])
{
    // clang-format off
    const struct option long_opts[] = {
        {"verbose", no_argument, nullptr, 'v'},
        {"help",    no_argument, nullptr, 'h'},
        {0, 0, 0, 0}
    };
    // clang-format on

    int opt;
    while ((opt = getopt_long(argc, argv, "-vh", long_opts, nullptr)) != -1)
    {
        switch (opt)
        {
            case 'v': options.list_verbose = true; break;
            case 'h': help_list(EXIT_SUCCESS); break;
            case '?': help_list(EXIT_FAILURE); break;
        }
    }

    return true;
}

bool parse_switch_plugin_args(int argc, char* argv[])
{
    // clang-format off
    const struct option long_opts[] = {
        {"help",  no_argument, nullptr, 'h'},
        {0, 0, 0, 0}
    };
    // clang-format on

    int opt;
    while ((opt = getopt_long(argc, argv, "-h", long_opts, nullptr)) != -1)
    {
        switch (opt)
        {
            case 'h': help(EXIT_SUCCESS); break;
            case '?': help_install(EXIT_FAILURE); break;
        }
    }

    for (int i = optind; i < argc; ++i)
        options.arguments.emplace_back(argv[i]);

    if (options.arguments.empty())
        die("Please provide a source/plugin to enable/disable");

    return true;
}

static bool parseargs(int argc, char* argv[])
{
    // clang-format off
    int opt = 0;
    int option_index = 0;
    const char *optstring = "-Vh";
    static const struct option opts[] = {
        {"version", no_argument, 0, 'V'},
        {"help",    no_argument, 0, 'h'},

        {0,0,0,0}
    };

    // clang-format on
    optind = 1;
    while ((opt = getopt_long(argc, argv, optstring, opts, &option_index)) != -1)
    {
        switch (opt)
        {
            case 0:   break;
            case '?': help(EXIT_FAILURE); break;

            case 'V': version(); break;
            case 'h': help(); break;
            default:  return false;
        }
    }

    if (optind >= argc)
        help(EXIT_FAILURE);  // no subcommand

    std::string_view cmd      = argv[optind];
    int              sub_argc = argc - optind - 1;
    char**           sub_argv = argv + optind + 1;
    if (cmd == "install" || cmd == "i")
    {
        op     = INSTALL;
        optind = 0;
        return parse_install_args(sub_argc, sub_argv);
    }
    else if (cmd == "list" || cmd == "l")
    {
        op     = LIST;
        optind = 0;
        return parse_list_args(sub_argc, sub_argv);
    }
    else if (cmd == "gen-manifest")
    {
        op     = GEN_MANIFEST;
        optind = 0;
    }
    else if (cmd == "enable")
    {
        op     = ENABLE;
        optind = 0;
        return parse_switch_plugin_args(sub_argc, sub_argv);
    }
    else if (cmd == "disable")
    {
        op     = DISABLE;
        optind = 0;
        return parse_switch_plugin_args(sub_argc, sub_argv);
    }
    else if (cmd == "help")
    {
        if (sub_argc >= 1)
        {
            std::string_view target = sub_argv[0];
            if (target == "install")
                help_install();
            else if (target == "list")
                help_list();
            else
                die("Couldn't find help text for subcommand '{}'", cmd);
        }
        else
        {
            help(EXIT_FAILURE);
        }
    }

    return true;
}

void switch_plugin(StateManager&& state, bool switch_)
{
    const char*        switch_str = switch_ ? "Enabl" : "Disabl";  // e/ed/ing
    const toml::table& tbl        = state.get_state();

    for (const std::string& arg : options.arguments)
    {
        const size_t pos = arg.find('/');
        if (pos == arg.npos)
            die("Plugin to {}e '{}' doesn't have a slash '/' to separate source and plugin", switch_str, arg);

        const std::string& source = arg.substr(0, pos);
        const std::string& plugin = arg.substr(pos + 1);

        const auto* source_tbl = tbl["repositories"][source].as_table();
        if (!source_tbl)
            die("Couldn't find source '{}'", source);
        if (const auto* plugins_arr_tbl = source_tbl->get_as<toml::array>("plugins"))
        {
            for (const auto& plugin_node : *plugins_arr_tbl)
            {
                const toml::table* plugin_tbl = plugin_node.as_table();
                if (!plugin_tbl || ManifestSpace::getStrValue(*plugin_tbl, "name") != plugin)
                    continue;

                for (const fs::path path : ManifestSpace::getStrArrayValue(*plugin_tbl, "libraries"))
                {
                    fs::path base_path = path;
                    if (base_path.extension() == ".disabled")
                        base_path.replace_extension();  // normalize to enabled form

                    const fs::path& enabled_path  = base_path;
                    const fs::path& disabled_path = base_path.string() + ".disabled";

                    fs::path current_path;
                    if (fs::exists(enabled_path))
                        current_path = enabled_path;
                    else if (fs::exists(disabled_path))
                        current_path = disabled_path;
                    else
                    {
                        warn("Plugin library '{}' not found. Skipping", base_path.string());
                        continue;
                    }

                    const fs::path& target_path = switch_ ? enabled_path : disabled_path;
                    if (current_path == target_path)
                    {
                        warn("{} is already {}ed", arg, switch_str);
                        continue;
                    }

                    fs::rename(current_path, target_path);
                    info("{}ed {}!", switch_str, arg);
                }
            }
        }
    }
}

void list_all_plugins(StateManager&& state)
{
    if (options.list_verbose)
    {
        for (const manifest_t& manifest : state.get_all_repos())
        {
            fmt::println("\033[1;32mRepository:\033[0m {}", manifest.name);
            fmt::println("\033[1;33mURL:\033[0m {}", manifest.url);
            fmt::println("\033[1;34mPlugins:");
            for (const plugin_t& plugin : manifest.plugins)
            {
                fmt::println("\033[1;34m - {}\033[0m", plugin.name);
                fmt::println("\t\033[1;35mDescription:\033[0m {}", plugin.description);
                fmt::println("\t\033[1;36mAuthor(s):\033[0m {}", fmt::join(plugin.authors, ", "));
                fmt::println("\t\033[1;38;2;220;220;220mLicense(s):\033[0m {}", fmt::join(plugin.licenses, ", "));
                fmt::println("\t\033[1;38;2;144;238;144mPrefixe(s):\033[0m {}", fmt::join(plugin.prefixes, ", "));
                fmt::print("\n");
            }
            fmt::print("\033[0m");
        }
    }
    else
    {
        for (const manifest_t& manifest : state.get_all_repos())
        {
            fmt::println("\033[1;32mRepository:\033[0m {} (\033[1;33m{}\033[0m)", manifest.name, manifest.url);
            fmt::println("\033[1;34mPlugins:");
            for (const plugin_t& plugin : manifest.plugins)
                fmt::println("   \033[1;34m{} - \033[1;35m{}\n", plugin.name, plugin.description);
            fmt::print("\033[0m");
        }
    }
}

int main(int argc, char* argv[])
{
    if (!parseargs(argc, argv))
        return -1;

    fs::create_directories({ getHomeCacheDir() / "cufetchpm" / "plugins" });
    fs::create_directories({ getConfigDir() / "plugins" });
    StateManager state;
    switch (op)
    {
        case INSTALL:
        {
            if (options.arguments.size() < 1)
                die("Please provide a git url repository");
            PluginManager plugin_manager(std::move(state));
            for (const std::string& arg : options.arguments)
            {
                if (fs::exists(arg))
                    plugin_manager.build_plugins(arg);
                else
                    plugin_manager.add_repo_plugins(arg);
            }
            break;
        }
        case LIST:
        {
            list_all_plugins(std::move(state));
            break;
        }
        case GEN_MANIFEST:
        {
            auto f = fmt::output_file("cufetchpm.toml", fmt::file::CREATE | fmt::file::WRONLY);
            f.print("{}", AUTO_MANIFEST);
            f.close();
            break;
        }
        case ENABLE:
        {
            switch_plugin(std::move(state), true);
            break;
        }
        case DISABLE:
        {
            switch_plugin(std::move(state), false);
            break;
        }
        default: warn("uh?");
    }

    return 0;
}
