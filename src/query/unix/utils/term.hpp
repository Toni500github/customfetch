#ifndef _TERM_HPP
#define _TERM_HPP

#include <string>

void get_term_version_exec(const std::string_view term, std::string& ret, bool _short = false, bool _stderr = false);

bool fast_detect_konsole_ver(std::string& ret);
bool fast_detect_st_ver(std::string& ret);

#endif // _TERM_HPP
