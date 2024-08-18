#include "packages.hpp"

#include <filesystem>
#include <fstream>
#include <string>

#include "query.hpp"
#include "util.hpp"

std::string get_all_pkgs(System::pkg_managers_t& pkg_managers, const Config& config)
{
    std::string ret;

    for (const std::string& name : config.pkgs_managers)
    {
        if (name == "pacman" && std::filesystem::exists("/var/lib/pacman/local"))
        {
            pkg_managers.pacman_pkgs = std::distance(std::filesystem::directory_iterator{"/var/lib/pacman/local"}, {});
            // remove /var/lib/pacman/local/ALPM_DB_VERSION count
            pkg_managers.pacman_pkgs--;
            ret += fmt::format("{} (pacman), ", pkg_managers.pacman_pkgs);
        }

        else if (name == "flatpak" && std::filesystem::exists("/var/lib/flatpak/app"))
        {
            pkg_managers.flatpak_pkgs = std::distance(std::filesystem::directory_iterator{"/var/lib/flatpak/app"}, {});

            if (pkg_managers.flatpak_pkgs > 0)
                ret += fmt::format("{} (flatpak), ", pkg_managers.flatpak_pkgs);
        }

        else if (name == "dpkg" && std::filesystem::exists("/var/lib/dpkg/status"))
        {
            std::ifstream f("/var/lib/dpkg/status", std::ios::in);
            std::string line;
            while (std::getline(f, line))
            {
                if (line == "Status: install ok installed")
                    pkg_managers.dpkg_pkgs++;
            }

            if (pkg_managers.dpkg_pkgs > 0)
                ret += fmt::format("{} (dpkg), ", pkg_managers.dpkg_pkgs);
        }

    }

    if (ret.empty())
        return MAGIC_LINE;

    ret.erase(ret.length() - 2);  // remove last ", "

    return ret;
}
