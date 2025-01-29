/*
 * Copyright 2024 Toni500git
 * 
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "parse.hpp"

#include <unistd.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <ios>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "config.hpp"
#include "fmt/color.h"
#include "fmt/format.h"
#include "fmt/ranges.h"
#include "query.hpp"
#include "switch_fnv1a.hpp"
#include "util.hpp"

class Parser
{
public:
    Parser(const std::string_view src, std::string& pureOutput) : src{ src }, pureOutput{ pureOutput } {}

    bool try_read(const char c)
    {
        if (is_eof())
            return false;

        if (src[pos] == c)
        {
            ++pos;
            return true;
        }

        return false;
    }

    char read_char(const bool add_pureOutput = false)
    {
        if (is_eof())
            return 0;

        if (add_pureOutput)
            pureOutput += src[pos];

        ++pos;
        return src[pos - 1];
    }

    bool is_eof()
    { return pos >= src.length(); }

    void rewind(const size_t count = 1)
    { pos -= std::min(pos, count); }

    const std::string_view src;
    std::string&           pureOutput;
    size_t                 dollar_pos = 0;
    size_t                 pos = 0;
};

// declarations of static members in query.hpp
Query::System::System_t   Query::System::m_system_infos;
Query::Theme::Theme_t     Query::Theme::m_theme_infos;
Query::User::User_t       Query::User::m_users_infos;
Query::Battery::Battery_t Query::Battery::m_battery_infos;
Query::CPU::CPU_t         Query::CPU::m_cpu_infos;
Query::RAM::RAM_t         Query::RAM::m_memory_infos;
Query::GPU::GPU_t         Query::GPU::m_gpu_infos;
Query::Disk::Disk_t       Query::Disk::m_disk_infos;

struct statvfs Query::Disk::m_statvfs;
struct utsname Query::System::m_uname_infos;
struct sysinfo Query::System::m_sysInfos;
struct passwd* Query::User::m_pPwd;

bool Query::System::m_bInit          = false;
bool Query::RAM::m_bInit             = false;
bool Query::CPU::m_bInit             = false;
bool Query::User::m_bInit            = false;
bool Query::Battery::m_bInit         = false;
bool Query::User::m_bDont_query_dewm = false;

// useless useful tmp string for parse() without using the original
// pureOutput
std::string _;

static std::array<std::string, 3> get_ansi_color(const std::string_view str, const colors_t& colors)
{
    const size_t first_m = str.rfind('m');
    if (first_m == std::string::npos)
        die(_("Parser: failed to parse layout/ascii art: missing 'm' while using ANSI color escape code in '{}'"), str);

    std::string col = str.data();
    col.erase(first_m);  // 1;42

#if !ANDROID_APP
    std::string weight = hasStart(col, "1;") ? "bold" : "normal";
    std::string type   = "fgcolor";  // either fgcolor or bgcolor

    if (hasStart(col, "1;") || hasStart(col, "0;"))
        col.erase(0, 2);

    debug("col = {}", col);
    const int n = std::stoi(col);

    // unfortunatly you can't do bold and light in pango
    if ((n >= 100 && n <= 107) || (n >= 90 && n <= 97))
        weight = "light";

    if ((n >= 100 && n <= 107) || (n >= 40 && n <= 47))
        type = "bgcolor";
#else
    std::string weight = hasStart(col, "1;") ? "<b>" : "";
    std::string type   = "color";  // either color or background-color

    if (hasStart(col, "1;") || hasStart(col, "0;"))
        col.erase(0, 2);

    const int n = std::stoi(col);
    if ((n >= 100 && n <= 107) || (n >= 40 && n <= 47))
        type = "background-color";
#endif  // !ANDROID_APP

    // last number
    // clang-format off
    switch (col.back())
    {
        case '0': col = colors.gui_black;   break;
        case '1': col = colors.gui_red;     break;
        case '2': col = colors.gui_green;   break;
        case '3': col = colors.gui_yellow;  break;
        case '4': col = colors.gui_blue;    break;
        case '5': col = colors.gui_magenta; break;
        case '6': col = colors.gui_cyan;    break;
        case '7': col = colors.gui_white;   break;
    }

    if (col.at(0) != '#')
        col.erase(0, col.find('#'));

    return { col, weight, type };
    // clang-format on
}

static std::string convert_ansi_escape_rgb(const std::string_view noesc_str)
{
    if (std::count(noesc_str.begin(), noesc_str.end(), ';') < 4)
        die(_("ANSI escape code color '\\e[{}' should have an rgb type value\n"
              "e.g \\e[38;2;255;255;255m"),
            noesc_str);
    if (noesc_str.rfind('m') == std::string::npos)
        die(_("Parser: failed to parse layout/ascii art: missing 'm' while using ANSI color escape code in '\\e[{}'"),
            noesc_str);

    const std::vector<std::string>& rgb_str = split(noesc_str.substr(5), ';');

    const uint r      = std::stoul(rgb_str.at(0));
    const uint g      = std::stoul(rgb_str.at(1));
    const uint b      = std::stoul(rgb_str.at(2));
    const uint result = (r << 16) | (g << 8) | (b);

    std::stringstream ss;
    ss << std::hex << result;
    return ss.str();
}

std::string parse(const std::string& input, std::string& _, parse_args_t& parse_args)
{
    return parse(input, parse_args.systemInfo, _, parse_args.layout, parse_args.tmp_layout,
                 parse_args.config, parse_args.colors, parse_args.parsingLayout, parse_args.no_more_reset);
}

std::string parse(const std::string& input, parse_args_t& parse_args)
{
    return parse(input, parse_args.systemInfo, parse_args.pureOutput, parse_args.layout, parse_args.tmp_layout,
                 parse_args.config, parse_args.colors, parse_args.parsingLayout, parse_args.no_more_reset);
}

std::string get_and_color_percentage(const float& n1, const float& n2, parse_args_t& parse_args,
                                     const bool invert)
{
    const Config& config = parse_args.config;
    const float   result = n1 / n2 * static_cast<float>(100);

    std::string color;
    if (!invert)
    {
        if (result <= 45)
            color = "${" + config.percentage_colors.at(0) + "}";
        else if (result <= 80)
            color = "${" + config.percentage_colors.at(1) + "}";
        else
            color = "${" + config.percentage_colors.at(2) + "}";
    }
    else
{
        if (result <= 45)
            color = "${" + config.percentage_colors.at(2) + "}";
        else if (result <= 80)
            color = "${" + config.percentage_colors.at(1) + "}";
        else
            color = "${" + config.percentage_colors.at(0) + "}";
    }

    return parse(fmt::format("{}{:.2f}%${{0}}", color, result), _, parse_args);
}

std::string getInfoFromName(const systemInfo_t& systemInfo, const std::string_view moduleName,
                            const std::string_view moduleMemberName)
{
    if (const auto& it1 = systemInfo.find(moduleName.data()); it1 != systemInfo.end())
    {
        if (const auto& it2 = it1->second.find(moduleMemberName.data()); it2 != it1->second.end())
        {
            const variant& result = it2->second;

            if (std::holds_alternative<std::string>(result))
                return std::get<std::string>(result);

            else if (std::holds_alternative<double>(result))
                return fmt::format("{:.2f}", (std::get<double>(result)));

            else
                return fmt::to_string(std::get<size_t>(result));
        }
    }

    return "(unknown/invalid module)";
}

std::string parse(Parser& parser, parse_args_t& parse_args, const bool evaluate = true, const char until = 0);

std::optional<std::string> parse_conditional_tag(Parser& parser, parse_args_t& parse_args, const bool evaluate)
{
    if (!parser.try_read('['))
        return {};

    const std::string& condA = parse(parser, parse_args, evaluate, ',');
    const std::string& condB = parse(parser, parse_args, evaluate, ',');

    const bool cond = (condA == condB);

    const std::string& condTrue  = parse(parser, parse_args, cond, ',');
    const std::string& condFalse = parse(parser, parse_args, !cond, ']');

    return cond ? condTrue : condFalse;
}

std::optional<std::string> parse_command_tag(Parser& parser, parse_args_t& parse_args, const bool evaluate)
{
    if (!parser.try_read('('))
        return {};

    std::string command = parse(parser, parse_args, evaluate, ')');

    if (!evaluate)
        return {};

    const bool removetag = (command.front() == '!');
    if (removetag)
        command.erase(0, 1);

    const std::string& cmd_output = read_shell_exec(command);
    if (!parse_args.parsingLayout && !removetag && parser.dollar_pos != std::string::npos)
        parse_args.pureOutput.replace(parser.dollar_pos, command.length() + "$()"_len, cmd_output);

    return cmd_output;
}

std::optional<std::string> parse_color_tag(Parser& parser, parse_args_t& parse_args, const bool evaluate)
{
    if (!parser.try_read('{'))
        return {};

    std::string color = parse(parser, parse_args, evaluate, '}');

    if (!evaluate)
        return {};

    std::string     output;
    const Config&   config = parse_args.config;
    const colors_t& colors = parse_args.colors;
    const size_t    taglen = color.length() + "${}"_len;

#if ANDROID_APP
    std::string& endspan = parse_args.endspan;
    output += endspan;
    endspan.clear();

    const auto& append_endspan = [&endspan](const std::string_view tag) {
        endspan += "</";
        endspan += tag;
        endspan += ">";
    };
#else
    const std::string endspan = (!parse_args.firstrun_clr ? "</span>" : "");
#endif

    if (config.m_disable_colors)
    {
        if (parser.dollar_pos != std::string::npos)
            parse_args.pureOutput.erase(parser.dollar_pos, taglen);
        return "";
    }

    // if at end there a '$', it will make the end output "$</span>" and so it will confuse
    // addValueFromModule() and so let's make it "$ </span>". this is geniunenly stupid
    if (config.gui && output.back() == '$')
        output += ' ';

    if (!config.colors_name.empty())
    {
        const auto& it_name = std::find(config.colors_name.begin(), config.colors_name.end(), color);
        if (it_name != config.colors_name.end())
        {
            const auto& it_value = std::distance(config.colors_name.begin(), it_name);

            if (hasStart(color, "auto"))
            {
                // "ehhmmm why goto and double code? that's ugly and unconvienient :nerd:"
                // I don't care, it does the work and well
                if (color == *it_name)
                    color = config.colors_value.at(it_value);
                goto jumpauto;
            }

            if (color == *it_name)
                color = config.colors_value.at(it_value);
        }
    }

    if (hasStart(color, "auto"))
    {
        int ver = color.length() > 4 ? std::stoi(color.substr(4)) - 1 : 0;
        if (ver < 1 || static_cast<size_t>(ver) >= auto_colors.size())
            ver = 0;

        if (auto_colors.empty())
            auto_colors.push_back(NOCOLOR_BOLD);

        color = auto_colors.at(ver);
    }

jumpauto:
#if !ANDROID_APP
    if (color == "1")
    {
        output += config.gui ? endspan + "<span weight='bold'>" : NOCOLOR_BOLD;
    }
    else if (color == "0")
    {
        output += config.gui ? endspan + "<span>" : NOCOLOR;
    }
    else
    {
        std::string str_clr;
        if (config.gui)
        {
            switch (fnv1a16::hash(color))
            {
                case "black"_fnv1a16:   str_clr = colors.gui_black; break;
                case "red"_fnv1a16:     str_clr = colors.gui_red; break;
                case "blue"_fnv1a16:    str_clr = colors.gui_blue; break;
                case "green"_fnv1a16:   str_clr = colors.gui_green; break;
                case "cyan"_fnv1a16:    str_clr = colors.gui_cyan; break;
                case "yellow"_fnv1a16:  str_clr = colors.gui_yellow; break;
                case "magenta"_fnv1a16: str_clr = colors.gui_magenta; break;
                case "white"_fnv1a16:   str_clr = colors.gui_white; break;
                default:                str_clr = color; break;
            }

            const size_t pos = str_clr.rfind('#');
            if (pos != std::string::npos)
            {
                std::string        tagfmt  = "span ";
                const std::string& opt_clr = str_clr.substr(0, pos);

                size_t      argmode_pos    = 0;
                const auto& append_argmode = [&](const std::string_view fmt, const std::string_view mode) -> size_t {
                    if (opt_clr.at(argmode_pos + 1) == '(')
                    {
                        const size_t closebrak = opt_clr.find(')', argmode_pos);
                        if (closebrak == std::string::npos)
                            die(_("'{}' mode in color '{}' doesn't have close bracket"), mode, str_clr);

                        const std::string& value = opt_clr.substr(argmode_pos + 2, closebrak - argmode_pos - 2);
                        tagfmt += fmt.data() + value + "' ";

                        return closebrak;
                    }
                    return 0;
                };

                bool bgcolor = false;
                for (size_t i = 0; i < opt_clr.length(); ++i)
                {
                    switch (opt_clr.at(i))
                    {
                        case 'b':
                            bgcolor = true;
                            tagfmt += "bgcolor='" + str_clr.substr(pos) + "' ";
                            break;
                        case '!': tagfmt += "weight='bold' "; break;
                        case 'u': tagfmt += "underline='single' "; break;
                        case 'i': tagfmt += "style='italic' "; break;
                        case 'o': tagfmt += "overline='single' "; break;
                        case 's': tagfmt += "strikethrough='true' "; break;

                        case 'a':
                            argmode_pos = i;
                            i += append_argmode("fgalpha='", "fgalpha");
                            break;

                        case 'A':
                            argmode_pos = i;
                            i += append_argmode("bgalpha='", "bgalpha");
                            break;

                        case 'L':
                            argmode_pos = i;
                            i += append_argmode("underline='", "underline option");
                            break;

                        case 'U':
                            argmode_pos = i;
                            i += append_argmode("underline_color='", "colored underline");
                            break;

                        case 'B':
                            argmode_pos = i;
                            i += append_argmode("bgcolor='", "bgcolor");
                            break;

                        case 'w':
                            argmode_pos = i;
                            i += append_argmode("weight='", "font weight style");
                            break;

                        case 'O':
                            argmode_pos = i;
                            i += append_argmode("overline_color='", "overline color");
                            break;

                        case 'S':
                            argmode_pos = i;
                            i += append_argmode("strikethrough_color='", "color of strikethrough line");
                            break;
                    }
                }

                if (!bgcolor)
                    tagfmt += "fgcolor='" + str_clr.substr(pos) + "' ";

                tagfmt.pop_back();
                output += endspan + "<" + tagfmt + ">";
            }

            // "\\e" is for checking in the ascii_art, \033 in the config
            else if (hasStart(str_clr, "\\e") || hasStart(str_clr, "\033"))
            {
                const std::string& noesc_str = hasStart(str_clr, "\033") ? str_clr.substr(2) : str_clr.substr(3);
                debug("noesc_str = {}", noesc_str);

                if (hasStart(noesc_str, "38;2;") || hasStart(noesc_str, "48;2;"))
                {
                    const std::string& hexclr = convert_ansi_escape_rgb(noesc_str);
                    output +=
                        fmt::format("{}<span {}gcolor='#{}'>", endspan, hasStart(noesc_str, "38") ? 'f' : 'b', hexclr);
                }
                else if (hasStart(noesc_str, "38;5;") || hasStart(noesc_str, "48;5;"))
                {
                    die(_("256 true color '{}' works only in terminal"), noesc_str);
                }
                else
                {
                    const std::array<std::string, 3>& clrs   = get_ansi_color(noesc_str, colors);
                    const std::string_view            color  = clrs.at(0);
                    const std::string_view            weight = clrs.at(1);
                    const std::string_view            type   = clrs.at(2);
                    output += fmt::format("{}<span {}='{}' weight='{}'>", endspan, type, color, weight);
                }
            }

            else
            {
                error(_("PARSER: failed to parse line with color '{}'"), str_clr);
                if (!parse_args.parsingLayout && parser.dollar_pos != std::string::npos)
                    parse_args.pureOutput.erase(parser.dollar_pos, taglen);
                return output;
            }
        }
        // if (!config.gui)
        else
        {
            switch (fnv1a16::hash(color))
            {
                case "black"_fnv1a16:   str_clr = colors.black; break;
                case "red"_fnv1a16:     str_clr = colors.red; break;
                case "blue"_fnv1a16:    str_clr = colors.blue; break;
                case "green"_fnv1a16:   str_clr = colors.green; break;
                case "cyan"_fnv1a16:    str_clr = colors.cyan; break;
                case "yellow"_fnv1a16:  str_clr = colors.yellow; break;
                case "magenta"_fnv1a16: str_clr = colors.magenta; break;
                case "white"_fnv1a16:   str_clr = colors.white; break;
                default:                str_clr = color; break;
            }

            const size_t pos = str_clr.rfind('#');
            if (pos != std::string::npos)
            {
                const std::string& opt_clr = str_clr.substr(0, pos);

                fmt::text_style style;

                const auto& skip_gui_argmode = [&opt_clr](const size_t index) -> size_t {
                    if (opt_clr.at(index + 1) == '(')
                    {
                        const size_t closebrak = opt_clr.find(')', index);
                        if (closebrak == std::string::npos)
                            return 0;

                        return closebrak;
                    }
                    return 0;
                };

                bool bgcolor = false;
                for (size_t i = 0; i < opt_clr.length(); ++i)
                {
                    switch (opt_clr.at(i))
                    {
                        case 'b':
                            bgcolor = true;
                            append_styles(style, fmt::bg(hexStringToColor(str_clr.substr(pos))));
                            break;
                        case '!': append_styles(style, fmt::emphasis::bold); break;
                        case 'u': append_styles(style, fmt::emphasis::underline); break;
                        case 'i': append_styles(style, fmt::emphasis::italic); break;
                        case 'l': append_styles(style, fmt::emphasis::blink); break;
                        case 's': append_styles(style, fmt::emphasis::strikethrough); break;

                        case 'U':
                        case 'B':
                        case 'S':
                        case 'a':
                        case 'w':
                        case 'O':
                        case 'A':
                        case 'L': i += skip_gui_argmode(i); break;
                    }
                }

                if (!bgcolor)
                    append_styles(style, fmt::fg(hexStringToColor(str_clr.substr(pos))));

                // you can't fmt::format(style, ""); ughh
                if (style.has_emphasis())
                {
                    fmt::detail::ansi_color_escape<char> emph(style.get_emphasis());
                    output += emph.begin();
                }
                if (style.has_background() || style.has_foreground())
                {
                    const uint32_t rgb_num = bgcolor ? style.get_background().value.rgb_color : style.get_foreground().value.rgb_color;
                    fmt::rgb rgb(rgb_num);
                    fmt::detail::ansi_color_escape<char> ansi(rgb, bgcolor ? "\x1B[48;2;" : "\x1B[38;2;");
                    output += ansi.begin();
                }
            }

            // "\\e" is for checking in the ascii_art, \033 in the config
            else if (hasStart(str_clr, "\\e") || hasStart(str_clr, "\033"))
            {
                output += "\033[";
                output += hasStart(str_clr, "\033") ? str_clr.substr(2) : str_clr.substr(3);
            }

            else
            {
                error(_("PARSER: failed to parse line with color '{}'"), str_clr);
                if (!parse_args.parsingLayout && parser.dollar_pos != std::string::npos)
                    parse_args.pureOutput.erase(parser.dollar_pos, taglen);
                return output;
            }
        }
        if (!parse_args.parsingLayout &&
            std::find(auto_colors.begin(), auto_colors.end(), color) == auto_colors.end())
            auto_colors.push_back(color);
    }
#else
    if (color == "1")
    {
        output += "<b>";
        append_endspan("b");
    }
    else if (color == "0")
    {
        output += "<span>";
        append_endspan("span");
    }
    else
    {
        std::string str_clr;
        switch (fnv1a16::hash(color))
        {
            case "black"_fnv1a16:   str_clr = colors.gui_black; break;
            case "red"_fnv1a16:     str_clr = colors.gui_red; break;
            case "blue"_fnv1a16:    str_clr = colors.gui_blue; break;
            case "green"_fnv1a16:   str_clr = colors.gui_green; break;
            case "cyan"_fnv1a16:    str_clr = colors.gui_cyan; break;
            case "yellow"_fnv1a16:  str_clr = colors.gui_yellow; break;
            case "magenta"_fnv1a16: str_clr = colors.gui_magenta; break;
            case "white"_fnv1a16:   str_clr = colors.gui_white; break;
            default:                str_clr = color; break;
        }

        const size_t pos = str_clr.rfind('#');
        if (pos != std::string::npos)
        {
            std::string        tagfmt;
            const std::string& opt_clr = str_clr.substr(0, pos);
            const std::string& hexclr  = str_clr.substr(pos + 1);

            size_t argmode_pos = 0;
            /*const auto& get_argmode_value = [&](const std::string_view error) -> std::string {
                if (opt_clr.at(argmode_pos + 1) == '(')
                {
                    const size_t closebrak = opt_clr.find(')', argmode_pos);
                    if (closebrak == std::string::npos)
                        die(_("{} mode in tag color {} doesn't have close bracket"), error, str_clr);

                    const std::string& value = opt_clr.substr(argmode_pos + 2, closebrak - argmode_pos - 2);
                    return value;
                }
                return "";
            };*/

            const auto& append_argmode = [&](const std::string_view fmt, const std::string_view error) -> size_t {
                if (opt_clr.at(argmode_pos + 1) == '(')
                {
                    const size_t closebrak = opt_clr.find(')', argmode_pos);
                    if (closebrak == std::string::npos)
                        die(_("{} mode in tag color {} doesn't have close bracket"), error, str_clr);

                    const std::string& value = opt_clr.substr(argmode_pos + 2, closebrak - argmode_pos - 2);
                    tagfmt += fmt.data() + value + ";\">";

                    return closebrak;
                }
                return 0;
            };

            const auto& skip_gui_argmode = [&](const size_t index) -> size_t {
                if (opt_clr.at(index + 1) == '(')
                {
                    const size_t closebrak = opt_clr.find(')', index);
                    if (closebrak == std::string::npos)
                        return 0;

                    return closebrak;
                }
                return 0;
            };

            bool bgcolor = false;
            for (size_t i = 0; i < opt_clr.length(); ++i)
            {
                switch (opt_clr.at(i))
                {
                    case 'b':
                        bgcolor = true;
                        tagfmt += "<span style=\"background-color:" + str_clr.substr(pos) + ";\">";
                        break;
                    case '!': tagfmt += "<b>"; append_endspan("b"); break;
                    case 'u': tagfmt += "<u>"; append_endspan("u"); break;
                    case 'i': tagfmt += "<i>"; append_endspan("i"); break;
                    case 's': tagfmt += "<s>"; append_endspan("s"); break;

                    case 'B':
                        argmode_pos = i;
                        i += append_argmode("<span style=\"background-color:", "bgcolor");
                        append_endspan("font");
                        break;

                    case 'S':
                        argmode_pos = i;
                        i += append_argmode("<span style=\"color:", "color of strikethrough line");
                        tagfmt += "<s>";
                        append_endspan("span");
                        append_endspan("s");
                        break;

                    case 'a':
                    case 'A':
                    // alpha setting doesn't seem to work correctly no matter what. disabling for now.
                    // if someone could help I would really really appreciate it
                    /*{
                        argmode_pos = i;
                        std::string alpha_s = get_argmode_value("f/bgcolor alpha");
                        std::uint16_t alpha = 0;
                        const size_t perc_pos = alpha_s.find('%');
                        if (perc_pos != alpha_s.npos)
                        {
                            alpha_s.erase(perc_pos);
                            alpha = (std::stoi(alpha_s) * 0xf / 100);
                        }
                        else
                        {
                            alpha = std::stoi(alpha_s);
                        }

                        std::stringstream ss;
                        ss << std::hex << alpha;
                        if (hexclr.length() <= 6)
                            str_clr.insert(pos + 1, ss.str());
                        else
                            str_clr.replace(pos + 1, 2, ss.str());

                        i += skip_gui_argmode(i);
                        break;
                    }*/

                    case 'L':
                    case 'U':
                    case 'w':
                    case 'O':
                    case 'o': i += skip_gui_argmode(i);
                }
            }

            if (!bgcolor)
                tagfmt += "<span style=\"color:" + str_clr.substr(pos) + ";\">";

            append_endspan("span");
            output += tagfmt;
        }

        // "\\e" is for checking in the ascii_art, \033 in the config
        else if (hasStart(str_clr, "\\e") || hasStart(str_clr, "\033"))
        {
            const std::string& noesc_str = hasStart(str_clr, "\033") ? str_clr.substr(2) : str_clr.substr(3);
            debug("noesc_str = {}", noesc_str);

            if (hasStart(noesc_str, "38;2;") || hasStart(noesc_str, "48;2;"))
            {
                const std::string& hexclr = convert_ansi_escape_rgb(noesc_str);
                output += fmt::format("<span style=\"{}color:#{};\">", hasStart(noesc_str, "48") ? "background-" : "",
                                      hexclr);
            }
            else if (hasStart(noesc_str, "38;5;") || hasStart(noesc_str, "48;5;"))
            {
                die(_("256 true color '{}' works only in terminal"), noesc_str);
            }
            else
            {
                const std::array<std::string, 3>& clrs   = get_ansi_color(noesc_str, colors);
                const std::string_view            color  = clrs.at(0);
                const std::string_view            weight = clrs.at(1);
                const std::string_view            type   = clrs.at(2);
                output += fmt::format("{}<span style=\"{}:{};\">", weight, type, color);

                if (weight == "<b>")
                    append_endspan("b");
            }
            append_endspan("span");
        }

        else
        {
            error(_("PARSER: failed to parse line with color '{}'"), str_clr);
            if (!parse_args.parsingLayout && parser.dollar_pos != std::string::npos)
                parse_args.pureOutput.erase(parser.dollar_pos, taglen);
            return output;
        }

        if (!parse_args.parsingLayout &&
            std::find(auto_colors.begin(), auto_colors.end(), color) == auto_colors.end())
            auto_colors.push_back(color);
    }
#endif  // !ANDROID_APP

    if (!parse_args.parsingLayout && parser.dollar_pos != std::string::npos)
        parse_args.pureOutput.erase(parser.dollar_pos, taglen);

    parse_args.firstrun_clr = false;

    return output;
}

std::optional<std::string> parse_info_tag(Parser& parser, parse_args_t& parse_args, const bool evaluate)
{
    if (!parser.try_read('<'))
        return {};

    const std::string& module = parse(parser, parse_args, evaluate, '>');

    if (!evaluate)
        return {};

    const size_t dot_pos = module.find('.');
    if (dot_pos == module.npos)
    {
        addValueFromModule(module, parse_args);
        const std::string& info = getInfoFromName(parse_args.systemInfo, module, "module-" + module);

        if (parser.dollar_pos != std::string::npos)
            parse_args.pureOutput.replace(parser.dollar_pos, module.length() + "$<>"_len, info);

        return info;
    }

    const std::string& moduleName       = module.substr(0, dot_pos);
    const std::string& moduleMemberName = module.substr(dot_pos + 1);
    addValueFromModuleMember(moduleName, moduleMemberName, parse_args);

    const std::string& info = getInfoFromName(parse_args.systemInfo, moduleName, moduleMemberName);

    if (parser.dollar_pos != std::string::npos)
        parse_args.pureOutput.replace(parser.dollar_pos, module.length() + "$<>"_len, info);
    return info;
}

std::optional<std::string> parse_perc_tag(Parser& parser, parse_args_t& parse_args, const bool evaluate)
{
    if (!parser.try_read('%'))
        return {};

    const std::string& command = parse(parser, parse_args, evaluate, '%');

    if (!evaluate)
        return {};

    const size_t comma_pos = command.find(',');
    if (comma_pos == std::string::npos)
        die(_("percentage tag '{}' doesn't have a comma for separating the 2 numbers"), command);

    const bool invert = (command.front() == '!');

    const float n1 = std::stof(parse(command.substr(invert ? 1 : 0, comma_pos), _, parse_args));
    const float n2 = std::stof(parse(command.substr(comma_pos + 1), _, parse_args));

    return get_and_color_percentage(n1, n2, parse_args, invert);
}

std::optional<std::string> parse_tags(Parser& parser, parse_args_t& parse_args, const bool evaluate)
{
    if (!parser.try_read('$'))
        return {};

    if (parser.dollar_pos != std::string::npos)
        parser.dollar_pos = parser.pureOutput.find('$', parser.dollar_pos);

    if (const auto& color_tag = parse_color_tag(parser, parse_args, evaluate))
        return color_tag;

    if (const auto& module_tag = parse_info_tag(parser, parse_args, evaluate))
        return module_tag;

    if (const auto& command_tag = parse_command_tag(parser, parse_args, evaluate))
        return command_tag;

    if (const auto& ifTag = parse_conditional_tag(parser, parse_args, evaluate))
        return ifTag;

    if (const auto& perc_tag = parse_perc_tag(parser, parse_args, evaluate))
        return perc_tag;

    parser.rewind();
    return {};
}

std::string parse(Parser& parser, parse_args_t& parse_args, const bool evaluate, const char until)
{
    std::string result;

    while (until == 0 ? !parser.is_eof() : !parser.try_read(until))
    {
        if (until != 0 && parser.is_eof())
        {
            error(_("PARSER: Missing tag close bracket {} in string '{}'"), until, parser.src);
            return result;
        }

        if (parser.try_read('\\'))
        {
            result += parser.read_char(until == 0);
        }
        else if (const auto& tagStr = parse_tags(parser, parse_args, evaluate))
        {
            result += *tagStr;
        }
        else
        {
            result += parser.read_char(until == 0);
        }
    }

    return result;
}

std::string parse(std::string input, systemInfo_t& systemInfo, std::string& pureOutput, std::vector<std::string>& layout,
                  std::vector<std::string>& tmp_layout, const Config& config, const colors_t& colors, const bool parsingLayout, bool& no_more_reset)
{
    if (!config.sep_reset.empty() && parsingLayout && !no_more_reset)
    {
        if (config.sep_reset_after)
            replace_str(input, config.sep_reset, config.sep_reset + "${0}");
        else
            replace_str(input, config.sep_reset, "${0}" + config.sep_reset);

        no_more_reset = true;
    }

    // escape pango markup
    // https://gitlab.gnome.org/GNOME/glib/-/blob/main/glib/gmarkup.c#L2150
    // workaround: just put "\<" or "\&" in the config, e.g "$<os.kernel> \<- Kernel"
    if (config.gui)
    {
        replace_str(input, "\\<", "&lt;");
        replace_str(input, "\\&", "&amp;");
    }
    else
    {
        replace_str(input, "\\<", "<");
        replace_str(input, "\\&", "&");
    }
#if ANDROID_APP
    if (!parsingLayout)
        replace_str(input, " ", "&nbsp;");
#endif

    parse_args_t parse_args{ systemInfo, pureOutput, layout, tmp_layout, config, colors, parsingLayout, true, no_more_reset, "" };
    Parser       parser{ input, pureOutput };

    std::string ret{ parse(parser, parse_args) };

#if ANDROID_APP
    if (config.gui && !parse_args.firstrun_clr)
        ret += parse_args.endspan;
#else
    if (config.gui && !parse_args.firstrun_clr)
        ret += "</span>";
#endif

    replace_str(parse_args.pureOutput, "&nbsp;", " ");

    return ret;
}

static std::string get_auto_uptime(const std::uint16_t days, const std::uint16_t hours, const std::uint16_t mins,
                                   const std::uint16_t secs, const Config& config)
{
    if (days == 0 && hours == 0 && mins == 0)
        return fmt::format("{}{}", secs, config.uptime_s_fmt);

    std::string ret;

    if (days > 0)
        ret += fmt::format("{}{}, ", days, config.uptime_d_fmt);

    if (hours > 0)
        ret += fmt::format("{}{}, ", hours, config.uptime_h_fmt);

    if (mins > 0)
        ret += fmt::format("{}{}, ", mins, config.uptime_m_fmt);

    ret.erase(ret.length() - 2);  // the last ", "

    return ret;
}

static std::string get_auto_gtk_format(const std::string_view gtk2, const std::string_view gtk3,
                                       const std::string_view gtk4)
{
    if ((gtk2 != MAGIC_LINE && gtk3 != MAGIC_LINE && gtk4 != MAGIC_LINE))
    {
        if (gtk2 == gtk3 && gtk2 == gtk4)
            return fmt::format("{} [GTK2/3/4]", gtk4);
        else if (gtk2 == gtk3)
            return fmt::format("{} [GTK2/3], {} [GTK4]", gtk2, gtk4);
        else if (gtk4 == gtk3)
            return fmt::format("{} [GTK2], {} [GTK3/4]", gtk2, gtk4);
        else
            return fmt::format("{} [GTK2], {} [GTK3], {} [GTK4]", gtk2, gtk3, gtk4);
    }

    else if (gtk3 != MAGIC_LINE && gtk4 != MAGIC_LINE)
    {
        if (gtk3 == gtk4)
            return fmt::format("{} [GTK3/4]", gtk4);
        else
            return fmt::format("{} [GTK3], {} [GTK4]", gtk3, gtk4);
    }

    else if (gtk2 != MAGIC_LINE && gtk3 != MAGIC_LINE)
    {
        if (gtk2 == gtk3)
            return fmt::format("{} [GTK2/3]", gtk3);
        else
            return fmt::format("{} [GTK2], {} [GTK3]", gtk2, gtk3);
    }

    else if (gtk4 != MAGIC_LINE)
        return fmt::format("{} [GTK4]", gtk4);
    else if (gtk3 != MAGIC_LINE)
        return fmt::format("{} [GTK3]", gtk3);
    else if (gtk2 != MAGIC_LINE)
        return fmt::format("{} [GTK2]", gtk2);

    return MAGIC_LINE;
}

static std::string prettify_term_name(const std::string_view term_name)
{
    switch (fnv1a16::hash(str_tolower(term_name.data())))
    {
        case "gnome-terminal"_fnv1a16:
        case "gnome terminal"_fnv1a16: return "GNOME Terminal";

        case "gnome-console"_fnv1a16:
        case "gnome console"_fnv1a16: return "GNOME console";
    }
    return term_name.data();
}

static std::string prettify_de_name(const std::string_view de_name)
{
    switch (fnv1a16::hash(str_tolower(de_name.data())))
    {
        case "kde"_fnv1a16:
        case "plasma"_fnv1a16:
        case "plasmashell"_fnv1a16:
        case "plasmawayland"_fnv1a16: return "KDE Plasma";

        case "gnome"_fnv1a16:
        case "gnome-shell"_fnv1a16: return "GNOME";

        case "xfce"_fnv1a16:
        case "xfce4"_fnv1a16:
        case "xfce4-session"_fnv1a16: return "Xfce4";

        case "mate"_fnv1a16:
        case "mate-session"_fnv1a16: return "Mate";

        case "lxqt"_fnv1a16:
        case "lxqt-session"_fnv1a16: return "LXQt";
    }

    return de_name.data();
}

systemInfo_t queried_gpus;
systemInfo_t queried_disks;
systemInfo_t queried_themes_names;
systemInfo_t queried_themes;

// clang-format on
void addValueFromModuleMember(const std::string& moduleName, const std::string& moduleMemberName,
                              parse_args_t& parse_args)
{
#define SYSINFO_INSERT(x) sysInfo.at(moduleName).insert({ moduleMemberName, variant(x) })

    // just aliases for convention
    const Config& config  = parse_args.config;
    systemInfo_t& sysInfo = parse_args.systemInfo;

    const auto&                                moduleMember_hash     = fnv1a16::hash(moduleMemberName);
    const std::uint16_t                        byte_unit             = config.use_SI_unit ? 1000 : 1024;
    constexpr std::array<std::string_view, 32> sorted_valid_prefixes = { "B",   "EB", "EiB", "GB", "GiB", "kB",
                                                                         "KiB", "MB", "MiB", "PB", "PiB", "TB",
                                                                         "TiB", "YB", "YiB", "ZB", "ZiB" };

    const auto& return_devided_bytes = [&sorted_valid_prefixes, &moduleMemberName](const double& amount) -> double {
        const std::string& prefix = moduleMemberName.substr(moduleMemberName.find('-') + 1);
        if (std::binary_search(sorted_valid_prefixes.begin(), sorted_valid_prefixes.end(), prefix))
            return devide_bytes(amount, prefix).num_bytes;

        return 0;
    };

    if (moduleName == "os")
    {
        Query::System query_system;

        const std::chrono::seconds  uptime_secs(query_system.uptime());
        const std::chrono::minutes& uptime_mins  = std::chrono::duration_cast<std::chrono::minutes>(uptime_secs);
        const std::chrono::hours&   uptime_hours = std::chrono::duration_cast<std::chrono::hours>(uptime_secs);

        // let's support a little of C++17 without any `#if __cpluscplus` stuff
        const std::uint16_t uptime_days = uptime_secs.count() / (60 * 60 * 24);

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo.at(moduleName).find(moduleMemberName) == sysInfo.at(moduleName).end())
        {
            switch (moduleMember_hash)
            {
                case "name"_fnv1a16: SYSINFO_INSERT(query_system.os_pretty_name()); break;
                case "name_id"_fnv1a16: SYSINFO_INSERT(query_system.os_id()); break;

                case "uptime"_fnv1a16:
                    SYSINFO_INSERT(get_auto_uptime(uptime_days, uptime_hours.count() % 24, uptime_mins.count() % 60,
                                                   uptime_secs.count() % 60, config));
                    break;

                case "uptime_secs"_fnv1a16: SYSINFO_INSERT(static_cast<size_t>(uptime_secs.count() % 60)); break;
                case "uptime_mins"_fnv1a16: SYSINFO_INSERT(static_cast<size_t>(uptime_mins.count() % 60)); break;
                case "uptime_hours"_fnv1a16: SYSINFO_INSERT(static_cast<size_t>(uptime_hours.count()) % 24); break;
                case "uptime_days"_fnv1a16: SYSINFO_INSERT(static_cast<size_t>(uptime_days)); break;

                case "kernel"_fnv1a16:
                    SYSINFO_INSERT(query_system.kernel_name() + ' ' + query_system.kernel_version());
                    break;

                case "kernel_name"_fnv1a16: SYSINFO_INSERT(query_system.kernel_name()); break;
                case "kernel_version"_fnv1a16: SYSINFO_INSERT(query_system.kernel_version()); break;
                case "pkgs"_fnv1a16: SYSINFO_INSERT(query_system.pkgs_installed(config)); break;
                case "initsys_name"_fnv1a16: SYSINFO_INSERT(query_system.os_initsys_name()); break;
                case "initsys_version"_fnv1a16: SYSINFO_INSERT(query_system.os_initsys_version()); break;
                case "hostname"_fnv1a16: SYSINFO_INSERT(query_system.hostname()); break;
                case "version_codename"_fnv1a16: SYSINFO_INSERT(query_system.os_version_codename()); break;
                case "version_id"_fnv1a16: SYSINFO_INSERT(query_system.os_versionid()); break;
            }
        }
    }

    else if (moduleName == "system")
    {
        Query::System query_system;

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo.at(moduleName).find(moduleMemberName) == sysInfo.at(moduleName).end())
        {
            switch (moduleMember_hash)
            {
                case "host"_fnv1a16:
                    SYSINFO_INSERT(query_system.host_vendor() + ' ' + query_system.host_modelname() + ' ' +
                                   query_system.host_version());
                    break;

                case "host_name"_fnv1a16: SYSINFO_INSERT(query_system.host_modelname()); break;
                case "host_vendor"_fnv1a16: SYSINFO_INSERT(query_system.host_vendor()); break;
                case "host_version"_fnv1a16: SYSINFO_INSERT(query_system.host_version()); break;
                case "arch"_fnv1a16: SYSINFO_INSERT(query_system.arch()); break;
            }
        }
    }

    // clang-format on
    else if (moduleName == "user")
    {
        Query::User query_user;

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo.at(moduleName).find(moduleMemberName) == sysInfo.at(moduleName).end())
        {
            switch (moduleMember_hash)
            {
                case "name"_fnv1a16: SYSINFO_INSERT(query_user.name()); break;

                case "shell"_fnv1a16:
                    SYSINFO_INSERT(query_user.shell_name() + ' ' + query_user.shell_version(query_user.shell_name()));
                    break;

                case "shell_name"_fnv1a16: SYSINFO_INSERT(query_user.shell_name()); break;
                case "shell_path"_fnv1a16: SYSINFO_INSERT(query_user.shell_path()); break;
                case "shell_version"_fnv1a16: SYSINFO_INSERT(query_user.shell_version(query_user.shell_name())); break;

                case "de_name"_fnv1a16:
                    SYSINFO_INSERT(prettify_de_name(
                        query_user.de_name(query_user.m_bDont_query_dewm, query_user.term_name(),
                                           query_user.wm_name(query_user.m_bDont_query_dewm, query_user.term_name()))));
                    break;

                case "de_version"_fnv1a16:
                    SYSINFO_INSERT(query_user.de_version(
                        query_user.de_name(query_user.m_bDont_query_dewm, query_user.term_name(),
                                           query_user.wm_name(query_user.m_bDont_query_dewm, query_user.term_name()))));
                    break;

                case "wm_name"_fnv1a16:
                    SYSINFO_INSERT(query_user.wm_name(query_user.m_bDont_query_dewm, query_user.term_name()));
                    break;

                case "wm_version"_fnv1a16:
                    SYSINFO_INSERT(query_user.wm_version(query_user.m_bDont_query_dewm, query_user.term_name()));
                    break;

                case "terminal"_fnv1a16:
                    SYSINFO_INSERT(prettify_term_name(query_user.term_name()) + ' ' +
                                   query_user.term_version(query_user.term_name()));
                    break;

                case "terminal_name"_fnv1a16: SYSINFO_INSERT(prettify_term_name(query_user.term_name())); break;
                case "terminal_version"_fnv1a16: SYSINFO_INSERT(query_user.term_version(query_user.term_name())); break;
            }
        }
    }

    else if (moduleName == "theme")
    {
        Query::Theme query_theme(queried_themes, config, false);

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo.at(moduleName).find(moduleMemberName) == sysInfo.at(moduleName).end())
        {
            switch (moduleMember_hash)
            {
                case "cursor"_fnv1a16:
                    if (query_theme.cursor_size() == UNKNOWN)
                        SYSINFO_INSERT(query_theme.cursor());
                    else
                        SYSINFO_INSERT(fmt::format("{} ({}px)", query_theme.cursor(), query_theme.cursor_size()));
                    break;

                case "cursor_name"_fnv1a16: SYSINFO_INSERT(query_theme.cursor()); break;
                case "cursor_size"_fnv1a16: SYSINFO_INSERT(query_theme.cursor_size()); break;
            }
        }
    }

    else if (moduleName == "theme-gsettings")
    {
        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo.at(moduleName).find(moduleMemberName) == sysInfo.at(moduleName).end())
        {
            if (hasStart(moduleMemberName, "cursor"))
            {
                Query::Theme query_cursor(queried_themes, config, true);
                switch (moduleMember_hash)
                {
                    case "cursor"_fnv1a16:
                        if (query_cursor.cursor_size() == UNKNOWN)
                            SYSINFO_INSERT(query_cursor.cursor());
                        else
                            SYSINFO_INSERT(fmt::format("{} ({}px)", query_cursor.cursor(), query_cursor.cursor_size()));
                        break;
                    case "cursor_name"_fnv1a16: SYSINFO_INSERT(query_cursor.cursor()); break;
                    case "cursor_size"_fnv1a16: SYSINFO_INSERT(query_cursor.cursor_size()); break;
                }
            }
            else
            {
                Query::Theme query_theme(0, queried_themes, "gsettings", config, true);
                switch (moduleMember_hash)
                {
                    case "name"_fnv1a16:  SYSINFO_INSERT(query_theme.gtk_theme()); break;
                    case "icons"_fnv1a16: SYSINFO_INSERT(query_theme.gtk_icon_theme()); break;
                    case "font"_fnv1a16:  SYSINFO_INSERT(query_theme.gtk_font()); break;
                }
            }
        }
    }

    // clang-format off
    else if (moduleName == "theme-gtk-all")
    {
        Query::Theme gtk2(2, queried_themes, "gtk2", config);
        Query::Theme gtk3(3, queried_themes, "gtk3", config);
        Query::Theme gtk4(4, queried_themes, "gtk4", config);
        
        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo.at(moduleName).find(moduleMemberName) == sysInfo.at(moduleName).end())
        {
            switch (moduleMember_hash)
            {
                case "name"_fnv1a16:  SYSINFO_INSERT(get_auto_gtk_format(gtk2.gtk_theme(),      gtk3.gtk_theme(),      gtk4.gtk_theme())); break;
                case "icons"_fnv1a16: SYSINFO_INSERT(get_auto_gtk_format(gtk2.gtk_icon_theme(), gtk3.gtk_icon_theme(), gtk4.gtk_icon_theme())); break;
                case "font"_fnv1a16:  SYSINFO_INSERT(get_auto_gtk_format(gtk2.gtk_font(),       gtk3.gtk_font(),       gtk4.gtk_font())); break;
            }
        }
    }

    else if (hasStart(moduleName, "theme-gtk"))
    {
        const std::uint8_t ver =
            static_cast<std::uint8_t>(moduleName.length() > 9 ? std::stoi(moduleName.substr(9)) : 0);

        if (ver <= 0)
            die(_("seems theme-gtk module name '{}' doesn't have a version number to query.\n"
                  "Syntax should be like 'theme_gtkN' which N stands for the version of gtk to query (single number)"),
                moduleName);

        Query::Theme query_theme(ver, queried_themes, fmt::format("gtk{}", ver), config);

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo.at(moduleName).find(moduleMemberName) == sysInfo.at(moduleName).end())
        {
            switch (moduleMember_hash)
            {
                case "name"_fnv1a16:  SYSINFO_INSERT(query_theme.gtk_theme()); break;
                case "icons"_fnv1a16: SYSINFO_INSERT(query_theme.gtk_icon_theme()); break;
                case "font"_fnv1a16:  SYSINFO_INSERT(query_theme.gtk_font()); break;
            }
        }
    }

    // clang-format on
    else if (moduleName == "cpu")
    {
        Query::CPU query_cpu;

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo.at(moduleName).find(moduleMemberName) == sysInfo.at(moduleName).end())
        {
            switch (moduleMember_hash)
            {
                case "name"_fnv1a16:            SYSINFO_INSERT(query_cpu.name()); break;
                case "nproc"_fnv1a16:           SYSINFO_INSERT(query_cpu.nproc()); break;
                case "freq_cur"_fnv1a16:        SYSINFO_INSERT(query_cpu.freq_cur()); break;
                case "freq_max"_fnv1a16:        SYSINFO_INSERT(query_cpu.freq_max()); break;
                case "freq_min"_fnv1a16:        SYSINFO_INSERT(query_cpu.freq_min()); break;
                case "freq_bios_limit"_fnv1a16: SYSINFO_INSERT(query_cpu.freq_bios_limit()); break;

                case "temp_C"_fnv1a16: SYSINFO_INSERT(query_cpu.temp()); break;
                case "temp_F"_fnv1a16: SYSINFO_INSERT(query_cpu.temp() * 1.8 + 34); break;
                case "temp_K"_fnv1a16: SYSINFO_INSERT(query_cpu.temp() + 273.15); break;
            }
        }
    }

    else if (hasStart(moduleName, "gpu"))
    {
        const std::string& id = moduleName.length() > 3 ? moduleName.substr(3) : "0";

        Query::GPU query_gpu(id, queried_gpus);

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo.at(moduleName).find(moduleMemberName) == sysInfo.at(moduleName).end())
        {
            switch (moduleMember_hash)
            {
                case "name"_fnv1a16:        SYSINFO_INSERT(query_gpu.name()); break;
                case "vendor"_fnv1a16:      SYSINFO_INSERT(shorten_vendor_name(query_gpu.vendor())); break;
                case "vendor_long"_fnv1a16: SYSINFO_INSERT(query_gpu.vendor()); break;
            }
        }
    }

    else if (hasStart(moduleName, "disk"))
    {
        if (moduleName.length() < "disk()"_len)
            die(_("invalid disk module name '{}', must be disk(/path/to/fs) e.g: disk(/)"), moduleName);

        enum
        {
            USED = 0,
            TOTAL,
            FREE
        };
        std::string path{ moduleName.data() };
        path.erase(0, 5);  // disk(
        path.pop_back();   // )
        debug("disk path = {}", path);

        Query::Disk                 query_disk(path, queried_disks, parse_args);
        std::array<byte_units_t, 3> byte_units;

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo.at(moduleName).find(moduleMemberName) == sysInfo.at(moduleName).end())
        {
            byte_units.at(TOTAL) = auto_devide_bytes(query_disk.total_amount(), byte_unit);
            byte_units.at(USED)  = auto_devide_bytes(query_disk.used_amount(), byte_unit);
            byte_units.at(FREE)  = auto_devide_bytes(query_disk.free_amount(), byte_unit);

            switch (moduleMember_hash)
            {
                case "fs"_fnv1a16:       SYSINFO_INSERT(query_disk.typefs()); break;
                case "device"_fnv1a16:   SYSINFO_INSERT(query_disk.device()); break;
                case "mountdir"_fnv1a16: SYSINFO_INSERT(query_disk.mountdir()); break;

                case "used"_fnv1a16:
                    SYSINFO_INSERT(fmt::format("{:.2f} {}", byte_units.at(USED).num_bytes, byte_units.at(USED).unit));
                    break;

                case "total"_fnv1a16:
                    SYSINFO_INSERT(fmt::format("{:.2f} {}", byte_units.at(TOTAL).num_bytes, byte_units.at(TOTAL).unit));
                    break;

                case "free"_fnv1a16:
                    SYSINFO_INSERT(fmt::format("{:.2f} {}", byte_units.at(FREE).num_bytes, byte_units.at(FREE).unit));
                    break;

                case "free_perc"_fnv1a16:
                    SYSINFO_INSERT(get_and_color_percentage(query_disk.free_amount(), query_disk.total_amount(),
                                                            parse_args, true));
                    break;

                case "used_perc"_fnv1a16:
                    SYSINFO_INSERT(
                        get_and_color_percentage(query_disk.used_amount(), query_disk.total_amount(), parse_args));
                    break;

                default:
                    if (hasStart(moduleMemberName, "free-"))
                        SYSINFO_INSERT(return_devided_bytes(query_disk.free_amount()));
                    else if (hasStart(moduleMemberName, "used-"))
                        SYSINFO_INSERT(return_devided_bytes(query_disk.used_amount()));
                    else if (hasStart(moduleMemberName, "total-"))
                        SYSINFO_INSERT(return_devided_bytes(query_disk.total_amount()));
            }
        }
    }

    else if (moduleName == "swap")
    {
        Query::RAM query_ram;
        enum
        {
            USED = 0,
            TOTAL,
            FREE,
        };
        std::array<byte_units_t, 3> byte_units;

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo.at(moduleName).find(moduleMemberName) == sysInfo.at(moduleName).end())
        {
            //                                                            idk, trick the diviser
            byte_units.at(FREE)  = auto_devide_bytes(query_ram.swap_free_amount() * byte_unit, byte_unit);
            byte_units.at(USED)  = auto_devide_bytes(query_ram.swap_used_amount() * byte_unit, byte_unit);
            byte_units.at(TOTAL) = auto_devide_bytes(query_ram.swap_total_amount() * byte_unit, byte_unit);

            switch (moduleMember_hash)
            {
                case "free"_fnv1a16:
                    SYSINFO_INSERT(fmt::format("{:.2f} {}", byte_units.at(FREE).num_bytes, byte_units.at(FREE).unit));
                    break;

                case "total"_fnv1a16:
                    SYSINFO_INSERT(fmt::format("{:.2f} {}", byte_units.at(TOTAL).num_bytes, byte_units.at(TOTAL).unit));
                    break;

                case "used"_fnv1a16:
                    SYSINFO_INSERT(fmt::format("{:.2f} {}", byte_units.at(USED).num_bytes, byte_units.at(USED).unit));
                    break;

                case "free_perc"_fnv1a16:
                    SYSINFO_INSERT(get_and_color_percentage(query_ram.swap_free_amount(), query_ram.swap_total_amount(),
                                                            parse_args, true));
                    break;

                case "used_perc"_fnv1a16:
                    SYSINFO_INSERT(get_and_color_percentage(query_ram.swap_used_amount(), query_ram.swap_total_amount(),
                                                            parse_args));
                    break;

                default:
                    if (hasStart(moduleMemberName, "free-"))
                        SYSINFO_INSERT(return_devided_bytes(query_ram.swap_free_amount()));
                    else if (hasStart(moduleMemberName, "used-"))
                        SYSINFO_INSERT(return_devided_bytes(query_ram.swap_used_amount()));
                    else if (hasStart(moduleMemberName, "total-"))
                        SYSINFO_INSERT(return_devided_bytes(query_ram.swap_total_amount()));
            }
        }
    }

    else if (moduleName == "ram")
    {
        Query::RAM query_ram;
        enum
        {
            USED = 0,
            TOTAL,
            FREE,
        };
        std::array<byte_units_t, 3> byte_units;

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo.at(moduleName).find(moduleMemberName) == sysInfo.at(moduleName).end())
        {
            //                                                     idk, trick the diviser
            byte_units.at(USED)  = auto_devide_bytes(query_ram.used_amount() * byte_unit, byte_unit);
            byte_units.at(TOTAL) = auto_devide_bytes(query_ram.total_amount() * byte_unit, byte_unit);
            byte_units.at(FREE)  = auto_devide_bytes(query_ram.free_amount() * byte_unit, byte_unit);

            switch (moduleMember_hash)
            {
                case "used"_fnv1a16:
                    SYSINFO_INSERT(fmt::format("{:.2f} {}", byte_units.at(USED).num_bytes, byte_units.at(USED).unit));
                    break;

                case "total"_fnv1a16:
                    SYSINFO_INSERT(fmt::format("{:.2f} {}", byte_units.at(TOTAL).num_bytes, byte_units.at(TOTAL).unit));
                    break;

                case "free"_fnv1a16:
                    SYSINFO_INSERT(fmt::format("{:.2f} {}", byte_units.at(FREE).num_bytes, byte_units.at(FREE).unit));
                    break;

                case "free_perc"_fnv1a16:
                    SYSINFO_INSERT(
                        get_and_color_percentage(query_ram.free_amount(), query_ram.total_amount(), parse_args, true));
                    break;

                case "used_perc"_fnv1a16:
                    SYSINFO_INSERT(
                        get_and_color_percentage(query_ram.used_amount(), query_ram.total_amount(), parse_args));
                    break;

                default:
                    if (hasStart(moduleMemberName, "free-"))
                        SYSINFO_INSERT(return_devided_bytes(query_ram.free_amount()));
                    else if (hasStart(moduleMemberName, "used-"))
                        SYSINFO_INSERT(return_devided_bytes(query_ram.used_amount()));
                    else if (hasStart(moduleMemberName, "total-"))
                        SYSINFO_INSERT(return_devided_bytes(query_ram.total_amount()));
            }
        }
    }

    else if (moduleName == "battery")
    {
        Query::Battery query_battery;

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo.at(moduleName).find(moduleMemberName) == sysInfo.at(moduleName).end())
        {
            switch (moduleMember_hash)
            {
                case "perc"_fnv1a16:
                    SYSINFO_INSERT(get_and_color_percentage(query_battery.perc(), 100, parse_args, true));
                    break;

                case "vendor"_fnv1a16:
                case "manufacturer"_fnv1a16:   SYSINFO_INSERT(query_battery.vendor()); break;
                case "technology"_fnv1a16:     SYSINFO_INSERT(query_battery.technology()); break;
                case "name"_fnv1a16:           SYSINFO_INSERT(query_battery.modelname()); break;
                case "status"_fnv1a16:         SYSINFO_INSERT(query_battery.status()); break;
                case "capacity_level"_fnv1a16: SYSINFO_INSERT(query_battery.capacity_level()); break;

                case "temp_C"_fnv1a16: SYSINFO_INSERT(query_battery.temp()); break;
                case "temp_F"_fnv1a16: SYSINFO_INSERT(query_battery.temp() * 1.8 + 34); break;
                case "temp_K"_fnv1a16: SYSINFO_INSERT(query_battery.temp() + 273.15); break;
            }
        }
    }

    else if (moduleName == "auto")
    {
        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo.at(moduleName).find(moduleMemberName) == sysInfo.at(moduleName).end())
        {
            switch (moduleMember_hash)
            {
                case "disk"_fnv1a16:
                    Query::Disk query_disks("", queried_disks, parse_args, true);
                    for (const std::string& str : query_disks.disks_formats())
                    {
                        parse_args.tmp_layout.push_back(str);
                        debug("tmp_layout = {}", parse_args.tmp_layout);
                        SYSINFO_INSERT(str);
                    }
                break;
            }
        }
    }

    else
        die(_("Invalid module name: {}"), moduleName);

#undef SYSINFO_INSERT
}

void addValueFromModule(const std::string& moduleName, parse_args_t& parse_args)
{
    const std::string& moduleMemberName = "module-" + moduleName;
#define SYSINFO_INSERT(x) sysInfo.at(moduleName).insert({ moduleMemberName, variant(x) })

    // just aliases for convention
    const Config& config  = parse_args.config;
    systemInfo_t& sysInfo = parse_args.systemInfo;

    const std::uint16_t byte_unit = config.use_SI_unit ? 1000 : 1024;

    if (moduleName == "title")
    {
        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo.at(moduleName).find(moduleMemberName) == sysInfo.at(moduleName).end())
        {
            SYSINFO_INSERT(parse("${auto2}$<user.name>${0}@${auto2}$<os.hostname>", _, parse_args));
        }
    }

    else if (moduleName == "title_sep")
    {
        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo.at(moduleName).find(moduleMemberName) == sysInfo.at(moduleName).end())
        {
            // no need to parse anything
            Query::User   query_user;
            Query::System query_system;
            const size_t& title_len =
                std::string_view(query_user.name() + '@' + query_system.hostname()).length();

            std::string str;
            str.reserve(config.builtin_title_sep.length() * title_len);
            for (size_t i = 0; i < title_len; i++)
                str += config.builtin_title_sep;

            SYSINFO_INSERT(str);
        }
    }

    else if (moduleName == "cpu")
    {
        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo.at(moduleName).find(moduleMemberName) == sysInfo.at(moduleName).end())
        {
            Query::CPU query_cpu;
            SYSINFO_INSERT(
                fmt::format("{} ({}) @ {:.2f} GHz", query_cpu.name(), query_cpu.nproc(), query_cpu.freq_max()));
        }
    }

    else if (hasStart(moduleName, "gpu"))
    {
        const std::string& id = (moduleName.length() > 3 ? moduleName.substr(3) : "0");

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo.at(moduleName).find(moduleMemberName) == sysInfo.at(moduleName).end())
        {
            Query::GPU query_gpu(id, queried_gpus);
            SYSINFO_INSERT(shorten_vendor_name(query_gpu.vendor()) + " " + query_gpu.name());
        }
    }

    else if (hasStart(moduleName, "disk"))
    {
        if (moduleName.length() < "disk()"_len)
            die(_("invalid disk module name '{}', must be disk(/path/to/fs) e.g: disk(/)"), moduleName);

        enum
        {
            USED = 0,
            TOTAL,
        };
        std::string path = moduleName;
        path.erase(0, 5);  // disk(
        path.pop_back();   // )
        debug("disk path = {}", path);

        Query::Disk                 query_disk(path, queried_disks, parse_args);
        std::array<byte_units_t, 2> byte_units;

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo.at(moduleName).find(moduleMemberName) == sysInfo.at(moduleName).end())
        {
            byte_units.at(TOTAL) = auto_devide_bytes(query_disk.total_amount(), byte_unit);
            byte_units.at(USED)  = auto_devide_bytes(query_disk.used_amount(), byte_unit);

            const std::string& perc =
                get_and_color_percentage(query_disk.used_amount(), query_disk.total_amount(), parse_args);

            // clang-format off
            SYSINFO_INSERT (fmt::format("{:.2f} {} / {:.2f} {} {} - {}", 
                            byte_units.at(USED).num_bytes, byte_units.at(USED).unit,
                            byte_units.at(TOTAL).num_bytes,byte_units.at(TOTAL).unit, 
                            parse("${0}(" + perc + ")", _, parse_args),
			    query_disk.typefs()));
            // clang-format on
        }
    }

    else if (moduleName == "ram")
    {
        Query::RAM query_ram;
        enum
        {
            USED = 0,
            TOTAL,
        };
        std::array<byte_units_t, 2> byte_units;

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo.at(moduleName).find(moduleMemberName) == sysInfo.at(moduleName).end())
        {
            //                                                            idk, trick the divider
            byte_units.at(USED)  = auto_devide_bytes(query_ram.used_amount() * byte_unit, byte_unit);
            byte_units.at(TOTAL) = auto_devide_bytes(query_ram.total_amount() * byte_unit, byte_unit);

            const std::string& perc =
                get_and_color_percentage(query_ram.used_amount(), query_ram.total_amount(), parse_args);

            // clang-format off
            SYSINFO_INSERT(fmt::format("{:.2f} {} / {:.2f} {} {}",
                                       byte_units.at(USED).num_bytes, byte_units.at(USED).unit,
                                       byte_units.at(TOTAL).num_bytes,byte_units.at(TOTAL).unit,
                                       parse("${0}(" + perc + ")", _, parse_args)));
            // clang-format on
        }
    }

    else if (moduleName == "swap")
    {
        Query::RAM query_ram;
        enum
        {
            USED = 0,
            TOTAL,
        };
        std::array<byte_units_t, 2> byte_units;

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo.at(moduleName).find(moduleMemberName) == sysInfo.at(moduleName).end())
        {
            //                                                            idk, trick the divider
            byte_units.at(USED)  = auto_devide_bytes(query_ram.swap_used_amount() * byte_unit, byte_unit);
            byte_units.at(TOTAL) = auto_devide_bytes(query_ram.swap_total_amount() * byte_unit, byte_unit);

            // clang-format off
            if (byte_units.at(TOTAL).num_bytes < 1)
                SYSINFO_INSERT("Disabled");
            else
            {
                const std::string& perc = get_and_color_percentage(query_ram.swap_used_amount(), query_ram.swap_total_amount(), 
                                                                   parse_args);
                
                SYSINFO_INSERT(fmt::format("{:.2f} {} / {:.2f} {} {}",
                                            byte_units.at(USED).num_bytes, byte_units.at(USED).unit,
                                            byte_units.at(TOTAL).num_bytes,byte_units.at(TOTAL).unit,
                                            parse("${0}(" + perc + ")", _, parse_args)));
            }
            // clang-format on
        }
    }

    else if (moduleName == "battery")
    {
        Query::Battery query_battery;
        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo.at(moduleName).find(moduleMemberName) == sysInfo.at(moduleName).end())
        {
            SYSINFO_INSERT(fmt::format("{} [{}]", get_and_color_percentage(query_battery.perc(), 100, parse_args, true),
                                       query_battery.status()));
        }
    }

    else if (moduleName == "colors")
    {
        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo.at(moduleName).find(moduleMemberName) == sysInfo.at(moduleName).end())
        {
#if !ANDROID_APP
            SYSINFO_INSERT(parse("${\033[40m}   ${\033[41m}   ${\033[42m}   ${\033[43m}   ${\033[44m}   ${\033[45m}   ${\033[46m}   ${\033[47m}   ${0}", _, parse_args));
#else // bruh why the android HTML implementation gotta be so dumb
            SYSINFO_INSERT(parse("${\033[40m}&nbsp;&nbsp;&nbsp;${\033[41m}&nbsp;&nbsp;&nbsp;${\033[42m}&nbsp;&nbsp;&nbsp;${\033[43m}&nbsp;&nbsp;&nbsp;${\033[44m}&nbsp;&nbsp;&nbsp;${\033[45m}&nbsp;&nbsp;&nbsp;${\033[46m}&nbsp;&nbsp;&nbsp;${\033[47m}&nbsp;&nbsp;&nbsp;${0}", _, parse_args));
#endif
        }
    }

    else if (moduleName == "colors_light")
    {
        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo.at(moduleName).find(moduleMemberName) == sysInfo.at(moduleName).end())
        {
#if !ANDROID_APP
            SYSINFO_INSERT(parse("${\033[100m}   ${\033[101m}   ${\033[102m}   ${\033[103m}   ${\033[104m}   ${\033[105m}   ${\033[106m}   ${\033[107m}   ${0}", _, parse_args));
#else
            SYSINFO_INSERT(parse("${\033[100m}&nbsp;&nbsp;&nbsp;${\033[101m}&nbsp;&nbsp;&nbsp;${\033[102m}&nbsp;&nbsp;&nbsp;${\033[103m}&nbsp;&nbsp;&nbsp;${\033[104m}&nbsp;&nbsp;&nbsp;${\033[105m}&nbsp;&nbsp;&nbsp;${\033[106m}&nbsp;&nbsp;&nbsp;${\033[107m}&nbsp;&nbsp;&nbsp;${0}", _, parse_args));
#endif
        }
    }
    
    // clang-format off
    // I really dislike how repetitive this code is
    else if (hasStart(moduleName, "colors_symbol"))
    {
        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo.at(moduleName).find(moduleMemberName) == sysInfo.at(moduleName).end())
        {
            if (moduleName.length() <= "colors_symbol()"_len)
                die(_("color palette module member '{}' in invalid.\n"
                      "Must be used like 'colors_symbol(`symbol for printing the color palette`)'.\n"
                      "e.g 'colors_symbol(@)' or 'colors_symbol(string)'"),
                    moduleName);

            std::string symbol = moduleName;
            symbol.erase(0, "colors_symbol("_len);
            symbol.pop_back();
            debug("symbol = {}", symbol);

            SYSINFO_INSERT(
                parse(fmt::format("${{\033[30m}} {0} ${{\033[31m}} {0} ${{\033[32m}} {0} ${{\033[33m}} {0} ${{\033[34m}} {0} ${{\033[35m}} {0} ${{\033[36m}} {0} ${{\033[37m}} {0} ${{0}}",
                                  symbol), _, parse_args));
        }
    }

    else if (hasStart(moduleName, "colors_light_symbol"))
    {
        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo.at(moduleName).find(moduleMemberName) == sysInfo.at(moduleName).end())
        {
            if (moduleName.length() <= "colors_light_symbol()"_len)
                die(_("light color palette module member '{}' in invalid.\n"
                      "Must be used like 'colors_light_symbol(`symbol for printing the color palette`)'.\n"
                      "e.g 'colors_light_symbol(@)' or 'colors_light_symbol(string)'"),
                    moduleName);

            std::string symbol = moduleName;
            symbol.erase(0, "colors_light_symbol("_len);
            symbol.pop_back();
            debug("symbol = {}", symbol);

            SYSINFO_INSERT(
                parse(fmt::format("${{\033[90m}} {0} ${{\033[91m}} {0} ${{\033[92m}} {0} ${{\033[93m}} {0} ${{\033[94m}} {0} ${{\033[95m}} {0} ${{\033[96m}} {0} ${{\033[97m}} {0} ${{0}}",
                                  symbol), _, parse_args));
        }
    }

    else
        die(_("Invalid module name: {}"), moduleName);
}
