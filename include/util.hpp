#ifndef UTILS_HPP
#define UTILS_HPP

#include "fmt/color.h"
#include "fmt/core.h"

#include <filesystem>
#include <vector>
#include <string>
#include <sys/types.h>
#include <unordered_map>
#include <variant>

#define systemInfo_t std::unordered_map<std::string, std::unordered_map<std::string, std::variant<std::string, size_t>>>
#define VARIANT std::variant<std::string, size_t>

using std::filesystem::path;

#define BOLD           fmt::emphasis::bold
#define BOLD_TEXT(x)   (fmt::emphasis::bold | fmt::fg(x))
#define NOCOLOR        "\033[0m"
#define UNKNOWN         "<unknown>"

std::string name_from_entry(size_t dev_entry_pos);
std::string vendor_from_entry(size_t vendor_entry_pos, std::string_view vendor_id);
std::string binarySearchPCIArray(std::string_view vendor_id, std::string_view pci_id);
std::string binarySearchPCIArray(std::string_view vendor_id);
std::string shell_exec(std::string_view cmd);
std::vector<std::string> split(std::string_view text, char delim);
std::string expandVar(std::string& str);
void strip(std::string& input);

// Parse input, in-place, with data from systemInfo.
// Documentation on formatting is in the default config.toml file.
// pureOutput is set to the string, but without the brackets.
std::string parse(std::string& input, systemInfo_t &systemInfo, std::unique_ptr<std::string> &pureOutput);

fmt::rgb hexStringToColor(std::string_view hexstr);
std::string getHomeConfigDir();
std::string getConfigDir();

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
void die(const char *fmt, Args&&... args) {
    _error_log(fmt::runtime(fmt), std::forward<Args>(args)...);
    std::exit(1);
}

template <typename... Args>
void debug(std::string_view fmt, Args&&... args) {
    fmt::println(BOLD_TEXT(fmt::rgb(fmt::color::hot_pink)), "[DEBUG]: {}", fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

template <typename... Args>
void warn(std::string_view fmt, Args&&... args) {
    fmt::println(BOLD_TEXT(fmt::rgb(fmt::color::yellow)), "WARNING: {}", fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

#endif
