#include "config.hpp"
#include <fstream>
#include <iostream>
#include <sys/types.h>

// initialize Config, can only be ran once for each Config instance.
void Config::init(std::string& configFile, std::string& configDir) {
     if (this->initialized)
        return;
    
    if (!std::filesystem::exists(configDir)) {
        fmt::println("customfetch config folder was not found, Creating folders at {}!", configDir);
        std::filesystem::create_directories(configDir);
    }

    if (!std::filesystem::exists(configFile)) {
        fmt::println("{} not found, generating new one", configFile);
        std::ofstream f(configFile, std::ios::trunc);
        f << AUTOCONFIG;
        f.close();
    }

    this->loadConfigFile(configFile);
    this->initialized = true;

}

void Config::loadConfigFile(std::string_view filename) {
    try {
        this->tbl = toml::parse_file(filename);
    } catch (const toml::parse_error& err) {
        error("Parsing config file {} failed:", filename);
        std::cerr << err << std::endl;
        exit(-1);
    }
    
    // https://stackoverflow.com/a/78266628
    // changed instead of vector<int> to vector<string>
    // just a workaround for having the layout config variable as a vector<string>
    auto layout_array = tbl["config"]["layout"];
    if (toml::array* arr = layout_array.as_array())
        arr->for_each([this,filename](auto&& el)
        {
            if (const auto* str_elem = el.as_string()) {
                auto v = *str_elem;
                this->layouts.push_back(v->data()); // here's the thing
            }
            else 
                warn("An element of the layout variable in {} is not a string", filename);
        });
    
    auto includes_array = tbl["config"]["includes"];
    if (toml::array *arr = includes_array.as_array())
        arr->for_each([this,filename](auto&& element)
        {
            if (const auto *str_element = element.as_string()) {
                auto element_value = *str_element;
                this->includes.push_back(element_value->data());
            }
            else
                warn("An element of the includes variable in {} is not a string", filename);
        });

    this->source_path = getConfigValue<std::string>("config.ascii-art-path", "");
    this->offset         = getConfigValue<u_short>("config.offset", 5);
    this->gui            = getConfigValue<bool>("config.gui", true);
    

    color.red           = this->getThemeValue("red",     "#ff2000");
    color.green         = this->getThemeValue("green",   "#00ff00");
    color.blue          = this->getThemeValue("blue",    "#00aaff");
    color.cyan          = this->getThemeValue("cyan",    "#00ffff");
    color.yellow        = this->getThemeValue("yellow",  "#ffff00");
    color.magenta       = this->getThemeValue("magenta", "#ff11cc");
}

std::string Config::getThemeValue(const std::string& value, const std::string& fallback) {
    return this->tbl["config"][value].value<std::string>().value_or(fallback);
}
