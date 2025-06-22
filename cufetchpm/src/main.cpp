#include <cstring>
#include <string_view>
#include "common.hpp"
#include "fmt/base.h"

bool download_git(const std::string_view url);

int main (int argc, char *argv[])
{
    if (argc > 1 && strcmp(argv[1], "add") == 0)
    {
        if (argc == 2)
            die("please insert a git url to clone");

        if (download_git(argv[2]))
            info("plugin downloaded successfully");
    }
    else
    {
        fmt::println("usage: cufetchpm add <url>");
    }
    return 0;
}
