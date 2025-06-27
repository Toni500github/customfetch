#include <unistd.h>

#include <string>

#include "common.hpp"
#include "core-modules.hh"
#include "fmt/format.h"
#include "util.hpp"

const std::string freq_dir = "/sys/devices/system/cpu/cpu0/cpufreq";

static void trim(char* str)
{
    if (!str)
        return;

    // Trim leading space
    char* p = str;
    while (isspace((unsigned char)*p))
        ++p;
    memmove(str, p, strlen(p) + 1);

    // Trim trailing space
    p = str + strlen(str) - 1;
    while (p >= str && isspace((unsigned char)*p))
        --p;
    p[1] = '\0';
}

static bool read_value(const char* name, size_t n, bool do_rewind, char* buf, size_t buf_size)
{
    if (!cpuinfo || !buf || !buf_size)
        return false;
    if (do_rewind)
        rewind(cpuinfo);

    char*  line  = NULL;
    size_t len   = 0;
    bool   found = false;
    while (getline(&line, &len, cpuinfo) != -1)
    {
        if (strncmp(line, name, n))
            continue;

        char* colon = strchr(line, ':');
        if (!colon)
            continue;

        // Extract and trim value
        char* val = colon + 1;
        while (isspace((unsigned char)*val))
            ++val;
        trim(val);

        // Safe copy to buffer
        strncpy(buf, val, buf_size - 1);
        buf[buf_size - 1] = '\0';

        found = true;
        break;
    }

    free(line);
    return found;
}

float cpu_temp()
{
    for (const auto& dir : std::filesystem::directory_iterator{ "/sys/class/hwmon/" })
    {
        const std::string& name = read_by_syspath((dir.path() / "name").string());
        debug("name = {}", name);
        if (name != "cpu" && name != "k10temp" && name != "coretemp")
            continue;

        const std::string& temp_file = (access((dir.path() / "temp1_input").string().c_str(), F_OK) != 0)
                                           ? dir.path() / "device/temp1_input"
                                           : dir.path() / "temp1_input";
        if (access(temp_file.c_str(), F_OK) != 0)
            continue;

        const float ret = std::stof(read_by_syspath(temp_file));
        debug("cpu temp ret = {}", ret);

        return ret / 1000.0f;
    }
    return 0.0f;
}

MODFUNC(cpu_name)
{
    char name[4096];
    if (!read_value("model name", "model name"_len, true, name, sizeof(name)))
        return UNKNOWN;

    // sometimes /proc/cpuinfo at model name
    // the name will contain the min freq
    // happens on intel cpus especially
    char* at = strrchr(name, '@');
    if (!at)
        return name;
    if (at > name && *(at - 1) == ' ')
        *(at - 1) = '\0';
    else
        *at = '\0';

    trim(name);
    return name;
}

MODFUNC(cpu_nproc)
{
    uint nproc = 0;
    rewind(cpuinfo);

    char*  line = NULL;
    size_t len  = 0;
    while (getline(&line, &len, cpuinfo) != -1)
    {
        if (strncmp(line, "processor", "processor"_len) == 0)
            nproc++;
    }
    free(line);
    return fmt::to_string(nproc);
}

MODFUNC(cpu_freq_cur)
{
    if (access((freq_dir + "/scaling_cur_freq").c_str(), F_OK) != 0)
        return "0";
    return fmt::format("{:.2f}", std::stof(read_by_syspath(freq_dir + "/scaling_cur_freq")) / 1000000);
}

MODFUNC(cpu_freq_max)
{
    if (access((freq_dir + "/scaling_max_freq").c_str(), F_OK) != 0)
        return "0";
    return fmt::format("{:.2f}", std::stof(read_by_syspath(freq_dir + "/scaling_max_freq")) / 1000000);
}

MODFUNC(cpu_freq_min)
{
    if (access((freq_dir + "/scaling_min_freq").c_str(), F_OK) != 0)
        return "0";
    return fmt::format("{:.2f}", std::stof(read_by_syspath(freq_dir + "/scaling_min_freq")) / 1000000);
}

MODFUNC(cpu_freq_bios)
{
    if (access((freq_dir + "/bios_limit").c_str(), F_OK) != 0)
        return "0";
    return fmt::format("{:.2f}", std::stof(read_by_syspath(freq_dir + "/bios_limit")) / 1000000);
}
