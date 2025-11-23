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

#define TOML_HEADER_ONLY 0
#include "libcufetch/common.hh"
#include "toml++/toml.hpp"

enum types
{
    STR,
    BOOL,
    INT
};

struct override_configs_types
{
    types       value_type;
    std::string string_value = "";
    bool        bool_value   = false;
    int         int_value    = 0;
};

class EXPORT ConfigBase
{
public:

    /**
     * Get array string value of a config variable
     * @param value The config variable "path" (e.g "config.layout")
     * @param fallback Default value if couldn't retrive value
     */
    std::vector<std::string> getValueArrayStr(const std::string_view          value,
                                              const std::vector<std::string>& fallback) const
    {
        std::vector<std::string> ret;

        // https://stackoverflow.com/a/78266628
        if (const toml::array* array_it = tbl.at_path(value).as_array())
        {
            array_it->for_each([&ret](auto&& el) {
                if (const toml::value<std::string>* str_elem = el.as_string())
                    ret.push_back((*str_elem)->data());
            });

            return ret;
        }
        else
        {
            return fallback;
        }
    }

    /**
     * Get string value of a config variables
     * @param value The config variable "path" (e.g "config.data-dir")
     * @param fallback Default value if couldn't retrive value
     */
    std::string getValueStr(const std::string_view value, const std::string& fallback) const
    {
        return getValue<std::string>(value, fallback);
    }

    /**
     * Get integer value of a config variables
     * @param value The config variable "path" (e.g "config.offset")
     * @param fallback Default value if couldn't retrive value
     */
    int getValueInt(const std::string_view value, const int& fallback) const
    {
        return getValue<int>(value, fallback);
    }

    /**
     * Get boolean value of a config variables
     * @param value The config variable "path" (e.g "config.wrap-lines")
     * @param fallback Default value if couldn't retrive value
     */
    bool getValueBool(const std::string_view value, const bool fallback) const
    {
        return getValue<bool>(value, fallback);
    }

private:
    /**
     * Get value of a config variables
     * @param value The config variable "path" (e.g "config.source-path")
     * @param fallback Default value if couldn't retrive value
     */
    template <typename T>
    T getValue(const std::string_view value, const T& fallback) const
    {
        const auto& overridePos = overrides.find(value.data());

        // user wants a bool (overridable), we found an override matching the name, and the override is a bool.
        if constexpr (std::is_convertible_v<T, bool>)
            if (overridePos != overrides.end() && overrides.at(value.data()).value_type == BOOL)
                return overrides.at(value.data()).bool_value;

        if constexpr (std::is_convertible_v<T, std::string>)
            if (overridePos != overrides.end() && overrides.at(value.data()).value_type == STR)
                return overrides.at(value.data()).string_value;

        if constexpr (std::is_convertible_v<T, int>)
            if (overridePos != overrides.end() && overrides.at(value.data()).value_type == INT)
                return overrides.at(value.data()).int_value;

        const std::optional<T>& ret = this->tbl.at_path(value).value<T>();
        return ret.value_or(fallback);
    }

protected:
    std::unordered_map<std::string, override_configs_types> overrides;

    // Parsed config from loadConfigFile()
    toml::table tbl;

};
