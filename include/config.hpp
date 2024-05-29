#ifndef _CONFIG_HPP
#define _CONFIG_HPP

#define TOML_HEADER_ONLY 0
#define TOML_ENABLE_FORMATTERS 0

#include "util.hpp"
#include "fmt/color.h"
#include "toml++/toml.hpp"

enum types {
    STR,
    BOOL
};

struct strOrBool {
    types  valueType;
    std::string stringValue = "";
    bool   boolValue   = false;
};

struct _color_t {
    fmt::rgb c1;
    fmt::rgb c2;
};

class Config {
public:
    bool initialized;
    std::vector<std::string> layouts;
    std::map<std::string, strOrBool> overrides;


    // initialize Config, can only be ran once for each Config instance.
    void init(std::string& configFile, std::string& configDir);
    void loadConfigFile(std::string_view filename);
    fmt::rgb getThemeValue(const std::string& value, const std::string& fallback);

    // stupid c++ that wants template functions in header
    template <typename T>
    T getConfigValue(const std::string& value, T fallback) {
        auto overridePos = overrides.find(value);

        // user wants a bool (overridable), we found an override matching the name, and the override is a bool.
        if constexpr (std::is_same<T, bool>())
            if (overridePos != overrides.end() && overrides[value].valueType == BOOL)
                return overrides[value].boolValue;

        // user wants a str (overridable), we found an override matching the name, and the override is a str.
        if constexpr (std::is_same<T, std::string>())
            if (overridePos != overrides.end() && overrides[value].valueType == STR)
                return overrides[value].stringValue;

        std::optional<T> ret = this->tbl.at_path(value).value<T>();
        if constexpr (toml::is_string<T>) // if we want to get a value that's a string
            return ret ? expandVar(ret.value()) : expandVar(fallback);
        else
            return ret.value_or(fallback);
    }

private:
    toml::table tbl;
};

inline Config config;
inline struct _color_t color;

inline std::string configFile;

inline const constexpr std::string_view AUTOCONFIG = R"#([config]
layout = [
    "=================",
    "CPU: $<cpu.name>  ",
    "================="
    ]

c1 = "#fffff"
c2 = "#ff000"
)#";

#endif
