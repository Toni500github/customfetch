#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "libcufetch/common.hh"
#include "libcufetch/config.hh"

struct module_t;

// Map from a modules name to its pointer.
using moduleMap_t = std::unordered_map<std::string, const module_t&>;

/* The additional args that parse() needs for getting the necessary infos/configs.
 * Only used for making the argument passing more clear.
 * Always pass it non-const and by reference
 */
struct EXPORT parse_args_t
{
    const moduleMap_t&        modulesInfo;
    std::string&              pureOutput;
    std::vector<std::string>& layout;
    std::vector<std::string>& tmp_layout;
    const ConfigBase&         config;
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
 * @param tmp_layout The temponary layout to be used for multiple-line modules
 * @param config The config
 * @param colors The colors
 * @param parsingLayout If we are parsing layout or not
 * @param no_more_reset If we are recursively parsing, e.g we are inside tags
 */
std::string parse(std::string input, const moduleMap_t& modulesInfo, std::string& pureOutput,
                  std::vector<std::string>& layout, std::vector<std::string>& tmp_layout, const ConfigBase& config,
                  const bool parsingLayout, bool& no_more_reset);

// parse() for parse_args_t& arguments
APICALL EXPORT std::string parse(const std::string& input, parse_args_t& parse_args);

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
