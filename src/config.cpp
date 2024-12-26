/*
 * Copyright 2024 Toni500git
 * 
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "config.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "util.hpp"

Config::Config(const std::string_view configFile, const std::string_view configDir, colors_t& colors)
{
    if (!std::filesystem::exists(configDir))
    {
        warn("customfetch config folder was not found, Creating folders at {}!", configDir);
        std::filesystem::create_directories(configDir);
    }

    if (!std::filesystem::exists(configFile))
    {
        warn("config file {} not found, generating new one", configFile);
        this->generateConfig(configFile);
    }

    this->loadConfigFile(configFile, colors);
}

void Config::loadConfigFile(const std::string_view filename, colors_t& colors)
{
    try
    {
        this->tbl = toml::parse_file(filename);
    }
    catch (const toml::parse_error& err)
    {
        error("Parsing config file {} failed:", filename);
        std::cerr << err << std::endl;
        exit(-1);
    }

    // clang-format off
    // Idk but with `this->` looks more readable
    this->layout              = getValueArrayStr("config.layout", {});
    this->percentage_colors   = getValueArrayStr("config.percentage-colors", {"green", "yellow", "red"});
    this->gui                 = getValue<bool>("gui.enable", false);
    this->slow_query_warnings = getValue<bool>("config.slow-query-warnings", false);
    this->sep_reset_after     = getValue<bool>("config.sep-reset-after", false);
    this->use_SI_unit         = getValue<bool>("config.use-SI-byte-unit", false);
    this->wrap_lines          = getValue<bool>("config.wrap-lines", false);
    this->offset              = getValue<std::uint16_t>("config.offset", 5);
    this->logo_padding_left   = getValue<std::uint16_t>("config.logo-padding-left", 0);
    this->layout_padding_top  = getValue<std::uint16_t>("config.layout-padding-top", 0);
    this->logo_padding_top    = getValue<std::uint16_t>("config.logo-padding-top", 0);
    this->sep_reset           = getValue<std::string>("config.sep-reset", ":");
    this->ascii_logo_type     = getValue<std::string>("config.ascii-logo-type", "");
    this->source_path         = getValue<std::string>("config.source-path", "os");
    this->logo_position       = getValue<std::string>("config.logo-position", "left");
    this->data_dir            = getValue<std::string>("config.data-dir", get_data_dir("customfetch"));
    this->builtin_title_sep   = getValue<std::string>("config.title-sep", "-");
    this->font                = getValue<std::string>("gui.font", "Liberation Mono Normal 12");
    this->gui_bg_image        = getValue<std::string>("gui.bg-image", "disable");

    this->uptime_d_fmt = getValue<std::string>("os.uptime.days", " days");
    this->uptime_h_fmt = getValue<std::string>("os.uptime.hours", " hours");
    this->uptime_m_fmt = getValue<std::string>("os.uptime.mins", " mins");
    this->uptime_s_fmt = getValue<std::string>("os.uptime.secs", " secs");

    this->pkgs_managers= getValueArrayStr("os.pkgs.pkg-managers", {});
    this->pacman_dirs  = getValueArrayStr("os.pkgs.pacman-dirs",  {"/var/lib/pacman/local"});
    this->dpkg_files   = getValueArrayStr("os.pkgs.dpkg-files",   {"/var/lib/dpkg/status"});
    this->flatpak_dirs = getValueArrayStr("os.pkgs.flatpak-dirs", {"/var/lib/flatpak/app", "~/.local/share/flatpak/app"});
    this->apk_files    = getValueArrayStr("os.pkgs.apk-files",    {"/var/lib/apk/db/installed"});

    colors.black       = getThemeValue("config.black",   "\033[1;30m");
    colors.red         = getThemeValue("config.red",     "\033[1;31m");
    colors.green       = getThemeValue("config.green",   "\033[1;32m");
    colors.yellow      = getThemeValue("config.yellow",  "\033[1;33m");
    colors.blue        = getThemeValue("config.blue",    "\033[1;34m");
    colors.magenta     = getThemeValue("config.magenta", "\033[1;35m");
    colors.cyan        = getThemeValue("config.cyan",    "\033[1;36m");
    colors.white       = getThemeValue("config.white",   "\033[1;37m");

    colors.gui_black   = getThemeValue("gui.black",   "!#000005");
    colors.gui_red     = getThemeValue("gui.red",     "!#ff2000");
    colors.gui_green   = getThemeValue("gui.green",   "!#00ff00");
    colors.gui_blue    = getThemeValue("gui.blue",    "!#00aaff");
    colors.gui_cyan    = getThemeValue("gui.cyan",    "!#00ffff");
    colors.gui_yellow  = getThemeValue("gui.yellow",  "!#ffff00");
    colors.gui_magenta = getThemeValue("gui.magenta", "!#ff11cc");
    colors.gui_white   = getThemeValue("gui.white",   "!#ffffff");

    if (this->percentage_colors.size() < 3)
    {
        warn("the config array percentage-colors doesn't have 3 colors for being used in percentage tag and modules\n"
             "backing up to green, yellow and red");
        this->percentage_colors = {"green", "yellow", "red"};
    }

    for (const std::string& str : this->getValueArrayStr("config.alias-colors", {}))
        this->addAliasColors(str);

    const char *no_color = std::getenv("NO_COLOR");
    if (no_color != NULL && no_color[0] != '\0')
        this->m_disable_colors = true;
}

std::string Config::getThemeValue(const std::string_view value, const std::string_view fallback) const
{
    return this->tbl.at_path(value).value<std::string>().value_or(fallback.data());
}

std::vector<std::string> Config::getValueArrayStr(const std::string_view          value,
                                                  const std::vector<std::string>& fallback)
{
    std::vector<std::string> ret;

    // https://stackoverflow.com/a/78266628
    const auto& array = tbl.at_path(value);
    if (const toml::array* array_it = array.as_array())
    {
        array_it->for_each(
            [&ret, value](auto&& el)
            {
                if (const toml::value<std::string>* str_elem = el.as_string())
                {
                    const toml::value<std::string>& v = *str_elem;
                    ret.push_back(v->data());
                }
                else
                    warn("An element of the {} array variable is not a string", value);
            }
        );
    }
    else
        return fallback;

    return ret;
}

void Config::addAliasColors(const std::string& str)
{
    const size_t pos = str.find('=');
    if (pos == std::string::npos)
        die("alias color '{}' does NOT have an equal sign '=' for separating color name and value.\n"
            "for more check with --help", str);

    const std::string& name  = str.substr(0, pos);
    const std::string& value = str.substr(pos + 1);

    this->colors_name.push_back(name);
    this->colors_value.push_back(value);
}

void Config::generateConfig(const std::string_view filename)
{
#if !ANDROID_APP
    if (std::filesystem::exists(filename))
    {
        if (!askUserYorN(false, "WARNING: config file {} already exists. Do you want to overwrite it?", filename))
            std::exit(1);
    }
#endif

    std::ofstream f(filename.data(), std::ios::trunc);
    f << AUTOCONFIG;
    f.close();
}
