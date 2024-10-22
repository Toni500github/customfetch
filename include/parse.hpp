#ifndef _PARSE_HPP
#define _PARSE_HPP

#include "config.hpp"
#include "query.hpp"

/* the additional args that parse() needs for getting the necessary infos/configs.
 * only used for making the argument passing more clear.
 * Always pass it non-const and by reference
 */
struct parse_args_t
{
    systemInfo_t&   systemInfo;
    std::string&    pureOutput;
    const Config&   config;
    const colors_t& colors;
    const bool      parsingLayout;
    bool&           firstrun_clr;
};

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
                  const colors_t& colors, const bool parsingLayout);

/* Set module members values to a systemInfo_t map.
 * If the name of said module matches any module name, it will be added
 * else, error out.
 * @param moduleName The module name
 * @param moduleMemberName The module member name
 * @param parse_args The parse() like arguments
 */
void addValueFromModule(const std::string& moduleName, const std::string& moduleMemberName, parse_args_t& parse_args);

/*
 * Return a module member value
 */
std::string getInfoFromName(const systemInfo_t& systemInfo, const std::string_view moduleName,
                            const std::string_view moduleMemberName);

// Function to combine multiple fmt::text_style arguments
template <typename... Styles>
void append_styles(fmt::text_style& current_style, Styles&&... styles)
{
    current_style |= (styles | ...);
}

#endif
