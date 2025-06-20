#include <cstdio>
#include <cstring>
#include <cassert>
#include <string>

#include "common.hpp"
#include "linux-core-modules.hh"

static const char *read_value(const char *name, size_t n)
{
    if (!os_release)
        return strdup(UNKNOWN);
    rewind(os_release); // Reset file pointer to start

    char *buf = strdup(UNKNOWN); // Default value
    char *line = NULL;
    size_t len = 0;
    while (getline(&line, &len, os_release) != -1)
    {
        if (strncmp(line, name, n) != 0)
            continue;

        // Find the first quote after the key
        char *start = strchr(line + n, '"');
        if (!start) continue;
        start++;

        // Find the closing quote
        char *end = strrchr(start, '"');
        if (!end) continue;

        free(buf);
        buf = strndup(start, end - start);
        break;
    }

    free(line);
    return buf;
}

const std::string os_name()
{ return read_value("NAME=", 5); }

const std::string os_pretty_name()
{ return read_value("PRETTY_NAME=", 12); }

const std::string os_name_id()
{ return read_value("ID=", 3); }
