#include "packages.hpp"

#include <filesystem>
#include <fstream>
#include <algorithm>
#include <string>

#include "switch_fnv1a.hpp"

static size_t get_num_count_dir(const std::string_view path)
{
    if (!std::filesystem::exists(path))
        return 0;

    const auto& dirIter = std::filesystem::directory_iterator{path};

    return std::count_if(begin(dirIter), end(dirIter),
                        [](const auto& entry) { return entry.is_directory(); });
}

static size_t get_num_string_file(const std::string_view path, const std::string_view str)
{
    size_t ret = 0;
    std::ifstream f(path.data());
    if (!f.is_open())
        return 0;

    std::string line;
    while (std::getline(f, line))
    {
        if (line == str)
            ret++;
    }

    return ret;
}

std::string get_all_pkgs(const Config& config)
{
    std::string ret;
    pkgs_managers_count_t pkgs_count;

#define ADD_PKGS_COUNT(pkgman) \
    if (pkgs_count.pkgman > 0) \
        ret += fmt::format("{} ({}), ", pkgs_count.pkgman, #pkgman);

    for (const std::string& name : config.pkgs_managers)
    {
        switch (fnv1a16::hash(name))
        {
            case "pacman"_fnv1a16:
                pkgs_count.pacman = get_num_count_dir("/var/lib/pacman/local");
                ADD_PKGS_COUNT(pacman);
                break;

            case "flatpak"_fnv1a16:
                pkgs_count.flatpak += get_num_count_dir("/var/lib/flatpak/app");
                pkgs_count.flatpak += get_num_count_dir(expandVar("~/.local/share/flatpak/app"));
                ADD_PKGS_COUNT(flatpak);
                break;

            case "dpkg"_fnv1a16:
                pkgs_count.dpkg = get_num_string_file("/var/lib/dpkg/status", "Status: install ok installed");
                ADD_PKGS_COUNT(dpkg);
                break;

            case "apk"_fnv1a16:
                pkgs_count.apk = get_num_string_file("/var/lib/apk/db/installed", "C:Q");
                ADD_PKGS_COUNT(apk);
                break;
        }
    }

    if (ret.empty())
        return MAGIC_LINE;

    ret.erase(ret.length() - 2);  // remove last ", "

    return ret;
}
