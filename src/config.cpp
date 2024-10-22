#include "config.hpp"

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
    this->layout             = this->getValueArrayStr("config.layout", {});
    this->percentage_colors  = this->getValueArrayStr("config.percentage-colors", {"green", "yellow", "red"});
    this->gui                = this->getValue<bool>("gui.enable", false);
    this->slow_query_warnings= this->getValue<bool>("config.slow-query-warnings", false);
    this->sep_reset_after    = this->getValue<bool>("config.sep-reset-after", false);
    this->use_SI_unit        = this->getValue<bool>("config.use-SI-byte-unit", false);
    this->wrap_lines         = this->getValue<bool>("config.wrap-lines", true);
    this->ascii_logo_type    = this->getValue<std::string>("config.ascii-logo-type", "");
    this->source_path        = this->getValue<std::string>("config.source-path", "os");
    this->data_dir           = this->getValue<std::string>("config.data-dir", "/usr/share/customfetch");
    this->sep_reset          = this->getValue<std::string>("config.sep-reset", ":");
    this->offset             = this->getValue<std::uint16_t>("config.offset", 5);
    this->logo_padding_left  = this->getValue<std::uint16_t>("config.logo-padding-left", 0);
    this->layout_padding_top = this->getValue<std::uint16_t>("config.layout-padding-top", 0);
    this->logo_padding_top   = this->getValue<std::uint16_t>("config.logo-padding-top", 0);
    this->font               = this->getValue<std::string>("gui.font", "Liberation Mono Normal 12");
    this->gui_bg_image       = this->getValue<std::string>("gui.bg-image", "disable");

    this->builtin_title_sep  = this->getValue<std::string>("config.title-sep", "-");

    this->uptime_d_fmt = this->getValue<std::string>("os.uptime.days", " days");
    this->uptime_h_fmt = this->getValue<std::string>("os.uptime.hours", " hours");
    this->uptime_m_fmt = this->getValue<std::string>("os.uptime.mins", " mins");
    this->uptime_s_fmt = this->getValue<std::string>("os.uptime.secs", " secs");

    this->pkgs_managers= this->getValueArrayStr("os.pkgs.pkg-managers", {});
    this->pacman_dirs  = this->getValueArrayStr("os.pkgs.pacman-dirs",  {"/var/lib/pacman/local"});
    this->dpkg_files   = this->getValueArrayStr("os.pkgs.dpkg-files",   {"/var/lib/dpkg/status"});
    this->flatpak_dirs = this->getValueArrayStr("os.pkgs.flatpak-dirs", {"/var/lib/flatpak/app", "~/.local/share/flatpak/app"});
    this->apk_files    = this->getValueArrayStr("os.pkgs.apk-files",    {"/var/lib/apk/db/installed"});

    colors.black       = this->getThemeValue("config.black",   "\033[1;30m");
    colors.red         = this->getThemeValue("config.red",     "\033[1;31m");
    colors.green       = this->getThemeValue("config.green",   "\033[1;32m");
    colors.yellow      = this->getThemeValue("config.yellow",  "\033[1;33m");
    colors.blue        = this->getThemeValue("config.blue",    "\033[1;34m");
    colors.magenta     = this->getThemeValue("config.magenta", "\033[1;35m");
    colors.cyan        = this->getThemeValue("config.cyan",    "\033[1;36m");
    colors.white       = this->getThemeValue("config.white",   "\033[1;37m");

    colors.gui_black   = this->getThemeValue("gui.black",   "!#000005");
    colors.gui_red     = this->getThemeValue("gui.red",     "!#ff2000");
    colors.gui_green   = this->getThemeValue("gui.green",   "!#00ff00");
    colors.gui_blue    = this->getThemeValue("gui.blue",    "!#00aaff");
    colors.gui_cyan    = this->getThemeValue("gui.cyan",    "!#00ffff");
    colors.gui_yellow  = this->getThemeValue("gui.yellow",  "!#ffff00");
    colors.gui_magenta = this->getThemeValue("gui.magenta", "!#ff11cc");
    colors.gui_white   = this->getThemeValue("gui.white",   "!#ffffff");

    if (this->percentage_colors.size() < 3)
    {
        warn("the config array percentage-colors doesn't have 3 colors for being used in percentage tag and modules\n"
             "backing up to green, yellow and red");
        this->percentage_colors = {"green", "yellow", "red"};
    }

    for (const std::string& str : this->getValueArrayStr("config.alias-colors", {}))
        this->addAliasColors(str);
}

// Config::getValue() but don't want to specify the template
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
        die("alias color '{}' does NOT have an equal sign '=' for separiting color name and value.\n"
            "for more check with --help", str);

    const std::string& name  = str.substr(0, pos);
    const std::string& value = str.substr(pos + 1);

    this->colors_name.push_back(name);
    this->colors_value.push_back(value);
}

void Config::generateConfig(const std::string_view filename)
{
    if (std::filesystem::exists(filename))
    {
        if (!askUserYorN(false, "WARNING: config file {} already exists. Do you want to overwrite it?", filename))
            std::exit(1);
    }

    std::ofstream f(filename.data(), std::ios::trunc);
    f << AUTOCONFIG;
    f.close();
}
