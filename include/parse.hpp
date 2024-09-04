#ifndef _PARSE_HPP
#define _PARSE_HPP

#include "config.hpp"
#include "query.hpp"

// Parse input, in-place, with data from systemInfo.
// Documentation on formatting is in the default config.toml file.
// pureOutput is set to the string, but without the brackets.
std::string parse(const std::string_view input, systemInfo_t& systemInfo, std::string& pureOutput, const Config& config,
                  const colors_t& colors, const bool parsingLayout, const bool is_image = false);

// Set module values to a systemInfo_t map.
// If the name of said module matches any module name, it will be added
// else, error out.
void addValueFromModule(systemInfo_t& sysInfo, const std::string& moduleName, const std::string& moduleValueName,
                        const Config& config, const colors_t& colors);

std::string getInfoFromName(const systemInfo_t& systemInfo, const std::string_view moduleName,
                            const std::string_view moduleValueName);

// Function to combine multiple fmt::text_style arguments
template <typename... Styles>
void append_styles(fmt::text_style& current_style, Styles&&... styles)
{
    current_style = current_style | (styles | ...);
}

#endif
