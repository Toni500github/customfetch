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

#include "parse.hpp"

#include <unistd.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <ios>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "cufetch/common.hh"
#include "cufetch/config.hh"
#include "cufetch/cufetch.hh"
#include "fmt/color.h"
#include "fmt/format.h"
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
    size_t                 pos        = 0;
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

struct utsname Query::System::m_uname_infos;
struct passwd* Query::User::m_pPwd;
unsigned long  Query::System::m_uptime;

bool Query::System::m_bInit          = false;
bool Query::RAM::m_bInit             = false;
bool Query::CPU::m_bInit             = false;
bool Query::User::m_bInit            = false;
bool Query::Battery::m_bInit         = false;
bool Query::User::m_bDont_query_dewm = false;

// useless useful tmp string for parse() without using the original
// pureOutput
std::string _;

#if GUI_APP
// Get span tags from an ANSI escape color such as \e[0;31m
// @param noesc_str The ansi color without \\e[ or \033[
// @param colors The colors struct we'll look at
// @return An array of 3 span tags elements in the follow: color, weight, type
static std::array<std::string, 3> get_ansi_color(const std::string_view noesc_str, const ConfigBase& config)
{
    const size_t first_m = noesc_str.rfind('m');
    if (first_m == std::string::npos)
        die(_("Parser: failed to parse layout/ascii art: missing 'm' while using ANSI color escape code in '{}'"),
            noesc_str);

    std::string col{ noesc_str.data() };
    col.erase(first_m);  // 1;42

    std::string weight{ hasStart(col, "1;") ? "bold" : "normal" };
    std::string type{ "fgcolor" };  // either fgcolor or bgcolor

    if (hasStart(col, "1;") || hasStart(col, "0;"))
        col.erase(0, 2);

    debug("col = {}", col);
    const int n = std::stoi(col);

    if ((n >= 100 && n <= 107) || (n >= 40 && n <= 47))
        type = "bgcolor";

    // last number
    // clang-format off
    switch (col.back())
    {
        case '0': col = config.getThemeValue("gui.black",   "!#000005"); break;
        case '1': col = config.getThemeValue("gui.red",     "!#ff2000"); break;
        case '2': col = config.getThemeValue("gui.green",   "!#00ff00"); break;
        case '3': col = config.getThemeValue("gui.yellow",  "!#ffff00"); break;
        case '4': col = config.getThemeValue("gui.blue",    "!#00aaff"); break;
        case '5': col = config.getThemeValue("gui.magenta", "!#ff11cc"); break;
        case '6': col = config.getThemeValue("gui.cyan",    "!#00ffff"); break;
        case '7': col = config.getThemeValue("gui.white",   "!#ffffff"); break;
    }

    if (col.at(0) != '#')
        col.erase(0, col.find('#'));

    if ((n >= 100 && n <= 107) || (n >= 90 && n <= 97))
    {
        const fmt::rgb color = hexStringToColor(col);
        const uint r = color.r * 0.65f + 0xff * 0.35f;
        const uint b = color.b * 0.65f + 0xff * 0.35f;
        const uint g = color.g * 0.65f + 0xff * 0.35f;
        const uint result = (r << 16) | (g << 8) | (b);

        std::stringstream ss;
        ss << std::hex << result;
        col = ss.str();
        col.insert(0, "#");
    }

    return { col, weight, type };
    // clang-format on
}

// Convert an ANSI escape RGB color, such as \e[38;2;132;042;231m
// into an hex color string
// @param noesc_str The ansi color without \\e[ or \033[
// @return The hex equivalent string
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
#endif

EXPORT std::string parse(const std::string& input, std::string& _, parse_args_t& parse_args)
{
    return parse(input, parse_args.modulesInfo, _, parse_args.layout, parse_args.tmp_layout, parse_args.config,
                 parse_args.parsingLayout, parse_args.no_more_reset);
}

EXPORT std::string parse(const std::string& input, parse_args_t& parse_args)
{
    return parse(input, parse_args.modulesInfo, parse_args.pureOutput, parse_args.layout, parse_args.tmp_layout,
                 parse_args.config, parse_args.parsingLayout, parse_args.no_more_reset);
}

std::string get_and_color_percentage(const float n1, const float n2, parse_args_t& parse_args, const bool invert)
{
    const std::vector<std::string>& percentage_colors = parse_args.config.getValueArrayStr("config.percentage-colors", {"green", "yellow", "red"});
    const float                     result = n1 / n2 * static_cast<float>(100);

    std::string color;
    if (!invert)
    {
        if (result <= 45)
            color = "${" + percentage_colors.at(0) + "}";
        else if (result <= 80)
            color = "${" + percentage_colors.at(1) + "}";
        else
            color = "${" + percentage_colors.at(2) + "}";
    }
    else
    {
        if (result <= 45)
            color = "${" + percentage_colors.at(2) + "}";
        else if (result <= 80)
            color = "${" + percentage_colors.at(1) + "}";
        else
            color = "${" + percentage_colors.at(0) + "}";
    }

    return parse(fmt::format("{}{:.2f}%${{0}}", color, result), _, parse_args);
}

std::string getInfoFromName(parse_args_t& parse_args, const std::string& moduleName)
{
    std::string name;
    name.reserve(moduleName.size());

    /* true when we find a '(' */
    bool collecting = false;

    /* current position */
    size_t i                   = -1;
    size_t stripped_char_count = 0; /* amount of chars stripped from `name` */

    /* position of start, resets every separator */
    size_t start_pos = 0;

    moduleArgs_t* moduleArgs = new moduleArgs_t;

    /* argument that's collected from what's between the parenthesis in "module(...).test" */
    std::string arg;
    arg.reserve(moduleName.size());
    for (const char c : moduleName)
    {
        i++;
        if (c == '(' && !collecting)
        {
            collecting = true;
            continue;
        }

        if ((c == '.' || i + 1 == moduleName.size()))
        {
            if (collecting)
            {
                if (arg.back() != ')' && c != ')')
                    die("Module name `{}` is invalid. Arguments must end with )", moduleName);

                if (arg.back() == ')')
                    arg.pop_back();

                moduleArgs_t* moduleArg = moduleArgs;
                while (moduleArg->next != nullptr)
                    moduleArg = moduleArg->next;

                moduleArg->name       = std::string{ name.begin() + start_pos, name.end() };
                moduleArg->value      = arg;
                moduleArg->next       = new moduleArgs_t;
                moduleArg->next->prev = moduleArg;

                if (c == '.')
                {
                    name.push_back('.');
                    stripped_char_count++;
                }
            }
            else
            {
                name.push_back(c);
            }

            start_pos  = i + 1 - stripped_char_count;
            arg        = "";
            collecting = false;

            continue;
        }

        if (!collecting)
        {
            name.push_back(c);
        }
        else
        {
            stripped_char_count++;
            arg.push_back(c);
        }
    }

    std::string result = "(unknown/invalid module)";
    if (const auto& it = parse_args.modulesInfo.find(name); it != parse_args.modulesInfo.end())
    {
        struct callbackInfo_t callbackInfo = { moduleArgs, parse_args };

        result = it->second.handler(&callbackInfo);
    }

    while (moduleArgs)
    {
        moduleArgs_t* next = moduleArgs->next;

        delete moduleArgs;

        moduleArgs = next;
    }

    return result;
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

    if (parse_args.config.getValue("intern.args.disallow-commands", false))
        die(_("Trying to execute command $({}) but --disallow-command-tag is set"), command);

    const bool removetag = (command.front() == '!');
    if (removetag)
        command.erase(0, 1);

    const std::string& cmd_output = read_shell_exec(command);
    if (!parse_args.parsingLayout && !removetag && parser.dollar_pos != std::string::npos)
        parse_args.pureOutput.replace(parser.dollar_pos, command.length() + "$()"_len, cmd_output);

    return cmd_output;
}

template <typename... Styles>
static void append_styles(fmt::text_style& current_style, Styles&&... styles)
{
    current_style |= (styles | ...);
}

std::optional<std::string> parse_color_tag(Parser& parser, parse_args_t& parse_args, const bool evaluate)
{
    if (!parser.try_read('{'))
        return {};

    std::string color = parse(parser, parse_args, evaluate, '}');

    if (!evaluate)
        return {};

    std::string       output;
    const size_t      taglen  = color.length() + "${}"_len;
    const ConfigBase& config  = parse_args.config;
    const std::string endspan = !parse_args.firstrun_clr ? "</span>" : "";

    if (config.getValue("intern.args.disable-colors", false))
    {
        if (parser.dollar_pos != std::string::npos)
            parse_args.pureOutput.erase(parser.dollar_pos, taglen);
        return "";
    }

    // if at end there a '$', it will make the end output "$</span>" and so it will confuse
    // addValueFromModule() and so let's make it "$ </span>". this is geniunenly stupid
#if GUI_APP
    if (output[0] == '$')
        output += ' ';
#endif
    
    static std::vector<std::string> alias_colors_name, alias_colors_value;
    const std::vector<std::string>& alias_colors = config.getValueArrayStr("config.alias-colors", {});
    if (!alias_colors.empty() && (alias_colors_name.empty() && alias_colors_value.empty()))
    {
        for (const std::string& str : alias_colors)
        {
            const size_t pos = str.find('=');
            if (pos == std::string::npos)
                die(_("alias color '{}' does NOT have an equal sign '=' for separating color name and value\n"
                    "For more check with --help"), str);

            alias_colors_name.push_back(str.substr(0, pos));
            alias_colors_value.push_back(str.substr(pos + 1));
        }

        const auto& it_name = std::find(alias_colors_name.begin(), alias_colors_name.end(), color);
        if (it_name != alias_colors_name.end())
        {
            const size_t index = std::distance(alias_colors_name.begin(), it_name);
            color              = alias_colors_value.at(index);
        }
    }

    static std::vector<std::string> auto_colors;
    if (hasStart(color, "auto"))
    {
        int ver = color.length() > 4 ? std::stoi(color.substr(4)) - 1 : 0;

        if (auto_colors.empty())
            auto_colors.push_back(NOCOLOR_BOLD);

        if (ver < 0 || static_cast<size_t>(ver) >= auto_colors.size())
            ver = 0;

        color = auto_colors.at(ver);
    }

#if GUI_APP
    if (color == "1")
    {
        output += endspan + "<span weight='bold'>";
    }
    else if (color == "0")
    {
        output += endspan + "<span>";
    }
#else
    if (color == "1")
    {
        output += NOCOLOR_BOLD;
    }
    else if (color == "0")
    {
        output += NOCOLOR;
    }
#endif
    else
    {
        std::string str_clr;
#if GUI_APP
        switch (fnv1a16::hash(color))
        {
            case "black"_fnv1a16:   str_clr = config.getThemeValue("gui.black",   "!#000005"); break;
            case "red"_fnv1a16:     str_clr = config.getThemeValue("gui.red",     "!#ff2000"); break;
            case "green"_fnv1a16:   str_clr = config.getThemeValue("gui.green",   "!#00ff00"); break;
            case "yellow"_fnv1a16:  str_clr = config.getThemeValue("gui.yellow",  "!#ffff00"); break;
            case "blue"_fnv1a16:    str_clr = config.getThemeValue("gui.blue",    "!#00aaff"); break;
            case "magenta"_fnv1a16: str_clr = config.getThemeValue("gui.magenta", "!#ff11cc"); break;
            case "cyan"_fnv1a16:    str_clr = config.getThemeValue("gui.cyan",    "!#00ffff"); break;
            case "white"_fnv1a16:   str_clr = config.getThemeValue("gui.white",   "!#ffffff"); break;
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
                const std::array<std::string, 3>& clrs   = get_ansi_color(noesc_str, config);
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

// #if !GUI_APP
#else
        switch (fnv1a16::hash(color))
        {
            case "black"_fnv1a16:   str_clr = config.getThemeValue("config.black",   "\033[1;30m"); break;
            case "red"_fnv1a16:     str_clr = config.getThemeValue("config.red",     "\033[1;31m"); break;
            case "green"_fnv1a16:   str_clr = config.getThemeValue("config.green",   "\033[1;32m"); break;
            case "yellow"_fnv1a16:  str_clr = config.getThemeValue("config.yellow",  "\033[1;33m"); break;
            case "blue"_fnv1a16:    str_clr = config.getThemeValue("config.blue",    "\033[1;34m"); break;
            case "magenta"_fnv1a16: str_clr = config.getThemeValue("config.magenta", "\033[1;35m"); break;
            case "cyan"_fnv1a16:    str_clr = config.getThemeValue("config.cyan",    "\033[1;36m"); break;
            case "white"_fnv1a16:   str_clr = config.getThemeValue("config.white",   "\033[1;37m"); break;
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
                const uint32_t rgb_num =
                    bgcolor ? style.get_background().value.rgb_color : style.get_foreground().value.rgb_color;
                fmt::rgb                             rgb(rgb_num);
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
#endif

        if (!parse_args.parsingLayout && std::find(auto_colors.begin(), auto_colors.end(), color) == auto_colors.end())
            auto_colors.push_back(color);
    }

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

    const std::string& info = getInfoFromName(parse_args, module);

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

EXPORT std::string parse(std::string input, const moduleMap_t& modulesInfo, std::string& pureOutput,
                  std::vector<std::string>& layout, std::vector<std::string>& tmp_layout, const ConfigBase& config,
                  const bool parsingLayout, bool& no_more_reset)
{
    const std::string& sep_reset = config.getValue<std::string>("config.sep-reset", ":");
    if (!sep_reset.empty() && parsingLayout && !no_more_reset)
    {
        if (config.getValue("config.sep-reset-after", false))
            replace_str(input, sep_reset, sep_reset + "${0}");
        else
            replace_str(input, sep_reset, "${0}" + sep_reset);

        no_more_reset = true;
    }

    parse_args_t parse_args{ modulesInfo, pureOutput, layout, tmp_layout, config, parsingLayout, true, no_more_reset };
    Parser       parser{ input, pureOutput };

    std::string ret{ parse(parser, parse_args) };

#if GUI_APP
    if (!parse_args.firstrun_clr)
        ret += "</span>";

    replace_str(parse_args.pureOutput, "&nbsp;", " ");

    // escape pango markup
    // https://gitlab.gnome.org/GNOME/glib/-/blob/main/glib/gmarkup.c#L2150
    // workaround: just put "\<" or "\&" in the config, e.g "$<os.kernel> \<- Kernel"
    replace_str(ret, "\\<", "&lt;");
    replace_str(ret, "\\&", "&amp;");
    replace_str(ret, "&", "&amp;");
#else
    replace_str(ret, "\\<", "<");
    replace_str(ret, "\\&", "&");
#endif

    return ret;
}
