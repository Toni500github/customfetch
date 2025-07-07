#pragma once

#include <string>
#include <unordered_map>

#define TOML_HEADER_ONLY 0
#include "toml++/toml.hpp"

#include "cufetch/common.hh"

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
     * Get value of config variables
     * @param value The config variable "path" (e.g "config.source-path")
     * @param fallback Default value if couldn't retrive value
     */
    template <typename T>
    T getValue(const std::string_view value, const T&& fallback) const
    {
        const auto& overridePos = overrides.find(value.data());

        // user wants a bool (overridable), we found an override matching the name, and the override is a bool.
        if constexpr (std::is_convertible<T, bool>())
            if (overridePos != overrides.end() && overrides.at(value.data()).value_type == BOOL)
                return overrides.at(value.data()).bool_value;

        if constexpr (std::is_convertible<T, std::string>())
            if (overridePos != overrides.end() && overrides.at(value.data()).value_type == STR)
                return overrides.at(value.data()).string_value;

        if constexpr (std::is_convertible<T, int>())
            if (overridePos != overrides.end() && overrides.at(value.data()).value_type == INT)
                return overrides.at(value.data()).int_value;

        const std::optional<T>& ret = this->tbl.at_path(value).value<T>();
        return ret.value_or(fallback);
    }

    /**
     * getValue() but don't want to specify the template, so it's std::string,
     * and thus the name, only used when retriving the colors for terminal and GUI
     * @param value The config variable "path" (e.g "config.gui-red")
     * @param fallback Default value if couldn't retrive value
     */
    std::string getThemeValue(const std::string_view value, const std::string_view fallback) const
    {
        return this->tbl.at_path(value).value<std::string>().value_or(fallback.data());
    }

    /**
     * Get value of config array of string variables
     * @param value The config variable "path" (e.g "config.gui-red")
     * @param fallback Default value if couldn't retrive value
     */
    std::vector<std::string> getValueArrayStr(const std::string_view value, const std::vector<std::string>& fallback) const
    {
        std::vector<std::string> ret;

        // https://stackoverflow.com/a/78266628
        if (const toml::array* array_it = tbl.at_path(value).as_array())
        {
            array_it->for_each(
                [&ret](auto&& el)
                {
                    if (const toml::value<std::string>* str_elem = el.as_string())
                        ret.push_back((*str_elem)->data());
                }
            );

            return ret;
        }
        else
        {
            return fallback;
        }
    }

protected:
    std::unordered_map<std::string, override_configs_types> overrides;

    // Parsed config from loadConfigFile()
    toml::table tbl;
};
