#include "common.hpp"
#include "linux-core-modules.hh"
#include <cstdio>
#include <string>
#include <string_view>

static double read_value(const std::string_view key)
{
    if (!meminfo)
        return 0.0;

    std::string result{UNKNOWN};
    char* line = nullptr;
    size_t len = 0;

    while (getline(&line, &len, meminfo) != -1)
    {
        if (strncmp(line, key.data(), key.length()) != 0)
            continue;

        // Skip colon and whitespace
        char* value = line + 1;
        while (isspace(*value)) value++;

        // Find end of numeric value (stop at first non-digit or '.')
        char* end = value;
        while (*end && (isdigit(*end) || *end == '.')) end++;

        if (value != end)
            result.assign(value, end - value);
        break;
    }

    free(line);
    rewind(meminfo);
    return std::stod(result);
}

double ram_free()
{ return read_value("MemAvailable:"); }

double ram_total()
{ return read_value("MemTotal:"); }

double swap_free()
{ return read_value("SwapFree:"); }

double swap_total()
{ return read_value("SwapTotal:"); }
