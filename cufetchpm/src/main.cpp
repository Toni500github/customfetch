#include <cstdlib>
#include <filesystem>

#include "fmt/ranges.h"
#include "libcufetch/common.hh"
#include "fmt/os.h"
#include "fmt/compile.h"
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
                die("Unknown subcommand '{}'", cmd);
        }
        else
        {
            help(EXIT_FAILURE);
        }
    }

    return true;
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
                        fmt::println("\t\033[1;38;2;220;220;220mLicense(s):\033[0m {}",
                                     fmt::join(plugin.licenses, ", "));
                        fmt::println("\t\033[1;38;2;144;238;144mPrefixe(s):\033[0m {}",
                                     fmt::join(plugin.prefixes, ", "));
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
            break;
        }
        case GEN_MANIFEST:
        {
            auto f = fmt::output_file("cufetchpm.toml", fmt::file::CREATE | fmt::file::WRONLY);
            f.print("{}", AUTO_MANIFEST);
            f.close();
            break;
        }
        default: warn("uh?");
    }

    return 0;
}
