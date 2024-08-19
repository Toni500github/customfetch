#ifndef _PACKAGES_HPP
#define _PACKAGES_HPP

#include "config.hpp"

std::string get_all_pkgs(const Config& config);

struct pkgs_managers_count_t
{
    size_t dpkg    = 0;
    size_t apk     = 0;
    size_t pacman  = 0;
    size_t flatpak = 0;
};

#endif
