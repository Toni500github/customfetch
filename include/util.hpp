#ifndef UTILS_HPP
#define UTILS_HPP

#include "fmt/color.h"
#include "fmt/core.h"

#include <filesystem>
#include <vector>
#include <string>

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
#define UNKNOWN         "<unknown>"

std::string name_from_entry(size_t dev_entry_pos);
std::string vendor_from_id(const std::string& pci_ids, const std::string& id_str);
std::string binarySearchPCIArray(std::string_view vendor_id, std::string_view pci_id);
std::vector<std::string> split(std::string_view text, char delim);
void strip(std::string& input);

// it's std::binary_search but instead returns the std::string
template<typename _ForwardIterator, typename _Tp>
std::string binary_search_str(_ForwardIterator __first, _ForwardIterator __last,
		  const _Tp& __val) 
{
    _ForwardIterator __i
	= std::__lower_bound(__first, __last, __val,
			     __gnu_cxx::__ops::__iter_less_val());
      return (__i != __last && !(__val < *__i)) ? __val : UNKNOWN;
}

template <typename... Args>
void _error_log(fmt::runtime_format_string<> fmt, Args&&... args) {
    fmt::println(stderr, BOLD_TEXT(fmt::rgb(fmt::color::red)), "ERROR: {}", fmt::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void error(std::string_view fmt, Args&&... args) {
    _error_log(fmt::runtime(fmt), std::forward<Args>(args)...);
}

template <typename... Args>
void die(std::string_view fmt, Args&&... args) {
    _error_log(fmt::runtime(fmt), std::forward<Args>(args)...);
    std::exit(1);
}

template <typename... Args>
void die(const char *fmt, Args&&... args) {
    _error_log(fmt::runtime(fmt), std::forward<Args>(args)...);
    std::exit(1);
}

#endif
