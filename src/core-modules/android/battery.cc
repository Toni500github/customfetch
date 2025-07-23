#include "platform.hpp"
#if CF_ANDROID

#include <string>
#include <string_view>
#include <vector>

#include "core-modules.hh"
#include "json.h"
#include "libcufetch/common.hh"
#include "libcufetch/fmt/format.h"
#include "switch_fnv1a.hpp"
#include "util.hpp"

static json::jobject            doc;
static std::vector<std::string> dumpsys;

static bool assert_doc()
{
    if (doc.size() <= 0)
    {
        std::string result;
        if (!read_exec({ "/data/data/com.termux/files/usr/libexec/termux-api", "BatteryStatus" }, result))
            return false;
        if (!json::jobject::tryparse(result.c_str(), doc))
            return false;
    }
    return true;
}

static bool assert_dumpsys()
{
    if (dumpsys.size() <= 0)
    {
        std::string result;
        if (!read_exec({ "/system/bin/dumpsys", "battery" }, result))
            return false;
        dumpsys = split(result, '\n');
        if (dumpsys[0] != "Current Battery Service state:")
            return false;
    }
    return true;
}

static std::string read_value_dumpsys(const std::string_view name, const bool is_status = false)
{
    if (!assert_dumpsys())
        return MAGIC_LINE;

    for (size_t i = 1; i < dumpsys.size(); ++i)
    {
        const size_t       pos   = dumpsys.at(i).rfind(':');
        const std::string& key   = dumpsys.at(i).substr(2, pos);
        const std::string& value = dumpsys.at(i).substr(pos + 2);

        if (is_status)
        {
            if (key.find("powered") != key.npos && value == "true")
                return key;
        }

        else if (key == name)
            return value;
    }

    return MAGIC_LINE;
}

// clang-format off
MODFUNC(battery_modelname)
{ return MAGIC_LINE; }

MODFUNC(battery_vendor)
{ return MAGIC_LINE; }

MODFUNC(battery_capacity_level)
{ return MAGIC_LINE; }

// clang-format on
MODFUNC(battery_status)
{
    if (!assert_doc())
        return read_value_dumpsys("powered", true);

    std::string charge_status{ str_tolower(doc["status"].as_string()) };
    charge_status.at(0) = toupper(charge_status.at(0));
    switch (fnv1a16::hash(doc["plugged"].as_string()))
    {
        case "PLUGGED_AC"_fnv1a16:       return "AC Connected, " + charge_status;
        case "PLUGGED_USB"_fnv1a16:      return "USB Connected, " + charge_status;
        case "PLUGGED_WIRELESS"_fnv1a16: return "Wireless Connected, " + charge_status;
        default:                         return charge_status;
    }
}

MODFUNC(battery_technology)
{
    if (!assert_doc())
        return read_value_dumpsys("technology");
    return MAGIC_LINE;
}

MODFUNC(battery_perc)
{
    if (!assert_doc())
    {
        double level = std::stod(read_value_dumpsys("level"));
        double scale = std::stod(read_value_dumpsys("scale"));
        if (level > 0 && scale > 0)
            return fmt::to_string(level * 100 / scale);
    }

    return doc["percentage"];
}

double battery_temp()
{
    if (!assert_doc())
        return std::stod(read_value_dumpsys("temperature")) / 10;
    return doc["temperature"];
}

#endif
