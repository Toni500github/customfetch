#include <unistd.h>

#include <string>

#include "common.hpp"
#include "fmt/format.h"
#include "linux-core-modules.hh"
#include "util.hpp"

const std::string freq_dir = "/sys/devices/system/cpu/cpu0/cpufreq";

static char* trim_whitespace(char* str)
{
    if (!str)
        return NULL;

    // Trim leading whitespace
    while (isspace((unsigned char)*str))
        str++;

    // If all spaces
    if (*str == '\0')
        return strdup("");

    // Trim trailing whitespace
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;

    *(end + 1) = '\0';
    return strdup(str);
}

static char* read_value(const char* name, size_t n, bool do_rewind)
{
    if (!cpuinfo)
        return NULL;
    if (do_rewind)
        rewind(cpuinfo);

    char*  line  = NULL;
    size_t len   = 0;
    char*  value = NULL;

    while (getline(&line, &len, cpuinfo) != -1)
    {
        if (strncmp(line, name, n) != 0)
            continue;

        char* colon = strchr(line, ':');
        if (!colon)
            continue;

        value = trim_whitespace(colon + 1);
        break;
    }

    free(line);
    return value;
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

modfunc cpu_name()
{
    char* name = read_value("model name", "model name"_len, true);
    if (!name)
        return UNKNOWN;

    char* at = strrchr(name, '@');
    if (!at)
        return name;

    // sometimes /proc/cpuinfo at model name
    // the name will contain the min freq
    // happens on intel cpus especially
    if (at > name && *(at - 1) == ' ')
        *(at - 1) = '\0';
    else
        *at = '\0';

    return name;
}

modfunc cpu_nproc()
{
    uint nproc = 0;
    rewind(cpuinfo);
    while (read_value("processor", "processor"_len, false))
        ++nproc;
    return fmt::to_string(nproc);
}

modfunc cpu_freq_cur()
{
    if (access((freq_dir + "/scaling_cur_freq").c_str(), F_OK) != 0)
        return "0";
    return fmt::format("{:.2f}", std::stof(read_by_syspath(freq_dir + "/scaling_cur_freq")) / 1000000);
}

modfunc cpu_freq_max()
{
    if (access((freq_dir + "/scaling_max_freq").c_str(), F_OK) != 0)
        return "0";
    return fmt::format("{:.2f}", std::stof(read_by_syspath(freq_dir + "/scaling_max_freq")) / 1000000);
}

modfunc cpu_freq_min()
{
    if (access((freq_dir + "/scaling_min_freq").c_str(), F_OK) != 0)
        return "0";
    return fmt::format("{:.2f}", std::stof(read_by_syspath(freq_dir + "/scaling_min_freq")) / 1000000);
}

modfunc cpu_freq_bios()
{
    if (access((freq_dir + "/bios_limit").c_str(), F_OK) != 0)
        return "0";
    return fmt::format("{:.2f}", std::stof(read_by_syspath(freq_dir + "/bios_limit")) / 1000000);
}
