#ifndef _CONFIG_HPP
#define _CONFIG_HPP

#define TOML_HEADER_ONLY 0

#include "util.hpp"
#include "fmt/color.h"
#include <array>
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

struct color_t {
    std::string red;
    std::string green;
    std::string blue;
    std::string cyan;
    std::string yellow;
    std::string magenta;
};

class Config {
public:
    bool        initialized;
    std::string ascii_art_path;
    bool        disable_ascii_art = false;
    u_short     offset = 0;
    bool        gui = false;
    std::vector<std::string> layouts;
    std::vector<std::string> includes;
    std::map<std::string, strOrBool> overrides;


    // initialize Config, can only be ran once for each Config instance.
    void init(std::string& configFile, std::string& configDir);
    void loadConfigFile(std::string_view filename);
    std::string getThemeValue(const std::string& value, const std::string& fallback);

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
inline struct color_t color;

inline std::string configFile;

inline const constexpr std::string_view AUTOCONFIG = R"#([config]
# check below for documentation about config
# you can use those "modules" in the ascii art text file too

# includes directive, include the top name of each module you use.
# e.g. if you want to use $<os.name>, then `includes = ["os"]`.
# you can also put specific includes, for example if you only want os.name, then `includes = ["os.name"]`
includes = ["os", "cpu", "gpu", "ram"]

layout = [
    "${red}$<os.username>${0}@${cyan}$<os.hostname>",
    "───────────────────────────",
    "${red}OS${0}: $<os.name>",
    "${cyan}Uptime${0}: $<os.uptime_hours> hours, $<os.uptime_mins> minutes",
    "${green}Kernel${0}: $<os.kernel_name> $<os.kernel_version>",
    "${yellow}Arch${0}: $<os.arch>",
    "${magenta}CPU${0}: $<cpu.name>",
    "${blue}GPU${0}: $<gpu.name>",
    "${#03ff93}RAM usage${0}: $<ram.used> MB / $<ram.total> MB",
    "",
    "${\e[40m}   ${\e[41m}   ${\e[42m}   ${\e[43m}   ${\e[44m}   ${\e[45m}   ${\e[46m}   ${\e[47m}   ", # normal colors
    "${\e[100m}   ${\e[101m}   ${\e[102m}   ${\e[103m}   ${\e[104m}   ${\e[105m}   ${\e[106m}   ${\e[107m}   " # light colors
]

# make it empty for disabling custom ascii-art displaying
# and instead display the distro logo 
ascii-art-path = "test.txt"

# offset between the ascii art and the system infos
offset = 5

# Open a window and print there instead of on the terminal.
# note: customfetch needs to be compiled with GUI_SUPPORT=1 (which is enabled by default)
gui = false

# Colors can be with: hexcodes (#55ff88) OR bash escape code colors like "\e[1;34m"
# remember to add ${0} where you want to reset color
red = "#ff2000"
green = "#00ff00"
blue = "#00aaff"
cyan = "#00ffff"
yellow = "#ffff00"
magenta = "#ff11cc"

# customfetch is designed with customizability in mind
# here is how it works:
# the variable "layout" is used for showing the infos and/or something else
# as like as the user want, no limitation.
# inside here there are 3 "modules": $<> $() ${}

# $<> means you access a sub-member of a member
# e.g $<user.name> will print the username, $<os.kernel_version> will print the kernel version and so on.
# run "cufetch -l" for a list of builti-in components

# $() let's you execute bash commands
# e.g $(echo \"hello world\") will indeed echo out Hello world.
# you can even use pipes
# e.g $(echo \"hello world\" | cut -d' ' -f2) will only print world

# ${} is used to telling which color to use for colorizing the text
# e.g "${red}hello world" will indeed print "hello world" in red (or the color you set in the variable)
# you can even put a custom hex color e.g: ${#ff6622}
# OR bash escape code colors e.g ${\e[1;32m} or ${\e[0;34m}

# Little FAQ
# Q: "but then if I want to make only some words/chars in a color and the rest normal?"
# A: there is ${0}. e.g "${red}hello ${0}world, yet again" will only print "hello" in red, and then "world, yet again" normal

)#";

#endif
