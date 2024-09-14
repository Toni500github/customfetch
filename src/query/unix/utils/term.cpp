#include "term.hpp"
#include "fmt/format.h"
#include "util.hpp"
#include <fstream>

std::string detect_st_ver()
{
    std::string ret, line;
    std::ifstream f(which("st"), std::ios::binary);

    while (read_binary_file(f, line))
    {
        if (line == "WINDOWID" && hasStart(ret, "%s "))
            return ret.substr(3);

        ret = line;
    }
    debug("failed to fast detect st version");
    return UNKNOWN;
}

std::string detect_konsole_ver()
{
    const char* env = std::getenv("KONSOLE_VERSION");
    if (env)
    {
        long major = strtol(env, NULL, 10);
        if (major >= 0)
        {
            long patch = major % 100;
            major /= 100;
            long minor = major % 100;
            major /= 100;
            return fmt::format("{}.{}.{}", major, minor, patch);
        }
    }
    debug("failed to fast detect konsole version");
    return UNKNOWN;
}
