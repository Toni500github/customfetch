#pragma once

#include "common.hpp"
#include <pwd.h>
#include <sys/utsname.h>

// system.cc
inline utsname g_uname_infos;
modfunc arch();
modfunc host();
modfunc host_name();
modfunc host_version();
modfunc host_vendor();

// os.cc
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

// cpu.cc
inline std::FILE *cpuinfo;
modfunc cpu_freq_cur();
modfunc cpu_freq_max();
modfunc cpu_freq_min();
modfunc cpu_freq_bios();
float   cpu_temp();
modfunc cpu_nproc();
modfunc cpu_name();

// user.cc
inline struct passwd* g_pwd;
inline bool is_tty = false;
inline std::string term_pid, term_name, wm_name, de_name, wm_path_exec;
std::string get_terminal_name();
std::string get_terminal_pid();
modfunc user_name();
modfunc user_shell_path();
modfunc user_shell_name();
modfunc user_shell_version();
modfunc user_term_name();
modfunc user_term_version();
modfunc user_wm_name();
modfunc user_wm_version();
modfunc user_de_name();
modfunc user_de_version();
