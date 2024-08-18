#ifndef _UTILS_HPP
#define _UTILS_HPP

#include <dlfcn.h>
#include <sys/types.h>

#include <string>
#include <vector>

#include "fmt/color.h"
#include "fmt/core.h"

// clang-format off
constexpr std::size_t operator""_len(const char*, std::size_t ln) noexcept
{ 
    return ln;
}

struct byte_units_t
{
    std::string unit;
    float       num_bytes;
};

constexpr const char NOCOLOR[] = "\033[0m";
constexpr const char UNKNOWN[] = "(unknown)";

// magic line to be sure that I don't cut the wrong line
constexpr const char MAGIC_LINE[] = "(cut this shit NOW!! RAHHH)";

/* lib  = library to load (string)
 * code = code to execute if anything goes wrong
 */
#define LOAD_LIBRARY(lib, code)            \
    void* handle = dlopen(lib, RTLD_LAZY); \
    if (!handle)                           \
        code;

/* ret_type = type of what the function returns
 * func     = the function name
 * ...      = the arguments in a function if any
 */
#define LOAD_LIB_SYMBOL(ret_type, func, ...)   \
    typedef ret_type (*func##_t)(__VA_ARGS__); \
    func##_t func = reinterpret_cast<func##_t>(dlsym(handle, #func));

#define UNLOAD_LIBRARY() dlclose(handle);

#define BOLD_COLOR(x) (fmt::emphasis::bold | fmt::fg(x))

bool         hasEnding(const std::string_view fullString, const std::string_view ending);
bool         hasStart(const std::string_view fullString, const std::string_view start);
std::string  name_from_entry(size_t dev_entry_pos);
std::string  vendor_from_entry(size_t vendor_entry_pos, const std::string_view vendor_id);
std::string  binarySearchPCIArray(const std::string_view vendor_id, const std::string_view pci_id);
std::string  binarySearchPCIArray(const std::string_view vendor_id);
std::string  shell_exec(const std::string_view cmd);
void         getFileValue(u_short& iterIndex, const std::string& line, std::string& str, const size_t& amount);
byte_units_t auto_devide_bytes(const size_t num);
bool         is_file_image(const unsigned char* bytes);
void         ctrl_d_handler(const std::istream& cin);
std::string  expandVar(std::string ret);
std::string  which(const std::string& command);
bool         read_binary_file(std::ifstream& f, std::string& ret);
void         replace_str(std::string& str, const std::string& from, const std::string& to);
bool         read_exec(std::vector<const char*> cmd, std::string& output, bool useStdErr = false, bool noerror_print = true);
std::string  str_tolower(std::string str);
std::string  str_toupper(std::string str);
void         strip(std::string& input);
std::string  read_by_syspath(const std::string_view path);
fmt::rgb     hexStringToColor(const std::string_view hexstr);
std::string  getHomeConfigDir();
std::string  getConfigDir();
std::vector<std::string> split(const std::string_view text, char delim);

inline void CF_ShortenVendorName(std::string &vendor) {
    if (vendor == "Advanced Micro Devices, Inc.")
        vendor = "AMD";
    else if (vendor == "Intel Corporation")
        vendor = "Intel";
    else if (vendor == "NVIDIA Corporation")
        vendor = "NVIDIA";
}

template <typename... Args>
void error(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::println(stderr, BOLD_COLOR(fmt::rgb(fmt::color::red)), "ERROR: {}",
                 fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

template <typename... Args>
void die(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::println(stderr, BOLD_COLOR(fmt::rgb(fmt::color::red)), "ERROR: {}",
                 fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
    std::exit(1);
}

template <typename... Args>
void debug(const std::string_view fmt, Args&&... args) noexcept
{
#if DEBUG
    fmt::println(BOLD_COLOR((fmt::rgb(fmt::color::hot_pink))), "[DEBUG]: {}",
                 fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
#endif
}

template <typename... Args>
void warn(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::println(BOLD_COLOR((fmt::rgb(fmt::color::yellow))), "WARNING: {}",
                 fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

#endif
