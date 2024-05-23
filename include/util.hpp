#include "fmt/color.h"
#include "fmt/core.h"

#include <cstdio>
#include <filesystem>
#include <string>

using std::string;
using std::string_view;
using std::filesystem::path;

#ifdef ENABLE_NLS
/* here so it doesn't need to be included elsewhere */
#include <libintl.h> 
#include <locale.h>
/* define _() as shortcut for gettext() */
#define _(str) gettext(str)
#else
#define _(s) (char *)s
#endif

#define BOLD           fmt::emphasis::bold
#define BOLD_TEXT(x)   (fmt::emphasis::bold | fmt::fg(x))
#define NOCOLOR        "\033[0m"

template <typename... Args>
void error(fmt::runtime_format_string<> fmt, Args&&... args) {
    fmt::println(stderr, BOLD_TEXT(fmt::rgb(fmt::color::red)), "ERROR: {}", fmt::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void die(string_view fmt, Args&&... args) {
    error(fmt::runtime(fmt), std::forward<Args>(args)...);
    std::exit(1);
}

template <typename... Args>
void die(const char *fmt, Args&&... args) {
    error(fmt::runtime(fmt), std::forward<Args>(args)...);
    std::exit(1);
}
