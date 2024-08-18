#ifndef _PARSE_HPP
#define _PARSE_HPP

#include "config.hpp"
#include "query.hpp"

// Parse input, in-place, with data from systemInfo.
// Documentation on formatting is in the default config.toml file.
// pureOutput is set to the string, but without the brackets.
std::string parse(const std::string_view input, systemInfo_t& systemInfo, std::string& pureOutput, const Config& config,
                  colors_t& colors, bool parsingLaoyut);

// Set module values to a systemInfo_t map.
// If the name of said module matches any module name, it will be added
// else, error out.
void addValueFromModule(systemInfo_t& sysInfo, const std::string& moduleName, const std::string& moduleValueName,
                        const Config& config);

std::string getInfoFromName(const systemInfo_t& systemInfo, const std::string_view moduleName,
                            const std::string_view moduleValueName);
#endif
