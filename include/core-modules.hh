#pragma once

#include <pwd.h>
#include <sys/utsname.h>

#include "cufetch/cufetch.hh"

#define MODFUNC(name) const std::string name(__attribute__((unused)) const callbackInfo_t* callbackInfo = nullptr)

// system.cc
inline utsname g_uname_infos;
MODFUNC(arch);
MODFUNC(host);
MODFUNC(host_name);
MODFUNC(host_version);
MODFUNC(host_vendor);

// os.cc
inline std::FILE* os_release;
MODFUNC(os_name);
MODFUNC(os_pretty_name);
MODFUNC(os_name_id);
MODFUNC(os_version_id);
MODFUNC(os_version_codename);
unsigned long os_uptime();
MODFUNC(os_kernel_name);
MODFUNC(os_kernel_version);
MODFUNC(os_hostname);
MODFUNC(os_initsys_name);
MODFUNC(os_initsys_version);

// cpu.cc
inline std::FILE* cpuinfo;
MODFUNC(cpu_freq_cur);
MODFUNC(cpu_freq_max);
MODFUNC(cpu_freq_min);
MODFUNC(cpu_freq_bios);
float   cpu_temp();
MODFUNC(cpu_nproc);
MODFUNC(cpu_name);

// user.cc
inline struct passwd* g_pwd;
inline bool           is_tty = false;
inline std::string    term_pid, term_name, wm_name, de_name, wm_path_exec;
std::string           get_terminal_name();
std::string           get_terminal_pid();
MODFUNC(user_name);
MODFUNC(user_shell_path);
MODFUNC(user_shell_name);
MODFUNC(user_shell_version);
MODFUNC(user_term_name);
MODFUNC(user_term_version);
MODFUNC(user_wm_name);
MODFUNC(user_wm_version);
MODFUNC(user_de_name);
MODFUNC(user_de_version);

// ram.cc and swap.cc
inline std::FILE* meminfo;
double            ram_free();
double            ram_total();
double            ram_used();
double            swap_free();
double            swap_total();
double            swap_used();

// disk.cc
inline std::FILE* mountsFile;
MODFUNC(disk_fsname);
MODFUNC(disk_device);
MODFUNC(disk_mountdir);
MODFUNC(disk_types);
double disk_total(const callbackInfo_t* callbackInfo);
double disk_free(const callbackInfo_t* callbackInfo);
double disk_used(const callbackInfo_t* callbackInfo);

// battery.cc
MODFUNC(battery_modelname);
MODFUNC(battery_perc);
MODFUNC(battery_status);
MODFUNC(battery_capacity_level);
MODFUNC(battery_technology);
MODFUNC(battery_vendor);
double battery_temp();

// gpu.cc
MODFUNC(gpu_name);
MODFUNC(gpu_vendor);

#undef MODFUNC
#define MODFUNC(name) const std::string name(__attribute__((unused)) const callbackInfo_t* callbackInfo)
