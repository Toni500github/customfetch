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

#ifndef _DISPLAY_HPP
#define _DISPLAY_HPP

#include <string>
#include <vector>

#include "config.hpp"
#include "platform.hpp"
#include "query.hpp"

#if CF_MACOS
constexpr std::string_view ascii_logo =
R"(${green}                    c.'
${green}                 ,xNMM.
${green}               .OMMMMo
${green}               lMM"
${green}     .;loddo:.  .olloddol;.
${green}   cKMMMMMMMMMMNWMMMMMMMMMM0:
${yellow} .KMMMMMMMMMMMMMMMMMMMMMMMWd.
${yellow} XMMMMMMMMMMMMMMMMMMMMMMMX.
${red};MMMMMMMMMMMMMMMMMMMMMMMM:
${red}:MMMMMMMMMMMMMMMMMMMMMMMM:
${red}.MMMMMMMMMMMMMMMMMMMMMMMMX.
${red} kMMMMMMMMMMMMMMMMMMMMMMMMWd.
${magenta} 'XMMMMMMMMMMMMMMMMMMMMMMMMMMk
${magenta}  'XMMMMMMMMMMMMMMMMMMMMMMMMK.
    ${blue}kMMMMMMMMMMMMMMMMMMMMMMd
     ${blue};KMMMMMMMWXXWMMMMMMMk.
       ${blue}"cooc*"    "*coo'"
)";
#elif CF_ANDROID
constexpr std::string_view ascii_logo =
R"(${green}  ;,           ,;
${green}   ';,.-----.,;'
${green}  ,'           ',
${green} /    O     O    \\
${green}|                 |
${green}'-----------------'
)";
#else
constexpr std::string_view ascii_logo =
R"(${black}        #####
${black}       #######
${black}       ##${1}O${black}#${1}O${black}##
${black}       #${yellow}#####${black}#
${black}     ##${1}##${yellow}###${1}##${black}##
${black}    #${1}##########${black}##
${black}   #${1}############${black}##
${black}   #${1}############${black}###
${yellow}  ##${black}#${1}###########${black}##${yellow}#
${yellow}######${black}#${1}#######${black}#${yellow}######
${yellow}#######${black}#${1}#####${black}#${yellow}#######
${yellow}  #####${black}#######${yellow}#####
)";
#endif

namespace Display
{

/*
 * Render the layout along side the source file and return the vector
 * @param config The config class
 * @param colors The colors
 * @param already_analyzed_path If already checked that the source path is not a binary file
 * @param path Path to source file
 */
std::vector<std::string> render(const Config& config, const colors_t& colors, const bool already_analyzed_path,
                                const std::filesystem::path& path, moduleMap_t& moduleMap);

/*
 * Display the rendered result (or just display a normal vector of string
 * @param renderResult The rendered vector usually by Display::render()
 */
void display(const std::vector<std::string>& renderResult);

/*
 * Detect the distro you are using and return the path to the ASCII art
 * @param config The config class
 */
std::string detect_distro(const Config& config);

inline unsigned int calc_perc(const float perc, const int width, const int len)
{
    const int ret = (perc / 100 * width) - (perc / 100 * len);
    debug("maxLineLength = {}", len);
    debug("calc_perc ret = {}", ret);
    return ret > 0 ? ret : 0;
}

// default ascii logo fd
inline int ascii_logo_fd = -1;

}  // namespace Display

#endif
