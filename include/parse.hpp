/*
 * Copyright 2025 Toni500git
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 * following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote
 * products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _PARSE_HPP
#define _PARSE_HPP

#include <string>
#include <vector>

#include "config.hpp"
#include "query.hpp"

/* The additional args that parse() needs for getting the necessary infos/configs.
 * Only used for making the argument passing more clear.
 * Always pass it non-const and by reference
 */
struct parse_args_t
{
    moduleMap_t&              modulesInfo;
    std::string&              pureOutput;
    std::vector<std::string>& layout;
    std::vector<std::string>& tmp_layout;
    const Config&             config;
    const colors_t&           colors;
    bool                      parsingLayout;
    bool                      firstrun_clr  = true;
    bool                      no_more_reset = false;
};

/* Parse input, in-place, with data from modulesInfo.
 * Documentation on formatting is in the flag -w or the customfetch.1 manual.
 * @param input The string to parse
 * @param modulesInfo The system infos
 * @param pureOutput The output of the string but without tags
 * @param layout The layout of customfetch
 * @param tmp_layout The temponary layout to be used for $<auto> modules
 * @param config The config
 * @param colors The colors
 * @param parsingLayout If we are parsing layout or not
 * @param no_more_reset If we are recursively parsing, e.g we are inside tags
 */
std::string parse(std::string input, moduleMap_t& modulesInfo, std::string& pureOutput, std::vector<std::string>& layout,
                  std::vector<std::string>& tmp_layout, const Config& config, const colors_t& colors,
                  const bool parsingLayout, bool& no_more_reset);

// parse() for parse_args_t& arguments
std::string parse(const std::string& input, parse_args_t& parse_args);
// some times we don't want to use the original pureOutput,
// so we have to create a tmp string just for the sake of the function arguments
std::string parse(const std::string& input, std::string& _, parse_args_t& parse_args);

/* Set module members values to a modulesInfo_t map.
 * If the name of said module matches any module name, it will be added
 * else, error out.
 * @param moduleName The module name
 * @param moduleMemberName The module member name
 * @param parse_args The parse() like arguments
 */
// void addValueFromModuleMember(const std::string& moduleName, const std::string& moduleMemberName,
//                               parse_args_t& parse_args);

/* Set module only values to a modulesInfo_t map.
 * If the name of said module matches any module name, it will be added
 * else, error out.
 * @param moduleName The module name
 * @param parse_args The parse() like arguments
 */
// void addValueFromModule(const std::string& moduleName, parse_args_t& parse_args);

/*
 * Return an info module member value
 * @param modulesInfo The modulesInfo_t map
 * @param moduleName The module name
 * @param moduleMemberName The module member name
 */
const std::string getInfoFromName(const moduleMap_t& modulesInfo, const std::string_view moduleName,
                                  const std::string_view moduleMemberName);

/*
 * Create a colored percentage from parse()
 * @param n1 The first number
 * @param n2 The second number
 * @param parse_args The parse() parameters
 * @param invert Is the result high number bad or good?
 * @return The colored percentage with ending %
 */
std::string get_and_color_percentage(const float n1, const float n2, parse_args_t& parse_args,
                                     const bool invert = false);

template <typename... Styles>
void append_styles(fmt::text_style& current_style, Styles&&... styles)
{
    current_style |= (styles | ...);
}

#endif
