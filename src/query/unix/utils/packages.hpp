#ifndef _PACKAGES_HPP
#define _PACKAGES_HPP

#include "query.hpp"
#include "config.hpp"

using namespace Query;

std::string get_all_pkgs(System::pkg_managers_t& pkg_managers, const Config& config);

#endif
