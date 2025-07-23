/*
 * Copyright 2025 Toni500git
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 * following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote
 * products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _UTILS_HPP
#define _UTILS_HPP

#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "fmt/base.h"
#include "fmt/color.h"
#include "libcufetch/common.hh"
#include "platform.hpp"

// clang-format off
// Get string literal length
constexpr std::size_t operator""_len(const char*, std::size_t ln) noexcept
{
    return ln;
}

// used for auto_devide_bytes()
struct byte_units_t
{
    std::string unit;
    double      num_bytes;
};

#if ENABLE_NLS && !CF_MACOS
/* here so it doesn't need to be included elsewhere */
#include <libintl.h>
#include <locale.h>
#define _(str) gettext(str)
#else
#define _(s) (char*)s
#endif

/* lib = library to load (string) */
#define LOAD_LIBRARY(lib) dlopen(lib, RTLD_NOW);

/* handler  = the library handle
 * ret_type = type of what the function returns
 * func     = the function name
 * ...      = the arguments in a function if any
 */
#define LOAD_LIB_SYMBOL(handler, ret_type, func, ...) \
    typedef ret_type (*func##_t)(__VA_ARGS__);        \
    func##_t func = reinterpret_cast<func##_t>(dlsym(handler, #func));

#define UNLOAD_LIBRARY(handle) dlclose(handle);

inline bool is_live_mode = false;

/* https://stackoverflow.com/questions/874134/find-out-if-string-ends-with-another-string-in-c#874160
 * Check if substring exists at the end
 * @param fullString The string to lookup
 * @param ending The string to check at the end of fullString
 */
EXPORT bool hasEnding(const std::string_view fullString, const std::string_view ending);

/* https://stackoverflow.com/questions/874134/find-out-if-string-ends-with-another-string-in-c#874160
 * Check if substring exists at the start
 * @param fullString The string to lookup
 * @param start The string to check at the start of fullString
 */
EXPORT bool hasStart(const std::string_view fullString, const std::string_view start);

/* Spilt a string into a vector using a delimeter
 * @param text The string to split
 * @param delim The delimeter used for spliting the text
 */
EXPORT std::vector<std::string> split(const std::string_view text, const char delim);

/* Get device name from `all_ids` in pci.ids.hpp
 * @param dev_entry_pos Line position from where the device is located
 */
EXPORT std::string name_from_entry(size_t dev_entry_pos);

/* Get vendor device name from `all_ids` in pci.ids.hpp
 * @param vendor_entry_pos Line position from where the device is located
 * @param vendor_id_s The vendor ID (e.g 10de)
 */
EXPORT std::string vendor_from_entry(const size_t vendor_entry_pos, const std::string_view vendor_id_s);

/* Function to perform binary search on the pci vendors array to find a device from a vendor.
 * @param vendor_id_s The vendor ID (e.g 10de)
 * @param pci_id_s The device ID (e.g 1f82)
 */
EXPORT std::string binarySearchPCIArray(const std::string_view vendor_id_s, const std::string_view pci_id_s);

/* Function to perform binary search on the pci vendors array to find a vendor.
 * @param vendor_id_s The vendor ID (e.g 10de)
 */
EXPORT std::string binarySearchPCIArray(const std::string_view vendor_id_s);

/* Get file value from a file and trim quotes and double-quotes
 * @param iterIndex The iteration index used for getting the necessary value only tot times
 * @param line The string used in std::getline
 * @param str The string to assign the trimmed value, inline
 * @param amount The amount to be used in the line.substr() (should be used with something like "foobar"_len)
 */
EXPORT void getFileValue(u_short& iterIndex, const std::string_view line, std::string& str, const size_t& amount);

/* Covert bytes (or bibytes) to be accurate to the max prefix (maxprefix or YB/YiB)
 * @param num The number to convert
 * @param base Base to devide (1000 = bytes OR 1024 = bibytes)
 * @param maxprefix The maxinum prefix we can go up to (empty for ignore)
 */
EXPORT byte_units_t auto_divide_bytes(const double num, const std::uint16_t base, const std::string_view maxprefix = "");

/* Covert bytes (or bibytes) to be accurate to a specific prefix
 * @param num The number to convert
 * @param prefix The prefix we want to convert to (GiB, MB, etc.)
 */
EXPORT byte_units_t divide_bytes(const double num, const std::string_view prefix);

/* Check if file is image (static or gif).
 * Doesn't check for mp4, mp3 or other binary formats
 * @param bytes The header bytes of the file
 */
EXPORT bool is_file_image(const unsigned char* bytes);

/* Write error message and exit if EOF (or CTRL-D most of the time)
 * @param cin The std::cin used for getting the input
 */ 
EXPORT void ctrl_d_handler(const std::istream& cin);

/* Replace special symbols such as ~ and $ (at the begging) in std::string
 * @param str The string
 * @param dont Don't do it
 * @return The modified string
 */
EXPORT std::string expandVar(std::string ret, bool dont = false);

/* Get a relative path from an enviroment variable (PATH, XDG_DATA_DIRS, ...)
 * Either path of an executable, directory, etc...
 * @param relative_path The path we would search in the env
 * @param env The enviroment variable without $
 * @param mode Mode of the file/directory using the enums declared in sys/stat.h
 *             Such as S_IXUSR for executables, S_IFDIR for directories, etc.
 */
EXPORT std::string get_relative_path(const std::string_view relative_path, const std::string_view env, const long long mode);

/* Simulate behaviour of the command `which`
 * @param command The command to lookup in the $PATH
 */
EXPORT std::string which(const std::string_view command);

/* Get file path from $XDG_DATA_DIRS
 * @param file The file to lookup in the env
 */
EXPORT std::string get_data_path(const std::string_view file);

/* Get directory path from $XDG_DATA_DIRS
 * @param dir The directory to lookup in the env
 */
EXPORT std::string get_data_dir(const std::string_view dir);

/* Read a binary file and get its current line,
 * which simulates the behaviour of the command `strings` but one line at the time
 * @param f The std::ifstream of the file
 * @param ret The string to return after we read the line
 * @example
 *  std::ifstream f(exec_path, std::ios::binary);
 *  if (!f.is_open())
 *       return false;
 *
 *  std::string line;
 *  while (read_binary_file(f, line))
 *  {
 *      if (line == "using GTK+-%d.%d.%d.")
 *          return true;
 *
 *      // previous line, which will eventually be the version
 *       ret = line;
 *   }
 *   return false;
 */
EXPORT bool read_binary_file(std::ifstream& f, std::string& ret);

/* https://gist.github.com/GenesisFR/cceaf433d5b42dcdddecdddee0657292
 * Replace every instances (inplace) of a substring
 * @param str The full string to use
 * @param from The substring to lookup to be replaced
 * @param to The substring to replace in instances of `from`
 */
EXPORT void replace_str(std::string& str, const std::string_view from, const std::string_view to);

/* Executes commands with execvp() and read its output
 * either from stdout or stderr
 * @param cmd The command array to execute
 * @param output The string to use for appending the output
 * @param useStdErr Read from stderr instead of stdout (default false)
 * @param noerror_print Print errors (default true)
 * @return true if the command successed, else false
 */
EXPORT bool read_exec(std::vector<std::string> cmd, std::string& output, bool useStdErr = false, bool noerror_print = true);

/* Make whole string lowercase
 * @param str The string to use
 */
EXPORT std::string str_tolower(std::string str);

/* Make whole string uppercase
 * @param str The string to use
 */
EXPORT std::string str_toupper(std::string str);

/* Remove all white spaces (' ', '\t', '\n') from string inplace!
 * @param input The string to strip
 * @param padding_only Just trim the string
 */
EXPORT void strip(std::string& input, bool padding_only = true);

/* Read file content (usually from /sys)
 * and return its first (and only) line, else UNKNOWN
 * @param path The path of the file to read
 * @param report_error Report error if any
 */
EXPORT std::string read_by_syspath(const std::string_view path, bool report_error = false);

/* Convert hex color (#255224) to a fmt::rgb
 * @param hexstr The hex color string (must have a '#' at the start)
 */
EXPORT fmt::rgb hexStringToColor(const std::string_view hexstr);

/* Abbreviate the vendors names
 * @param vendor The vendor name
 */
EXPORT std::string shorten_vendor_name(std::string vendor);

/*
 * Get the user config directory
 * either from $XDG_CONFIG_HOME or from $HOME/.config/
 * @return user's config directory
 */
EXPORT std::filesystem::path getHomeConfigDir();

/*
 * Get the customfetch config directory
 * where we'll have "config.toml"
 * from getHomeConfigDir()
 * @return customfetch's config directory
 */
EXPORT std::filesystem::path getConfigDir();

/*
 * Get the user cache directory
 * either from $XDG_CACHE_HOME or from $HOME/.cache/
 * @return user's cache directory
 */
EXPORT std::filesystem::path getHomeCacheDir();

/*
 * Get the customfetch cache directory
 * @return customfetch's cache directory
 */
EXPORT std::filesystem::path getCacheDir();

#if CF_ANDROID
/* Get android property name such as "ro.product.marketname"
 * @param name The property name
 */
EXPORT std::string get_android_property(const std::string_view name);
#endif

/** Ask the user a yes or no question.
 * @param def The default result
 * @param fmt The format string
 * @param args Arguments in the format
 * @returns the result, y = true, n = false, only returns def if the result is def
 */
template <typename... Args>
EXPORT bool askUserYorN(bool def, const std::string_view fmt, Args&&... args)
{
    const std::string& inputs_str = fmt::format(" [{}]: ", def ? "Y/n" : "y/N");
    std::string result;
    fmt::print(fmt::runtime(fmt), std::forward<Args>(args)...);
    fmt::print("{}", inputs_str);

    while (std::getline(std::cin, result) && (result.length() > 1))
        warn("Please answear y or n {}", inputs_str);

    ctrl_d_handler(std::cin);

    if (result.empty())
        return def;

    if (def ? std::tolower(result[0]) != 'n' : std::tolower(result[0]) != 'y')
        return def;

    return !def;
}

#endif
