#include "term.hpp"

#include <fstream>

#include "fmt/format.h"
#include "util.hpp"

void get_term_version_exec(const std::string_view term, std::string& ret, bool _short, bool _stderr)
{
    ret.clear();
    read_exec({ term.data(), _short ? "-v" : "--version" }, ret, _stderr);
}

bool fast_detect_st_ver(std::string& ret)
{
    std::string   line;
    std::ifstream f(which("st"), std::ios::binary);

    while (read_binary_file(f, line))
    {
        if (line == "WINDOWID" && hasStart(ret, "%s "))
        {
            ret.erase(0, 3);
            return true;
        }

        ret = line;
    }
    debug("failed to fast detect st version");

    get_term_version_exec("st", ret, true, true);
    return false;
}

// https://github.com/fastfetch-cli/fastfetch/blob/dev/src/detection/terminalshell/terminalshell.c#L345
bool fast_detect_konsole_ver(std::string& ret)
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
            ret = fmt::format("{}.{}.{}", major, minor, patch);
            return true;
        }
    }

    debug("failed to fast detect konsole version");
    get_term_version_exec("konsole", ret);

    return false;
}
