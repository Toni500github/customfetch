#include <filesystem>
#include <string>
#include <string_view>
#include <dirent.h>
#include "query.hpp"
#include "util.hpp"
#include "packages.hpp"

std::string get_all_pkgs(System::pkg_managers_t& pkg_manager, const Config& config) {
    std::string ret;
    DIR *dir;
    struct dirent *entry;

    for (std::string_view str : config.pkgs_managers)
    {
        str = str_tolower(str.data());
        if (str == "pacman" && std::filesystem::exists("/var/lib/pacman/local")) {
            dir = opendir("/var/lib/pacman/local");
            while((entry = readdir(dir)) != NULL)
                pkg_manager.pacman_pkgs++;

            // remove /var/lib/pacman/local/ALPM_DB_VERSION count and accounting for ..
            pkg_manager.pacman_pkgs -= 2;
            ret += fmt::format("{} (pacman), ", pkg_manager.pacman_pkgs);
        }

        if (str == "flatpak" && std::filesystem::exists("/var/lib/flatpak/app")) {
            pkg_manager.flatpak_pkgs = std::distance(std::filesystem::directory_iterator{"/var/lib/flatpak/app"}, {});

            if (pkg_manager.flatpak_pkgs > 0)
                ret += fmt::format("{} (flatpak), ", pkg_manager.flatpak_pkgs);
        }

    }

    if (ret.empty())
        return MAGIC_LINE;

    ret.erase(ret.length() - 2); // remove last ", "

    return ret;
}
