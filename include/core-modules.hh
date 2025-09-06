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

#pragma once

#include <pwd.h>
#include <sys/utsname.h>

#include "config.hpp"
#include "libcufetch/cufetch.hh"

#define MODFUNC(name) std::string name(__attribute__((unused)) const callbackInfo_t* callbackInfo)

// system.cc
MODFUNC(arch);
MODFUNC(host);
MODFUNC(host_name);
MODFUNC(host_version);
MODFUNC(host_vendor);

// os.cc
inline utsname    g_uname_infos;
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
float cpu_temp();
MODFUNC(cpu_nproc);
MODFUNC(cpu_name);
MODFUNC(android_cpu_vendor);
MODFUNC(android_cpu_model_name);

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
enum
{
    DISK_VOLUME_TYPE_HIDDEN    = 1 << 2,
    DISK_VOLUME_TYPE_REGULAR   = 1 << 3,
    DISK_VOLUME_TYPE_EXTERNAL  = 1 << 4,
    DISK_VOLUME_TYPE_READ_ONLY = 1 << 5,
};

inline std::FILE* mountsFile;
MODFUNC(disk_fsname);
MODFUNC(disk_device);
MODFUNC(disk_mountdir);
MODFUNC(disk_types);
MODFUNC(auto_disk);
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

// theme.cc
MODFUNC(theme_gtk_name);
MODFUNC(theme_gtk_icon);
MODFUNC(theme_gtk_font);
MODFUNC(theme_cursor_name);
MODFUNC(theme_cursor_size);
MODFUNC(theme_gtk_all_name);
MODFUNC(theme_gtk_all_icon);
MODFUNC(theme_gtk_all_font);
MODFUNC(theme_gsettings_name);
MODFUNC(theme_gsettings_icon);
MODFUNC(theme_gsettings_font);
MODFUNC(theme_gsettings_cursor_name);
MODFUNC(theme_gsettings_cursor_size);

void core_plugins_start(const Config& config);
void core_plugins_finish();
