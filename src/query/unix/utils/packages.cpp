#include "platform.hpp"

#ifdef CF_UNIX

#include "packages.hpp"

#include <filesystem>
#include <string>
#include <string_view>

#include "query.hpp"
#include "util.hpp"

std::string get_all_pkgs(System::pkg_managers_t& pkg_managers, const Config& config)
{
    std::string ret;

    for (std::string_view pkg_manager : config.pkgs_managers)
    {
        if (pkg_manager == "pacman" && std::filesystem::exists("/var/lib/pacman/local"))
        {
            pkg_managers.pacman_pkgs = std::distance(std::filesystem::directory_iterator{"/var/lib/pacman/local"}, {});

            // remove /var/lib/pacman/local/ALPM_DB_VERSION count
            pkg_managers.pacman_pkgs--;
            ret += fmt::format("{} (pacman), ", pkg_managers.pacman_pkgs);
        }

        if (pkg_manager == "flatpak" && std::filesystem::exists("/var/lib/flatpak/app"))
        {
            pkg_managers.flatpak_pkgs = std::distance(std::filesystem::directory_iterator{"/var/lib/flatpak/app"}, {});

            if (pkg_managers.flatpak_pkgs > 0)
                ret += fmt::format("{} (flatpak), ", pkg_managers.flatpak_pkgs);
        }
    }

    if (ret.empty())
        return MAGIC_LINE;

    ret.erase(ret.length() - 2);  // remove last ", "

    return ret;
}
#endif
