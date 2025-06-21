#pragma once

#include "common.hpp"
#include <sys/utsname.h>

inline utsname g_uname_infos;

modfunc arch();
modfunc host();
modfunc host_name();
modfunc host_version();
modfunc host_vendor();

inline std::FILE *os_release;
modfunc os_name();
modfunc os_pretty_name();
modfunc os_name_id();
modfunc os_version_id();
modfunc os_version_codename();
modfunc os_uptime();
modfunc os_kernel_name();
modfunc os_kernel_version();
modfunc os_hostname();
modfunc os_initsys_name();
modfunc os_initsys_version();
