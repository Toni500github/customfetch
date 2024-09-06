#include "term.hpp"
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
    debug("failed to detect st version");
    return UNKNOWN;
}
