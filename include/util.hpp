#ifndef UTILS_HPP
#define UTILS_HPP

#include "fmt/color.h"
#include "fmt/core.h"

#include <filesystem>
#include <vector>
#include <fstream>
#include <string>
#include <sys/types.h>

#define BOLD_TEXT(x)   (fmt::emphasis::bold | fmt::fg(x))
#define NOCOLOR        "\033[0m"
#define UNKNOWN         "(unknown)"

bool hasEnding(std::string_view fullString, std::string_view ending);
bool hasStart(std::string_view fullString, std::string_view start);
std::string name_from_entry(size_t dev_entry_pos);
std::string vendor_from_entry(size_t vendor_entry_pos, std::string_view vendor_id);
std::string binarySearchPCIArray(std::string_view vendor_id, std::string_view pci_id);
std::string binarySearchPCIArray(std::string_view vendor_id);
std::string shell_exec(std::string_view cmd);
std::vector<std::string> split(std::string_view text, char delim);
std::string expandVar(const std::string_view str);
// Replace string inplace
void replace_str(std::string &str, const std::string& from, const std::string& to);
std::string str_tolower(std::string str);
std::string str_toupper(std::string str);
void strip(std::string& input);
std::string read_by_syspath(const std::string_view path);
fmt::rgb hexStringToColor(std::string_view hexstr);
std::string getHomeConfigDir();
std::string getConfigDir();

// must be used with std::array
template <typename T, typename E>
void init_array(T&& array, E&& elements) {
    for (size_t i = 0; i < array.size(); i++) {
        array.at(i) = elements;
    }
}

template <typename... Args>
void _error_log(fmt::runtime_format_string<> fmt, Args&&... args) {
    fmt::println(stderr, BOLD_TEXT(fmt::rgb(fmt::color::red)), "ERROR: {}", fmt::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void error(std::string_view fmt, Args&&... args) {
    _error_log(fmt::runtime(fmt), std::forward<Args>(args)...);
}

template <typename... Args>
void die(std::string_view fmt, Args&&... args) {
    _error_log(fmt::runtime(fmt), std::forward<Args>(args)...);
    std::exit(1);
}

template <typename... Args>
void debug(std::string_view fmt, Args&&... args) {
#if defined(DEBUG) || DEBUG
    fmt::println(BOLD_TEXT(fmt::rgb(fmt::color::hot_pink)), "[DEBUG]: {}", fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
#endif
}

template <typename... Args>
void warn(std::string_view fmt, Args&&... args) {
    fmt::println(BOLD_TEXT(fmt::rgb(fmt::color::yellow)), "WARNING: {}", fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

#endif
