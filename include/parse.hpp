#ifndef _PARSE_HPP
#define _PARSE_HPP

#include "config.hpp"
#include "query.hpp"

/* Parse input, in-place, with data from systemInfo.
 * Documentation on formatting is in the default config.toml file or the cufetch.1 manual.
 * @param input The string to parse
 * @param systemInfo The system infos
 * @param pureOutput The output of the string but without tags
 * @param config The config
 * @param colors The colors
 * @param parsingLayout If we are parsing layout or not
 * @param is_image If the source path is an image (used for GUI mode only)
 */ 
std::string parse(const std::string_view input, systemInfo_t& systemInfo, std::string& pureOutput, const Config& config,
                  const colors_t& colors, const bool parsingLayout, const bool is_image = false);

/* Set module members values to a systemInfo_t map.
 * If the name of said module matches any module name, it will be added
 * else, error out.
 * @param sysInfo The systemInfo_t map
 * @param moduleName The module name
 * @param moduleMemberName The module member name
 * @param config The config
 * @param colors The colors
 * @param parsingLayout If we are parsing the layout or not (default true)
 */
void addValueFromModule(systemInfo_t& sysInfo, const std::string& moduleName, const std::string& moduleMemberName,
                        const Config& config, const colors_t& colors, bool parsingLayout = true);

/*
 * Return a module member value
 */
std::string getInfoFromName(const systemInfo_t& systemInfo, const std::string_view moduleName,
                            const std::string_view moduleMemberName);

// Function to combine multiple fmt::text_style arguments
template <typename... Styles>
void append_styles(fmt::text_style& current_style, Styles&&... styles)
{
    current_style = current_style | (styles | ...);
}

#endif
