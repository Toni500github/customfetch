#include <filesystem>
#include <string>
#include <string_view>
#include "query.hpp"
#include "packages.hpp"

std::string get_all_pkgs(System::pkg_managers_t& pkg_manager, const Config& config) {
    std::string ret;
    const std::string& bin_path{"/usr/bin/"};

    for (const std::string_view str : config.pkgs_managers) 
    { 
        if (str == "pacman" && std::filesystem::exists(bin_path + "pacman")) {
            for (auto const& _ : std::filesystem::directory_iterator{"/var/lib/pacman/local"})
                pkg_manager.pacman_pkgs++;
            
            // remove /var/lib/pacman/local/ALPM_DB_VERSION count
            pkg_manager.pacman_pkgs -= 1;
            ret += fmt::format("{} (pacman), ", pkg_manager.pacman_pkgs);
        }

        if (str == "flatpak" && std::filesystem::exists(bin_path + "flatpak")) {
            for (auto const& _ : std::filesystem::directory_iterator{"/var/lib/flatpak/app"})
                pkg_manager.flatpak_pkgs++;

            if (pkg_manager.flatpak_pkgs > 0)
                ret += fmt::format("{} (flatpak), ", pkg_manager.flatpak_pkgs);
        }

    }

    if (ret.empty())
        return MAGIC_LINE;

    ret.erase(ret.length() - 2); // remove last ", "

    return ret;
}
