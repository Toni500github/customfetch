#ifndef _UTILS_HPP
#define _UTILS_HPP

#include "fmt/color.h"
#include "fmt/core.h"
#include "platform.hpp"

#include <vector>
#include <string>

#ifndef CF_WINDOWS
# include <dlfcn.h>
# include <sys/types.h>
#endif

#ifndef CF_WINDOWS
/* lib  = library to load (string)
 * code = code to execute if anything goes wrong
 */
#define LOAD_LIBRARY(lib, code) \
void *handle = dlopen(lib, RTLD_LAZY); \
if (!handle) \
    code;

/* ret_type = type of what the function returns
 * func     = the function name
 * ...      = the arguments in a function if any
 */
#define LOAD_LIB_SYMBOL(ret_type, func, ...) \
typedef ret_type (* func ## _t)(__VA_ARGS__); \
func ## _t func = reinterpret_cast<func ## _t>(dlsym(handle, #func)); \

#define UNLOAD_LIBRARY() dlclose(handle);
#else
#define LOAD_LIBRARY(lib, code) do {} while(0);
#define LOAD_LIB_SYMBOL(ret_type, func, ...) do {} while(0);
#define UNLOAD_LIBRARY() do {} while(0);
#endif

consteval std::size_t operator""_len(const char*,std::size_t ln) noexcept{
    return ln;
}

struct byte_units_t {
    std::string unit;
    float num_bytes;
};

constexpr const char NOCOLOR[] = "\033[0m";
constexpr const char UNKNOWN[] = "(unknown)";

// magic line to be sure that I don't cut the wrong line 
constexpr const char MAGIC_LINE[] = "(cut this shit NOW!! RAHHH)";

bool hasEnding(const std::string_view fullString, const std::string_view ending);
bool hasStart(const std::string_view fullString, const std::string_view start);
std::string name_from_entry(size_t dev_entry_pos);
std::string vendor_from_entry(size_t vendor_entry_pos, const std::string_view vendor_id);
std::string binarySearchPCIArray(const std::string_view vendor_id, const std::string_view pci_id);
std::string binarySearchPCIArray(const std::string_view vendor_id);
std::string shell_exec(const std::string_view cmd);
std::vector<std::string> split(const std::string_view text, char delim);
byte_units_t auto_devide_bytes(const size_t num);
bool is_file_image(const unsigned char *bytes);
std::string expandVar(std::string ret);
// Replace string inplace
void replace_str(std::string& str, const std::string& from, const std::string& to);
bool read_exec(std::vector<const char *> cmd, std::string& output, bool useStdErr = false, bool noerror_print = true);
std::string str_tolower(const std::string str);
std::string str_toupper(const std::string str);
void strip(std::string& input);
std::string read_by_syspath(const std::string_view path);
fmt::rgb hexStringToColor(const std::string_view hexstr);
std::string getHomeConfigDir();
std::string getConfigDir();

template <typename... Args>
void error(const std::string_view fmt, Args&&... args) noexcept{
    fmt::println(stderr, (fmt::emphasis::bold | fmt::fg(fmt::rgb(fmt::color::red))), "ERROR: {}", fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

template <typename... Args>
void die(const std::string_view fmt, Args&&... args) noexcept{
    fmt::println(stderr, (fmt::emphasis::bold | fmt::fg(fmt::rgb(fmt::color::red))), "ERROR: {}", fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
    std::exit(1);
}

template <typename... Args>
void debug(const std::string_view fmt, Args&&... args) noexcept{
#if DEBUG
    fmt::println((fmt::emphasis::bold | fmt::fg((fmt::rgb(fmt::color::hot_pink)))), "[DEBUG]: {}", fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
#endif
}

template <typename... Args>
void warn(const std::string_view fmt, Args&&... args) noexcept{
    fmt::println((fmt::emphasis::bold | fmt::fg((fmt::rgb(fmt::color::yellow)))), "WARNING: {}", fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

#endif
