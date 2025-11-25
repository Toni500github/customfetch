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

#include "config.hpp"

#include <cstdlib>
#include <filesystem>
#include <string>

#include "fmt/os.h"
#include "texts.hpp"
#include "util.hpp"

Config::Config(const std::filesystem::path& configFile, const std::filesystem::path& configDir)
{
    if (!std::filesystem::exists(configDir))
    {
        warn(_("customfetch config folder was not found, Creating folders at {}!"), configDir.string());
        std::filesystem::create_directories(configDir);
    }

    if (!std::filesystem::exists(configFile))
    {
        warn(_("config file {} not found, generating new one"), configFile.string());
        this->generateConfig(configFile);
    }
}

void Config::loadConfigFile(const std::filesystem::path& filename)
{
    try
    {
        this->tbl = toml::parse_file(filename.string());
    }
    catch (const toml::parse_error& err)
    {
        die(_("Parsing config file '{}' failed:\n"
              "{}\n"
              "\t(error occurred at line {} column {})"),
            filename.string(), err.description(), err.source().begin.line, err.source().begin.column);
    }

    // clang-format off
    // Idk but with `this->` looks more readable
    this->layout              = getValueArrayStr("config.layout", {});
    this->percentage_colors   = getValueArrayStr("config.percentage-colors", {"green", "yellow", "red"});
    this->slow_query_warnings = getValueBool("config.slow-query-warnings", false);
    this->sep_reset_after     = getValueBool("config.sep-reset-after", false);
    this->use_SI_unit         = getValueBool("config.use-SI-byte-unit", false);
    this->wrap_lines          = getValueBool("config.wrap-lines", false);
    this->logo_padding_left   = getValueInt("config.logo-padding-left", 0);
    this->layout_padding_top  = getValueInt("config.layout-padding-top", 0);
    this->logo_padding_top    = getValueInt("config.logo-padding-top", 0);
    this->offset              = expandVar(getValueStr("config.offset", "5"));
    this->sep_reset           = expandVar(getValueStr("config.sep-reset", ":"));
    this->ascii_logo_type     = expandVar(getValueStr("config.ascii-logo-type", ""));
    this->source_path         = expandVar(getValueStr("config.source-path", "os"));
    this->logo_position       = expandVar(getValueStr("config.logo-position", "left"));
    this->data_dir            = expandVar(getValueStr("config.data-dir", get_data_dir("customfetch")));
    this->title_sep           = expandVar(getValueStr("config.title-sep", "-"));
    this->gui_bg_image        = expandVar(getValueStr("gui.bg-image", "disable"));
    this->gui_css_file        = expandVar(getValueStr("gui.gtk-css",  "disable"));

    this->uptime_d_fmt = expandVar(getValueStr("os.uptime.days", " days"));
    this->uptime_h_fmt = expandVar(getValueStr("os.uptime.hours", " hours"));
    this->uptime_m_fmt = expandVar(getValueStr("os.uptime.mins", " mins"));
    this->uptime_s_fmt = expandVar(getValueStr("os.uptime.secs", " secs"));

    this->pkgs_managers= getValueArrayStr("os.pkgs.pkg-managers", {});
    this->pacman_dirs  = getValueArrayStr("os.pkgs.pacman-dirs",  {"/var/lib/pacman/local"});
    this->dpkg_files   = getValueArrayStr("os.pkgs.dpkg-files",   {"/var/lib/dpkg/status"});
    this->flatpak_dirs = getValueArrayStr("os.pkgs.flatpak-dirs", {"/var/lib/flatpak/app", "~/.local/share/flatpak/app"});
    this->apk_files    = getValueArrayStr("os.pkgs.apk-files",    {"/var/lib/apk/db/installed"});

    this->colors.black       = getValueStr("config.black",   "\033[1;30m");
    this->colors.red         = getValueStr("config.red",     "\033[1;31m");
    this->colors.green       = getValueStr("config.green",   "\033[1;32m");
    this->colors.yellow      = getValueStr("config.yellow",  "\033[1;33m");
    this->colors.blue        = getValueStr("config.blue",    "\033[1;34m");
    this->colors.magenta     = getValueStr("config.magenta", "\033[1;35m");
    this->colors.cyan        = getValueStr("config.cyan",    "\033[1;36m");
    this->colors.white       = getValueStr("config.white",   "\033[1;37m");

    this->colors.gui_black   = getValueStr("gui.black",   "!#000005");
    this->colors.gui_red     = getValueStr("gui.red",     "!#ff2000");
    this->colors.gui_green   = getValueStr("gui.green",   "!#00ff00");
    this->colors.gui_blue    = getValueStr("gui.blue",    "!#00aaff");
    this->colors.gui_cyan    = getValueStr("gui.cyan",    "!#00ffff");
    this->colors.gui_yellow  = getValueStr("gui.yellow",  "!#ffff00");
    this->colors.gui_magenta = getValueStr("gui.magenta", "!#ff11cc");
    this->colors.gui_white   = getValueStr("gui.white",   "!#ffffff");

    if (this->percentage_colors.size() < 3)
    {
        warn(_("the config array percentage-colors doesn't have 3 colors for being used in percentage tag and modules.\n"
               "Backing up to green, yellow and red"));
        this->percentage_colors = {"green", "yellow", "red"};
    }

    for (const std::string& str : this->getValueArrayStr("config.alias-colors", {}))
        this->addAliasColors(str);

    const char *no_color = std::getenv("NO_COLOR");
    if (no_color != NULL && no_color[0] != '\0')
        this->args_disable_colors = true;
}

void Config::addAliasColors(const std::string& str)
{
    const size_t pos = str.find('=');
    if (pos == std::string::npos)
        die(_("alias color '{}' does NOT have an equal sign '=' for separating color name and value\n"
            "For more check with --help"), str);

    const std::string& name  = str.substr(0, pos);
    const std::string& value = str.substr(pos + 1);

    this->colors_name.push_back(name);
    this->colors_value.push_back(value);
}

static bool is_str_digital(const std::string& str)
{
    for (size_t i = 0; i < str.size(); ++i)
        if (!(str[i] >= '0' && str[i] <= '9'))
            return false;

    return true;
}

void Config::overrideOption(const std::string& opt)
{
    const size_t pos = opt.find('=');
    if (pos == std::string::npos)
        die(_("override option '{}' does NOT have an equal sign '=' for separating config name and value\n"
            "For more check with --help"), opt);

    std::string name {opt.substr(0, pos)};
    const std::string& value = opt.substr(pos + 1);

    // usually the user finds incovinient to write "config.foo"
    // for general config options
    if (name.find('.') == name.npos)
        name.insert(0, "config.");

    if (value == "true")
        overrides[name] = {BOOL, "", true, 0};
    else if (value == "false")
        overrides[name] = {BOOL, "", false, 0};
    else if ((value[0] == '"' && value.back() == '"') ||
             (value[0] == '\'' && value.back() == '\''))
        overrides[name] = {STR, value.substr(1, value.size()-2), false, 0};
    else if (is_str_digital(value))
        overrides[name] = {INT, "", false, std::stoi(value)};
    else
        die(_("looks like override value '{}' from '{}' is neither a bool, int or string value"), 
            value, name);
}

void Config::generateConfig(const std::filesystem::path &filename)
{
    if (std::filesystem::exists(filename))
    {
        if (!askUserYorN(false, "WARNING: config file '{}' already exists. Do you want to overwrite it?", filename.string()))
            std::exit(1);
    }

    auto f = fmt::output_file(filename.c_str());
    f.print("{}", AUTOCONFIG);
}
