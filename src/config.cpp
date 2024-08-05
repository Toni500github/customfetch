#include "config.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

// initialize Config, can only be ran once for each Config instance.
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
        std::ofstream f(configFile.data(), std::ios::trunc);
        f << AUTOCONFIG;
        f.close();
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

    // https://stackoverflow.com/a/78266628
    // changed instead of vector<int> to vector<string>
    // just a workaround for having the layout config variable as a vector<string>
    auto layout_array = tbl.at_path("config.layout");
    if (toml::array* arr = layout_array.as_array())
        arr->for_each(
            [this, filename](auto&& el)
            {
                if (const auto* str_elem = el.as_string())
                {
                    auto v = *str_elem;
                    this->layouts.push_back(v->data());  // here's the thing
                }
                else
                    warn("An element of the layout variable in {} is not a string", filename);
            });

    auto pkg_managers_array = tbl.at_path("config.pkg-managers");
    if (toml::array* arr = pkg_managers_array.as_array())
        arr->for_each(
            [this, filename](auto&& element)
            {
                if (const auto* str_element = element.as_string())
                {
                    auto element_value = *str_element;
                    this->pkgs_managers.push_back(str_tolower(element_value->data()));
                }
                else
                    warn("An element of the pkg-managers variable in {} is not a string", filename);
            });

    // clang-format off
    this->source_path      = this->getConfigValue<std::string>("config.source-path", "os");
    this->data_dir         = this->getConfigValue<std::string>("config.data-dir", "/usr/share/customfetch");
    this->sep_reset        = this->getConfigValue<std::string>("config.sep-reset", ":");
    this->offset           = this->getConfigValue<std::uint8_t>("config.offset", 5);
    this->gui              = this->getConfigValue<bool>("gui.enable", false);
    this->font             = this->getConfigValue<std::string>("gui.font", "Liberation Mono Normal 12");
    this->logo_padding_top = this->getConfigValue<std::uint8_t>("config.logo-padding-top", 0);

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

    // clang-format on
}

std::string Config::getThemeValue(const std::string& value, const std::string& fallback) const
{
    return this->tbl.at_path(value).value<std::string>().value_or(fallback);
}
