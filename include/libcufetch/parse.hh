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

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "libcufetch/common.hh"
#include "libcufetch/config.hh"

// C ABI is needed to prevent symbol mangling, but we don't actually need C compatibility,
// so we ignore this warning about return types that are potentially incompatible with C.
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
#endif

struct module_t;

// Map from a modules name to its pointer.
using moduleMap_t = std::unordered_map<std::string, const module_t&>;

/* Context struct used when parsing tags in strings.
 * @param modules_info The modules fetched infos
 * @param config The config instance
 * @param pure_output The output of the string but without tags
 * @param layout The layout array of customfetch
 * @param tmp_layout A temponary layout to be used for multiple-line modules
 * @param parsing_layout Are we parsing the layout or the ASCII art logo?
 */
struct EXPORT parse_args_t
{
    const moduleMap_t&        modules_info;
    const ConfigBase&         config;
    std::string&              pure_output;
    std::vector<std::string>& layout;
    std::vector<std::string>& tmp_layout;
    bool                      parsing_layout;
    bool                      no_more_reset = false;
    bool                      firstrun_clr  = true; // don't use it. Internal "flag"
};

/* Parse input, in-place, with data from modules_info.
 * Documentation on formatting is in the flag -w or the customfetch.1 manual.
 * @param input The string to parse
 * @param parse_args The parse arguments to be used (parse_args_t)
 */
APICALL EXPORT std::string parse(std::string input, parse_args_t& parse_args);

/*
 * Create a colored percentage from parse()
 * @param n1 The first number
 * @param n2 The second number
 * @param parse_args The parse() parameters
 * @param invert Is the result high number bad or good?
 * @return The colored percentage with ending %
 */
APICALL EXPORT std::string get_and_color_percentage(const float n1, const float n2, parse_args_t& parse_args,
                                                    const bool invert = false);
