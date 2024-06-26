#include <unordered_map>
#include <variant>
#include <string>
#include <memory>

#include "config.hpp"

using systemInfo_t = std::unordered_map<std::string, std::unordered_map<std::string, std::variant<std::string, size_t, float>>>;
using variant = std::variant<std::string, size_t, float>;

// Parse input, in-place, with data from systemInfo.
// Documentation on formatting is in the default config.toml file.
// pureOutput is set to the string, but without the brackets.
std::string parse(const std::string& input, systemInfo_t& systemInfo, std::string &pureOutput, Config& config, colors_t& colors);
std::string parse(const std::string& input, systemInfo_t& systemInfo, Config& config, colors_t& colors);

// Set module values to a systemInfo_t map.
// If the name of said module matches any module name, it will be added
// else, error out.
void addModuleValues(systemInfo_t& sysInfo, const std::string_view moduleName);
void addValueFromModule(systemInfo_t& sysInfo, const std::string& moduleName, const std::string& moduleValueName);
