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

#include "packages.hh"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>

#include "switch_fnv1a.hpp"
#include "util.hpp"

static size_t get_num_count_dir(const std::string_view path)
{
    if (!std::filesystem::exists(path))
        return 0;

    const auto& dirIter = std::filesystem::directory_iterator{ path };

    return std::count_if(begin(dirIter), end(dirIter), [](const auto& entry) { return entry.is_directory(); });
}

static size_t get_num_string_file(const std::string_view path, const std::string_view str)
{
    size_t        ret = 0;
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

#define ADD_PKGS_COUNT(pkgman) \
    if (pkgs_count.pkgman > 0) \
        ret += fmt::format("{} ({}), ", pkgs_count.pkgman, #pkgman);

std::string get_all_pkgs(const Config& config)
{
    std::string           ret;
    pkgs_managers_count_t pkgs_count;

    for (const std::string& name : config.pkgs_managers)
    {
        switch (fnv1a16::hash(name))
        {
            case "pacman"_fnv1a16:
                for (const std::string& str : config.pacman_dirs)
                    pkgs_count.pacman += get_num_count_dir(expandVar(str));
                ADD_PKGS_COUNT(pacman);
                break;

            case "flatpak"_fnv1a16:
                for (const std::string& str : config.flatpak_dirs)
                    pkgs_count.flatpak += get_num_count_dir(expandVar(str));
                ADD_PKGS_COUNT(flatpak);
                break;

            case "dpkg"_fnv1a16:
                for (const std::string& str : config.dpkg_files)
                    pkgs_count.dpkg += get_num_string_file(expandVar(str), "Status: install ok installed");
                ADD_PKGS_COUNT(dpkg);
                break;

            case "apk"_fnv1a16:
                for (const std::string& str : config.apk_files)
                    pkgs_count.apk += get_num_string_file(expandVar(str), "C:Q");
                ADD_PKGS_COUNT(apk);
                break;
        }
    }

    if (ret.empty())
        return MAGIC_LINE;

    ret.erase(ret.length() - 2);  // remove last ", "

    return ret;
}

#undef ADD_PKGS_COUNT
