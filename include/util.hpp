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

bool hasEnding(const std::string &fullString, const std::string &ending);
bool hasStart(const std::string &fullString, const std::string &start);
std::string name_from_entry(size_t dev_entry_pos);
std::string vendor_from_entry(size_t vendor_entry_pos, const std::string &vendor_id);
std::string binarySearchPCIArray(const std::string &vendor_id, const std::string &pci_id);
std::string binarySearchPCIArray(const std::string &vendor_id);
std::string shell_exec(const std::string &cmd);
std::vector<std::string> split(const std::string &text, char delim);
std::string expandVar(std::string &str);
// Replace string inplace
void replace_str(std::string &str, const std::string& from, const std::string& to);
std::string str_tolower(std::string str);
std::string str_toupper(std::string str);
void strip(std::string& input);
std::string read_by_syspath(const std::string &path);
fmt::rgb hexStringToColor(const std::string &hexstr);
std::string getHomeConfigDir();
std::string getConfigDir();

template <typename... Args>
constexpr void _error_log(fmt::runtime_format_string<> fmt, Args&&... args) {
    fmt::println(stderr, BOLD_TEXT(fmt::rgb(fmt::color::red)), "ERROR: {}", fmt::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
constexpr void error(const std::string &fmt, Args&&... args) {
    _error_log(fmt::runtime(fmt), std::forward<Args>(args)...);
}

template <typename... Args>
constexpr void die(const std::string &fmt, Args&&... args) {
    _error_log(fmt::runtime(fmt), std::forward<Args>(args)...);
    std::exit(1);
}

template <typename... Args>
constexpr void debug(const std::string &fmt, Args&&... args) {
#if defined(DEBUG) || DEBUG
    fmt::println(BOLD_TEXT(fmt::rgb(fmt::color::hot_pink)), "[DEBUG]: {}", fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
#endif
}

template <typename... Args>
constexpr void warn(const std::string &fmt, Args&&... args) {
    fmt::println(BOLD_TEXT(fmt::rgb(fmt::color::yellow)), "WARNING: {}", fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

#endif
