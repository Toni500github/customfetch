#include "platform.hpp"
#if CF_LINUX

#include <unistd.h>

#include <string>

#include "core-modules.hh"
#include "libcufetch/common.hh"
#include "util.hpp"

static std::string read_strip_syspath(const std::string_view path)
{
    std::string str = read_by_syspath(path);
    debug("str = {} || path = {}", str, path);

    // optimization
    if (str.back() == '\n')
        str.pop_back();
    else if (str != UNKNOWN)
        strip(str);

    return str;
}

static std::string get_battery_info(const std::string& file)
{
    if (access("/sys/class/power_supply/", F_OK) != 0)
        return MAGIC_LINE;

    for (const auto& dir_entry : std::filesystem::directory_iterator{ "/sys/class/power_supply/" })
    {
        const std::string& path = dir_entry.path().string() + "/";
        debug("battery path = {}", path);

        const std::string& tmp = read_strip_syspath(path + "type");
        if (tmp == UNKNOWN || tmp != "Battery")
            continue;
        if (read_strip_syspath(path + "scope") == "Device")
            continue;

        debug("battery found yeappyy");
        return read_strip_syspath(path + file);
    }

    return UNKNOWN;
}

MODFUNC(battery_modelname)
{ return get_battery_info("model_name"); }

MODFUNC(battery_perc)
{ return get_battery_info("capacity"); }

MODFUNC(battery_status)
{ return get_battery_info("status"); }

MODFUNC(battery_capacity_level)
{ return get_battery_info("capacity_level"); }

MODFUNC(battery_technology)
{ return get_battery_info("technology"); }

MODFUNC(battery_vendor)
{ return get_battery_info("manufacturer"); }

double battery_temp()
{
    const std::string& temp = get_battery_info("temp");
    if (temp != UNKNOWN || temp != MAGIC_LINE)
        return std::stod(temp) / 10;

    return 0;
}

#endif
