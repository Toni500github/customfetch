#ifndef _MANIFEST_HPP_
#define _MANIFEST_HPP_

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "libcufetch/common.hh"
#include "toml++/toml.hpp"

struct manifest_t
{
    std::string name;
    std::string license;
    std::string description;
    std::string output_dir;
    std::vector<std::string> authors;
    std::vector<std::string> build_steps;
};

const char* const MANIFEST_NAME = "cufetchpm.toml";

class CManifest {
public:
    CManifest(const std::string_view path);
    CManifest(toml::table&& tbl) : m_tbl(tbl){}
    CManifest(const toml::table& tbl) : m_tbl(std::move(tbl)){}
    CManifest &operator=(CManifest &&) = default;
    CManifest &operator=(const CManifest &) = default;
    ~CManifest() = default;

    manifest_t get_plugin(const std::string_view name);
    std::vector<manifest_t> get_all_plugins();

private:
    toml::table m_tbl;
    bool m_is_state = true;

    template <typename T>
    T getValue(const std::string_view name, const std::string_view value) const
    {
        const std::optional<T>& ret = m_tbl[name][value].value<T>();
        return ret.value_or(UNKNOWN);
    }

    std::vector<std::string> getValueArrayStr(const std::string_view name, const std::string_view value) const
    {
        std::vector<std::string> ret;

        // https://stackoverflow.com/a/78266628
        if (const toml::array* array_it = m_tbl[name][value].as_array())
        {
            array_it->for_each([&ret](auto&& el) {
                if (const toml::value<std::string>* str_elem = el.as_string())
                    ret.push_back((*str_elem)->data());
            });

            return ret;
        }
        return {};
    }
};

#endif // !_MANIFEST_HPP_;
