#include "parse.hpp"

#include <unistd.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

#include "config.hpp"
#include "fmt/color.h"
#include "query.hpp"
#include "switch_fnv1a.hpp"
#include "util.hpp"

Query::System::System_t Query::System::m_system_infos;
Query::Theme::Theme_t   Query::Theme::m_theme_infos;
Query::User::User_t     Query::User::m_users_infos;
Query::CPU::CPU_t       Query::CPU::m_cpu_infos;
Query::RAM::RAM_t       Query::RAM::m_memory_infos;
Query::GPU::GPU_t       Query::GPU::m_gpu_infos;
Query::Disk::Disk_t     Query::Disk::m_disk_infos;
bool                    Query::User::m_bDont_query_dewm = false;

struct statvfs Query::Disk::m_statvfs;
struct utsname Query::System::m_uname_infos;
struct sysinfo Query::System::m_sysInfos;
struct passwd* Query::User::m_pPwd;

bool Query::System::m_bInit = false;
bool Query::RAM::m_bInit    = false;
bool Query::CPU::m_bInit    = false;
bool Query::User::m_bInit   = false;

static std::array<std::string, 3> get_ansi_color(const std::string_view str, const colors_t& colors)
{
    if (hasStart(str, "38") || hasStart(str, "48"))
        die("Can't convert \\e[38; or \\e[48; codes in GUI. Please use #hexcode colors instead.");

    const size_t first_m = str.find('m');
    if (first_m == std::string::npos)
        die("Parser: failed to parse layout/ascii art: missing m while using ANSI color escape code");

    std::string col = str.data();
    col.erase(first_m);  // 1;42
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

    // last number
    // https://stackoverflow.com/a/5030086

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

    if (col[0] != '#')
        col.erase(0, col.find('#'));

    return { col, weight, type };
}

static std::string get_and_color_percentage(const float& n1, const float& n2, systemInfo_t& systemInfo, const Config& config, const colors_t& colors,
                                            const bool parsingLayout, bool invert = false)
{

    const float result = static_cast<float>(n1 / n2 * static_cast<float>(100));

    std::string_view color;
    if (!invert)
    {
        if (result <= 35)
            color = "${green}";
        else if (result <= 80)
            color = "${yellow}";
        else
            color = "${red}";
    }
    else
    {
        if (result <= 35)
            color = "${red}";
        else if (result <= 80)
            color = "${yellow}";
        else
            color = "${green}";
    }

    std::string _;
    return parse(fmt::format("{}{:.2f}%${{0}}", color, result), systemInfo, _, config, colors, parsingLayout);
}

static const std::string& check_gui_ansi_clr(const std::string& str)
{
    if (hasStart(str, "\033") || hasStart(str, "\\e"))
        die("GUI colors can't be in ANSI escape sequence");

    return str;
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

            else if (std::holds_alternative<float>(result))
                return fmt::format("{:.2f}", (std::get<float>(result)));

            else
                return fmt::to_string(std::get<size_t>(result));
        }
    }

    return "(unknown/invalid module)";
}

std::string parse(const std::string_view input, systemInfo_t& systemInfo, std::string& pureOutput, const Config& config,
                  const colors_t& colors, const bool parsingLayout)
{
    std::string output = input.data();
    pureOutput         = output;

    size_t dollarSignIndex = 0;
    size_t oldDollarSignIndex = 0;
    bool   start           = false;

    // we only use it in GUI mode,
    // prevent issue where in the ascii art,
    // theres at first either ${1} or ${0}
    // and that's a problem with pango markup
    bool   firstrun_noclr  = true;


    static std::vector<std::string> auto_colors;

    if (!config.sep_reset.empty() && parsingLayout)
    {
        if (config.sep_reset_after)
        {
            replace_str(output, config.sep_reset, config.sep_reset + "${0}");
            replace_str(pureOutput, config.sep_reset, config.sep_reset + "${0}");
        }
        else
        {
            replace_str(output, config.sep_reset, "${0}" + config.sep_reset);
            replace_str(pureOutput, config.sep_reset, "${0}" + config.sep_reset);
        }
    }

    while (true)
    {
        oldDollarSignIndex = dollarSignIndex;
        dollarSignIndex    = output.find('$', dollarSignIndex);

    retry:
        if (dollarSignIndex == std::string::npos)
            break;

        else if (dollarSignIndex <= oldDollarSignIndex && start)
        {
            dollarSignIndex = output.find('$', dollarSignIndex + 1);
            // oh nooo.. whatever
            goto retry;
        }

        start = true;

        // check for bypass
        // YOU CAN USE AND/NOT IN C++????
        // btw the second part checks if it has a \ before it and NOT a \ before the backslash, (check for escaped
        // backslash) example: \$ is bypassed, \\$ is NOT bypassed. this will not make an effort to check multiple
        // backslashes, thats your fault atp.
        if (dollarSignIndex > 0 and
            (output[dollarSignIndex - 1] == '\\' and (dollarSignIndex == 1 or output[dollarSignIndex - 2] != '\\')))
            continue;

        std::string command;
        size_t      endBracketIndex = -1;

        char type    = ' ';  // ' ' = undefined, ')' = shell exec, 2 = ')' asking for a module
        const char opentag = output[dollarSignIndex + 1];

        switch (opentag)
        {
            case '(': type = ')'; break;
            case '<': type = '>'; break;
            case '%': type = '%'; break;
            case '[': type = ']'; break;
            case '{': type = '}'; break;
            default:  // neither of them
                break;
        }

        if (type == ' ')
            continue;

        // let's get what's inside the brackets
        for (size_t i = dollarSignIndex + 2; i < output.size(); i++)
        {
            if (output[i] == type && output[i - 1] != '\\')
            {
                endBracketIndex = i;
                break;
            }
            else if (output.at(i) == type)
                command.pop_back();

            command += output.at(i);
        }

        if (static_cast<int>(endBracketIndex) == -1)
            die("PARSER: Opened tag is not closed at index {} in string {}", dollarSignIndex, output);

        const std::string& tagToReplace = fmt::format("${}{}{}", opentag, command, type);
        const size_t       tagpos       = pureOutput.find(tagToReplace);
        const size_t       taglen       = (endBracketIndex + 1) - dollarSignIndex;

        switch (type)
        {
            case ')':
            {
                const std::string& shell_cmd = shell_exec(command);
                output.replace(dollarSignIndex, taglen, shell_cmd);

                if (!parsingLayout && tagpos != std::string::npos)
                    pureOutput.replace(tagpos, taglen, shell_cmd);

            } break;

            case '>':
            {
                const size_t& dot_pos = command.find('.');
                if (dot_pos == std::string::npos)
                    die("module name '{}' doesn't have a dot '.' for separiting module name and value", command);

                const std::string& moduleName      = command.substr(0, dot_pos);
                const std::string& moduleMemberName = command.substr(dot_pos + 1);
                addValueFromModule(systemInfo, moduleName, moduleMemberName, config, colors, parsingLayout);

                output.replace(dollarSignIndex, taglen,
                                        getInfoFromName(systemInfo, moduleName, moduleMemberName));

                if (!parsingLayout && tagpos != std::string::npos)
                    pureOutput.replace(tagpos, taglen,
                                        getInfoFromName(systemInfo, moduleName, moduleMemberName));
            } break;

            case '%':
            {
                const size_t& comma_pos = command.find(',');
                if (comma_pos == std::string::npos)
                    die("percentage tag '{}' doesn't have a comma for separating the 2 numbers", command);

                std::string _;
                const float& n1 = std::stof(parse(command.substr(0, comma_pos),  systemInfo, _, config, colors, parsingLayout));
                const float& n2 = std::stof(parse(command.substr(comma_pos + 1), systemInfo, _, config, colors, parsingLayout));

                output.replace(dollarSignIndex, taglen, get_and_color_percentage(n1, n2, systemInfo, config, colors, parsingLayout, (command.back() == '!')));
                break;
            }

            case ']':
            {
                const size_t& conditional_comma = command.find(',');
                if (conditional_comma == command.npos)
                    die("conditional tag {} doesn't have a comma for separiting the conditional", command);

                const size_t& equalto_comma = command.find(',', conditional_comma + 1);
                if (equalto_comma == command.npos)
                    die("conditional tag {} doesn't have a comma for separiting the equalto", command);

                const size_t& true_comma = command.find(',', equalto_comma + 1);
                if (true_comma == command.npos)
                    die("conditional tag {} doesn't have a comma for separiting the true statment", command);

                const std::string& conditional    = command.substr(0, conditional_comma);
                const std::string& equalto        = command.substr(conditional_comma + 1, equalto_comma - conditional_comma - 1);
                const std::string& true_statment  = command.substr(equalto_comma + 1, true_comma - equalto_comma - 1);
                const std::string& false_statment = command.substr(true_comma + 1);

                std::string _;
                const std::string& parsed_conditional = parse(conditional, systemInfo, _, config, colors, parsingLayout);
                const std::string& parsed_equalto     = parse(equalto, systemInfo, _, config, colors, parsingLayout);

                if (parsed_conditional == parsed_equalto)
                {
                    const std::string& parsed_true_stam = parse(true_statment, systemInfo, _, config, colors, parsingLayout);
                    output.replace(dollarSignIndex, taglen, parsed_true_stam);
                }
                else
                {
                    const std::string& parsed_false_stam = parse(false_statment, systemInfo, _, config, colors, parsingLayout);
                    output.replace(dollarSignIndex, taglen, parsed_false_stam);
                }

            } break;

            case '}':  // please pay very attention when reading this unreadable and godawful code

                // if at end there a '$', it will make the end output "$</span>" and so it will confuse addValueFromModule()
                // and so let's make it "$ </span>".
                // this geniunenly stupid
                if (config.gui && output.back() == '$')
                    output += ' ';

                if (!config.m_arg_colors_name.empty())
                {
                    const auto& it_name = std::find(config.m_arg_colors_name.begin(), config.m_arg_colors_name.end(), command);
                    if (it_name != config.m_arg_colors_name.end())
                    {
                        const auto& it_value = std::distance(config.m_arg_colors_name.begin(), it_name);

                        if (hasStart(command, "auto"))
                        {
                            // "ehhmmm why goto and double code? that's ugly and unconvienient :nerd:"
                            // I don't care, it does the work and well
                            if (command == *it_name)
                                command = config.m_arg_colors_value.at(it_value);
                            goto jump;
                        }

                        if (command == *it_name)
                            command = config.m_arg_colors_value.at(it_value);
                    }
                }

                if (hasStart(command, "auto"))
                {
                    std::uint16_t ver =
                        static_cast<std::uint16_t>(command.length() > 4 ? std::stoi(command.substr(4)) - 1 : 0);
                    if (ver >= auto_colors.size() || ver < 1)
                        ver = 0;

                    if (auto_colors.empty())
                        auto_colors.push_back(NOCOLOR_BOLD);

                    command = auto_colors.at(ver);
                }

            jump:
                if (command == "1")
                {
                    if (firstrun_noclr)
                        output.replace(dollarSignIndex, taglen,
                                                config.gui ? "<span weight='bold'>" : NOCOLOR_BOLD);
                    else
                        output.replace(dollarSignIndex, taglen,
                                                config.gui ? "</span><span weight='bold'>" : NOCOLOR_BOLD);
                }
                else if (command == "0")
                {
                    if (firstrun_noclr)
                        output.replace(dollarSignIndex, taglen,
                                                config.gui ? "<span>" : NOCOLOR);
                    else
                        output.replace(dollarSignIndex, taglen,
                                                config.gui ? "</span><span>" : NOCOLOR);
                }
                else
                {
                    std::string str_clr;
                    if (config.gui)
                    {
                        switch (fnv1a16::hash(command))
                        {
                            case "black"_fnv1a16:   str_clr = check_gui_ansi_clr(colors.gui_black); break;
                            case "red"_fnv1a16:     str_clr = check_gui_ansi_clr(colors.gui_red); break;
                            case "blue"_fnv1a16:    str_clr = check_gui_ansi_clr(colors.gui_blue); break;
                            case "green"_fnv1a16:   str_clr = check_gui_ansi_clr(colors.gui_green); break;
                            case "cyan"_fnv1a16:    str_clr = check_gui_ansi_clr(colors.gui_cyan); break;
                            case "yellow"_fnv1a16:  str_clr = check_gui_ansi_clr(colors.gui_yellow); break;
                            case "magenta"_fnv1a16: str_clr = check_gui_ansi_clr(colors.gui_magenta); break;
                            case "white"_fnv1a16:   str_clr = check_gui_ansi_clr(colors.gui_white); break;
                            default:                str_clr = command; break;
                        }

                        const size_t pos = str_clr.find('#');
                        if (pos != std::string::npos)
                        {
                            std::string tagfmt = "span ";

                            const std::string& opt_clr = str_clr.substr(0, pos);

                            if (opt_clr.find('b') != std::string::npos)
                                tagfmt += "bgcolor='" + str_clr.substr(pos) + "' ";
                            else
                                tagfmt += "fgcolor='" + str_clr.substr(pos) + "' ";

                            if (opt_clr.find('!') != std::string::npos)
                                tagfmt += "weight='bold' ";

                            if (opt_clr.find('u') != std::string::npos)
                                tagfmt += "underline='single' ";

                            if (opt_clr.find('i') != std::string::npos)
                                tagfmt += "style='italic' ";

                            tagfmt.pop_back();

                            output.replace(dollarSignIndex, output.length() - dollarSignIndex,
                                                    fmt::format("<{}>{}</span>",
                                                                tagfmt, output.substr(endBracketIndex + 1)));
                        }

                        // "\\e" is for checking in the ascii_art, \033 in the config
                        else if (hasStart(str_clr, "\\e") ||
                                 hasStart(str_clr, "\033"))
                        {
                            const std::array<std::string, 3>& clrs = get_ansi_color(
                                (hasStart(str_clr, "\033") ? str_clr.substr(2) : str_clr.substr(3)), colors);

                            const std::string_view color  = clrs.at(0);
                            const std::string_view weight = clrs.at(1);
                            const std::string_view type   = clrs.at(2);
                            output.replace(dollarSignIndex, output.length() - dollarSignIndex,
                                                    fmt::format("<span {}='{}' weight='{}'>{}</span>",
                                                                type, color, weight, output.substr(endBracketIndex + 1)));
                        }

                        else
                            error("PARSER: failed to parse line with color '{}'", str_clr);

                        firstrun_noclr = false;
                    }

                    else
                    {
                        switch (fnv1a16::hash(command))
                        {
                            case "black"_fnv1a16:   str_clr = colors.black; break;
                            case "red"_fnv1a16:     str_clr = colors.red; break;
                            case "blue"_fnv1a16:    str_clr = colors.blue; break;
                            case "green"_fnv1a16:   str_clr = colors.green; break;
                            case "cyan"_fnv1a16:    str_clr = colors.cyan; break;
                            case "yellow"_fnv1a16:  str_clr = colors.yellow; break;
                            case "magenta"_fnv1a16: str_clr = colors.magenta; break;
                            case "white"_fnv1a16:   str_clr = colors.white; break;
                            default:                str_clr = command; break;
                        }

                        std::string formatted_replacement_string;

                        const size_t pos = str_clr.find('#');
                        if (pos != std::string::npos)
                        {
                            const std::string& opt_clr = str_clr.substr(0, pos);

                            fmt::text_style style;

                            if (opt_clr.find('b') != std::string::npos)
                                append_styles(style, fmt::bg(hexStringToColor(str_clr.substr(pos))));
                            else
                                append_styles(style, fmt::fg(hexStringToColor(str_clr.substr(pos))));

                            if (opt_clr.find('!') != std::string::npos)
                                append_styles(style, fmt::emphasis::bold);

                            if (opt_clr.find('u') != std::string::npos)
                                append_styles(style, fmt::emphasis::underline);

                            if (opt_clr.find('i') != std::string::npos)
                                append_styles(style, fmt::emphasis::italic);

                            formatted_replacement_string =
                                fmt::format(style, "{}", output.substr(endBracketIndex + 1));
                        }

                        else if (hasStart(str_clr, "\\e") || hasStart(str_clr, "\033"))
                        {
                            formatted_replacement_string =
                                fmt::format("\x1B[{}{}",
                                            // "\\e" is for checking in the ascii_art, \033 in the config
                                            hasStart(str_clr, "\033") ? str_clr.substr(2) : str_clr.substr(3),
                                            output.substr(endBracketIndex + 1));
                        }

                        else
                            error("PARSER: failed to parse line with color '{}'", str_clr);

                        output.replace(dollarSignIndex, output.length() - dollarSignIndex,
                                                formatted_replacement_string);
                    }

                    if (!parsingLayout &&
                        std::find(auto_colors.begin(), auto_colors.end(), command) == auto_colors.end())
                        auto_colors.push_back(command);
                }

                if (config.gui && firstrun_noclr)
                    output += "</span>";

                if (!parsingLayout && tagpos != std::string::npos)
                    pureOutput.erase(tagpos, tagToReplace.length());
        }
    }

    // https://github.com/dunst-project/dunst/issues/900
    // pango markup doesn't like '<' if it's not a tag
    // and doesn't like '&' too
    // workaround: just put "\<" or "\&" in the config, e.g "$<os.kernel> \<- Kernel"
    if (config.gui)
    {
        replace_str(output, "\\<", "&lt;");
        replace_str(output, "\\&", "&amp;");
        replace_str(output, "&amp;lt;", "&lt;");
        replace_str(output, "&lt;span", "\\<span");
        replace_str(output, "&lt;/span>", "\\</span>");
    }
    else
    {
        replace_str(output, "\\<", "<");
        replace_str(output, "\\&", "&");
    }

    replace_str(pureOutput, "\\<", "<");
    replace_str(pureOutput, "\\&", "&");

    return output;
}

static std::string get_auto_uptime(const std::uint16_t days, const std::uint16_t hours, const std::uint16_t mins, const std::uint16_t secs,
                                   const Config& config)
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
        case "gnome terminal"_fnv1a16:
            return "GNOME Terminal";

        case "gnome-console"_fnv1a16:
        case "gnome console"_fnv1a16:
            return "GNOME console";
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

void addValueFromModule(systemInfo_t& sysInfo, const std::string& moduleName, const std::string& moduleMemberName,
                        const Config& config, const colors_t& colors, bool parsingLayout)
{
#define SYSINFO_INSERT(x) sysInfo[moduleName].insert({ moduleMemberName, variant(x) })
    // yikes, here we go.
    const  auto&                         moduleMember_hash = fnv1a16::hash(moduleMemberName);
    static std::vector<std::uint16_t>    queried_gpus;
    static std::vector<std::string_view> queried_disks;
    static std::vector<std::string>      queried_themes_names;
    static systemInfo_t                  queried_themes;

    if (moduleName == "os")
    {
        Query::System query_system;

        std::chrono::seconds uptime_secs(query_system.uptime());
        const auto& uptime_mins  = std::chrono::duration_cast<std::chrono::minutes>(uptime_secs);
        const auto& uptime_hours = std::chrono::duration_cast<std::chrono::hours>(uptime_secs);
        
        // let's support a little of C++17 without any `#if __cpluscplus` stuff
        const std::uint16_t uptime_days  = uptime_secs.count() / (60 * 60 * 24);

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo[moduleName].find(moduleMemberName) == sysInfo[moduleName].end())
        {
            switch (moduleMember_hash)
            {
                case "name"_fnv1a16: SYSINFO_INSERT(query_system.os_pretty_name()); break;

                case "uptime"_fnv1a16:
                    SYSINFO_INSERT(get_auto_uptime(uptime_days, uptime_hours.count() % 24,
                                                   uptime_mins.count() % 60, uptime_secs.count() % 60, config));
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
            }
        }
    }

    else if (moduleName == "system")
    {
        Query::System query_system;

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo[moduleName].find(moduleMemberName) == sysInfo[moduleName].end())
        {
            switch (moduleMember_hash)
            {
                case "host"_fnv1a16:
                    SYSINFO_INSERT(fmt::format("{} {} {}", query_system.host_vendor(), query_system.host_modelname(),
                                               query_system.host_version()));
                    break;

                case "host_name"_fnv1a16: SYSINFO_INSERT(query_system.host_modelname()); break;

                case "host_vendor"_fnv1a16: SYSINFO_INSERT(query_system.host_vendor()); break;

                case "host_version"_fnv1a16: SYSINFO_INSERT(query_system.host_version()); break;

                case "arch"_fnv1a16: SYSINFO_INSERT(query_system.arch()); break;
            }
        }
    }

    else if (moduleName == "builtin")
    {
        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo[moduleName].find(moduleMemberName) == sysInfo[moduleName].end())
        {
            std::string _;
            switch(moduleMember_hash)
            {
                case "title"_fnv1a16:
                    SYSINFO_INSERT(parse("${auto2}$<user.name>${0}@${auto2}$<os.hostname>", sysInfo, _, config, colors, parsingLayout)); break;

                case "title_sep"_fnv1a16:
                {
                    Query::User query_user;
                    Query::System query_system;
                    const size_t& title_len = fmt::format("{}@{}", query_user.name(), query_system.hostname()).length();

                    std::string   str;
                    str.reserve(config.builtin_title_sep.length() * title_len);
                    for (size_t i = 0; i < title_len; i++)
                        str += config.builtin_title_sep;

                    SYSINFO_INSERT(str);
                } break;

                case "colors_bg"_fnv1a16:
                    SYSINFO_INSERT(parse("${\033[40m}   ${\033[41m}   ${\033[42m}   ${\033[43m}   ${\033[44m}   ${\033[45m}   ${\033[46m}   ${\033[47m}   ", sysInfo, _, config, colors, parsingLayout));
                    break;

                case "colors_light_bg"_fnv1a16:
                    SYSINFO_INSERT(parse("${\033[100m}   ${\033[101m}   ${\033[102m}   ${\033[103m}   ${\033[104m}   ${\033[105m}   ${\033[106m}   ${\033[107m}   ", sysInfo, _, config, colors, parsingLayout));
                    break;

                default:
                    // I really dislike how repetitive this code is
                    if (hasStart(moduleMemberName, "colors_symbol"))
                    {
                        if (moduleMemberName.length() <= "colors_symbol()"_len)
                            die("color palette module member '{}' in invalid.\n"
                                "Must be used like 'colors_symbol(`symbol for printing the color palette`)'.\n"
                                "e.g 'colors_symbol(@)' or 'colors_symbol(string)'",
                                moduleMemberName);

                        std::string symbol = moduleMemberName;
                        symbol.erase(0, "colors_symbol("_len);
                        symbol.pop_back();
                        debug("symbol = {}", symbol);

                        SYSINFO_INSERT(
                            parse(fmt::format("${{\033[30m}} {0} ${{\033[31m}} {0} ${{\033[32m}} {0} ${{\033[33m}} {0} ${{\033[34m}} {0} ${{\033[35m}} {0} ${{\033[36m}} {0} ${{\033[37m}} {0} ",
                                              symbol), sysInfo, _, config, colors, parsingLayout));
                    }
                    else if (hasStart(moduleMemberName, "colors_light_symbol"))
                    {
                        if (moduleMemberName.length() <= "colors_light_symbol()"_len)
                            die("light color palette module member '{}' in invalid.\n"
                                "Must be used like 'colors_light_symbol(`symbol for printing the color palette`)'.\n"
                                "e.g 'colors_light_symbol(@)' or 'colors_light_symbol(string)'",
                                moduleMemberName);

                        std::string symbol = moduleMemberName;
                        symbol.erase(0, "colors_light_symbol("_len);
                        symbol.pop_back();
                        debug("symbol = {}", symbol);

                        SYSINFO_INSERT(
                            parse(fmt::format("${{\033[90m}} {0} ${{\033[91m}} {0} ${{\033[92m}} {0} ${{\033[93m}} {0} ${{\033[94m}} {0} ${{\033[95m}} {0} ${{\033[96m}} {0} ${{\033[97m}} {0} ",
                                              symbol), sysInfo, _, config, colors, parsingLayout));
                    }
            }
        }
    }

    else if (moduleName == "user")
    {
        Query::User query_user;

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo[moduleName].find(moduleMemberName) == sysInfo[moduleName].end())
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

                case "term"_fnv1a16:
                    SYSINFO_INSERT(prettify_term_name(query_user.term_name()) + ' ' +
                                   query_user.term_version(query_user.term_name()));
                    break;

                case "term_name"_fnv1a16: SYSINFO_INSERT(prettify_term_name(query_user.term_name())); break;

                case "term_version"_fnv1a16: SYSINFO_INSERT(query_user.term_version(query_user.term_name())); break;
            }
        }
    }

    else if (moduleName == "theme")
    {
        Query::Theme query_theme(queried_themes);

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo[moduleName].find(moduleMemberName) == sysInfo[moduleName].end())
        {
            switch (moduleMember_hash)
            {
                case "cursor"_fnv1a16:      SYSINFO_INSERT(query_theme.cursor()); break;
                case "cursor_size"_fnv1a16: SYSINFO_INSERT(query_theme.cursor_size()); break;
            }
        }
    }

    else if (moduleName == "theme-gtk-all")
    {
        Query::Theme gtk2(2, queried_themes, queried_themes_names, "gtk2");
        Query::Theme gtk3(3, queried_themes, queried_themes_names, "gtk3");
        Query::Theme gtk4(4, queried_themes, queried_themes_names, "gtk4");

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo[moduleName].find(moduleMemberName) == sysInfo[moduleName].end())
        {
            switch (moduleMember_hash)
            {
                case "name"_fnv1a16:   SYSINFO_INSERT(get_auto_gtk_format(gtk2.gtk_theme(),      gtk3.gtk_theme(),      gtk4.gtk_theme())); break;
                case "icons"_fnv1a16:  SYSINFO_INSERT(get_auto_gtk_format(gtk2.gtk_icon_theme(), gtk3.gtk_icon_theme(), gtk4.gtk_icon_theme())); break;
                case "font"_fnv1a16:   SYSINFO_INSERT(get_auto_gtk_format(gtk2.gtk_font(),       gtk3.gtk_font(),       gtk4.gtk_font())); break;
            }
        }
    }

    else if (hasStart(moduleName, "theme-gtk"))
    {
        const std::uint8_t ver =
            static_cast<std::uint8_t>(moduleName.length() > 9 ? std::stoi(moduleName.substr(9)) : 0);

        if (ver <= 0)
            die("seems theme-gtk module name '{}' doesn't have a version number to query.\n"
                "Syntax should be like 'theme_gtkN' which N stands for the version of gtk to query (single number)",
                moduleName);

        Query::Theme query_theme(ver, queried_themes, queried_themes_names, fmt::format("gtk{}", ver));

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo[moduleName].find(moduleMemberName) == sysInfo[moduleName].end())
        {
            switch (moduleMember_hash)
            {
                case "name"_fnv1a16:  SYSINFO_INSERT(query_theme.gtk_theme()); break;
                case "icons"_fnv1a16: SYSINFO_INSERT(query_theme.gtk_icon_theme()); break;
                case "font"_fnv1a16:  SYSINFO_INSERT(query_theme.gtk_font()); break;
            }
        }
    }

    else if (moduleName == "cpu")
    {
        Query::CPU query_cpu;

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo[moduleName].find(moduleMemberName) == sysInfo[moduleName].end())
        {
            switch (moduleMember_hash)
            {
                case "cpu"_fnv1a16:
                    SYSINFO_INSERT(
                        fmt::format("{} ({}) @ {:.2f} GHz", query_cpu.name(), query_cpu.nproc(), query_cpu.freq_max()));
                    break;

                case "name"_fnv1a16: SYSINFO_INSERT(query_cpu.name()); break;

                case "nproc"_fnv1a16: SYSINFO_INSERT(query_cpu.nproc()); break;

                case "freq_bios_limit"_fnv1a16: SYSINFO_INSERT(query_cpu.freq_bios_limit()); break;

                case "freq_cur"_fnv1a16: SYSINFO_INSERT(query_cpu.freq_cur()); break;

                case "freq_max"_fnv1a16: SYSINFO_INSERT(query_cpu.freq_max()); break;

                case "freq_min"_fnv1a16: SYSINFO_INSERT(query_cpu.freq_min()); break;
            }
        }
    }

    else if (hasStart(moduleName, "gpu"))
    {
        std::uint16_t id =
            static_cast<std::uint16_t>(moduleName.length() > 3 ? std::stoi(std::string(moduleName).substr(3)) : 0);

        Query::GPU query_gpu(id, queried_gpus);

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo[moduleName].find(moduleMemberName) == sysInfo[moduleName].end())
        {
            if (moduleMemberName == "name")
                SYSINFO_INSERT(query_gpu.name());

            else if (moduleMemberName == "vendor")
                SYSINFO_INSERT(query_gpu.vendor());
        }
    }

    else if (hasStart(moduleName, "disk"))
    {
        if (moduleName.length() < "disk()"_len)
            die("invalid disk module name '{}', must be disk(/path/to/fs) e.g: disk(/)", moduleName);

        enum
        {
            USED = 0,
            TOTAL,
            FREE
        };
        std::string path = moduleName.data();
        path.erase(0, 5);  // disk(
        path.pop_back();   // )
        debug("disk path = {}", path);

        Query::Disk query_disk(path, queried_disks);
        std::array<byte_units_t, 3> byte_units;

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo[moduleName].find(moduleMemberName) == sysInfo[moduleName].end())
        {
            byte_units.at(TOTAL) = auto_devide_bytes(query_disk.total_amount());
            byte_units.at(USED)  = auto_devide_bytes(query_disk.used_amount());
            byte_units.at(FREE)  = auto_devide_bytes(query_disk.free_amount());

            switch (moduleMember_hash)
            {
                case "fs"_fnv1a16:       SYSINFO_INSERT(query_disk.typefs()); break;
                case "device"_fnv1a16:   SYSINFO_INSERT(query_disk.device()); break;
                case "mountdir"_fnv1a16: SYSINFO_INSERT(query_disk.mountdir()); break;

                // clang-format off
                case "disk"_fnv1a16:
                {
                    const std::string& perc = get_and_color_percentage(query_disk.used_amount(), query_disk.total_amount(), 
                                                                        sysInfo, config, colors, parsingLayout);

                    std::string _;
                    SYSINFO_INSERT(fmt::format("{:.2f} {} / {:.2f} {} {} - {}", 
                                               byte_units.at(USED).num_bytes, byte_units.at(USED).unit,
                                               byte_units.at(TOTAL).num_bytes,byte_units.at(TOTAL).unit, 
                                               parse("${0}(" + perc + ")", sysInfo, _, config, colors, parsingLayout),
						query_disk.typefs()));
                } break;
                // clang-format on

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
                                                            sysInfo, config, colors, parsingLayout, true));
                    break;

                case "used_perc"_fnv1a16:
                    SYSINFO_INSERT(get_and_color_percentage(query_disk.used_amount(), query_disk.total_amount(), 
                                                            sysInfo, config, colors, parsingLayout));
                    break;

                case "used-GiB"_fnv1a16: SYSINFO_INSERT(query_disk.used_amount() / 1073741824); break;
                case "used-MiB"_fnv1a16: SYSINFO_INSERT(query_disk.used_amount() / 1048576); break;
                case "used-KiB"_fnv1a16: SYSINFO_INSERT(query_disk.used_amount() / 1024); break;

                case "total-GiB"_fnv1a16: SYSINFO_INSERT(query_disk.total_amount() / 1073741824); break;
                case "total-MiB"_fnv1a16: SYSINFO_INSERT(query_disk.total_amount() / 1048576); break;
                case "total-KiB"_fnv1a16: SYSINFO_INSERT(query_disk.total_amount() / 1024); break;

                case "free-GiB"_fnv1a16: SYSINFO_INSERT(query_disk.free_amount() / 1073741824); break;
                case "free-MiB"_fnv1a16: SYSINFO_INSERT(query_disk.free_amount() / 1048576); break;
                case "free-KiB"_fnv1a16: SYSINFO_INSERT(query_disk.free_amount() / 1024); break;
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

        if (sysInfo[moduleName].find(moduleMemberName) == sysInfo[moduleName].end())
        {
            byte_units.at(FREE)  = auto_devide_bytes(query_ram.swap_free_amount() * 1024);
            byte_units.at(USED)  = auto_devide_bytes(query_ram.swap_used_amount() * 1024);
            byte_units.at(TOTAL) = auto_devide_bytes(query_ram.swap_total_amount() * 1024);

            switch (moduleMember_hash)
            {
                case "swap"_fnv1a16:
                    // clang-format off
                    if (byte_units.at(TOTAL).num_bytes < 1)
                        SYSINFO_INSERT("Disabled");
                    else
                    {
                        const std::string& perc = get_and_color_percentage(query_ram.swap_used_amount(), query_ram.swap_total_amount(), 
                                                                           sysInfo, config, colors, parsingLayout);
                        
                        std::string _;
                        SYSINFO_INSERT(fmt::format("{:.2f} {} / {:.2f} {} {}",
                                                    byte_units.at(USED).num_bytes, byte_units.at(USED).unit,
                                                    byte_units.at(TOTAL).num_bytes,byte_units.at(TOTAL).unit,
                                                    parse("${0}(" + perc + ")", sysInfo, _, config, colors, parsingLayout)));
                    }
                    break;
                    // clang-format on

                case "free"_fnv1a16:
                    SYSINFO_INSERT(
                        fmt::format("{:.2f} {}", byte_units.at(FREE).num_bytes, byte_units.at(FREE).unit));
                    break;

                case "total"_fnv1a16:
                    SYSINFO_INSERT(
                        fmt::format("{:.2f} {}", byte_units.at(TOTAL).num_bytes, byte_units.at(TOTAL).unit));
                    break;

                case "used"_fnv1a16:
                    SYSINFO_INSERT(
                        fmt::format("{:.2f} {}", byte_units.at(USED).num_bytes, byte_units.at(USED).unit));
                    break;

                case "free_perc"_fnv1a16:
                    SYSINFO_INSERT(get_and_color_percentage(query_ram.swap_free_amount(), query_ram.swap_total_amount(), 
                                                            sysInfo, config, colors, parsingLayout, true));
                    break;

                case "used_perc"_fnv1a16:
                    SYSINFO_INSERT(get_and_color_percentage(query_ram.swap_used_amount(), query_ram.swap_total_amount(), 
                                                            sysInfo, config, colors, parsingLayout));
                    break;

                case "free-GiB"_fnv1a16: SYSINFO_INSERT(query_ram.swap_free_amount() / 1048576); break;
                case "free-MiB"_fnv1a16: SYSINFO_INSERT(query_ram.swap_free_amount() / 1024); break;
                case "free-KiB"_fnv1a16: SYSINFO_INSERT(query_ram.swap_free_amount()); break;

                case "used-GiB"_fnv1a16: SYSINFO_INSERT(query_ram.swap_used_amount() / 1048576); break;
                case "used-MiB"_fnv1a16: SYSINFO_INSERT(query_ram.swap_used_amount() / 1024); break;
                case "used-KiB"_fnv1a16: SYSINFO_INSERT(query_ram.swap_used_amount()); break;

                case "total-GiB"_fnv1a16: SYSINFO_INSERT(query_ram.swap_total_amount() / 1048576); break;
                case "total-MiB"_fnv1a16: SYSINFO_INSERT(query_ram.swap_total_amount() / 1024); break;
                case "total-KiB"_fnv1a16: SYSINFO_INSERT(query_ram.swap_total_amount()); break;
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

        if (sysInfo[moduleName].find(moduleMemberName) == sysInfo[moduleName].end())
        {
            byte_units.at(USED)  = auto_devide_bytes(query_ram.used_amount() * 1024);
            byte_units.at(TOTAL) = auto_devide_bytes(query_ram.total_amount() * 1024);
            byte_units.at(FREE)  = auto_devide_bytes(query_ram.free_amount() * 1024);

            switch (moduleMember_hash)
            {
                case "ram"_fnv1a16:
                {
                    const std::string& perc = get_and_color_percentage(query_ram.used_amount(), query_ram.total_amount(),
                                                                        sysInfo, config, colors, parsingLayout);
                    
                    std::string _;
                    // clang-format off
                    SYSINFO_INSERT(fmt::format("{:.2f} {} / {:.2f} {} {}",
                                               byte_units.at(USED).num_bytes, byte_units.at(USED).unit,
                                               byte_units.at(TOTAL).num_bytes,byte_units.at(TOTAL).unit,
                                               parse("${0}(" + perc + ")", sysInfo, _, config, colors, parsingLayout)));
                    break;
                    // clang-format on
                }
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
                    SYSINFO_INSERT(get_and_color_percentage(query_ram.free_amount(), query_ram.total_amount(),
                                                            sysInfo, config, colors, parsingLayout, true));
                    break;

                case "used_perc"_fnv1a16:
                    SYSINFO_INSERT(get_and_color_percentage(query_ram.used_amount(), query_ram.total_amount(),
                                                            sysInfo, config, colors, parsingLayout));
                    break;

                case "used-GiB"_fnv1a16: SYSINFO_INSERT(query_ram.used_amount() / 1048576); break;
                case "used-MiB"_fnv1a16: SYSINFO_INSERT(query_ram.used_amount() / 1024); break;
                case "used-KiB"_fnv1a16: SYSINFO_INSERT(query_ram.used_amount()); break;

                case "total-GiB"_fnv1a16: SYSINFO_INSERT(query_ram.total_amount() / 1048576); break;
                case "total-MiB"_fnv1a16: SYSINFO_INSERT(query_ram.total_amount() / 1024); break;
                case "total-KiB"_fnv1a16: SYSINFO_INSERT(query_ram.total_amount()); break;

                case "free-GiB"_fnv1a16: SYSINFO_INSERT(query_ram.free_amount() / 1048576); break;
                case "free-MiB"_fnv1a16: SYSINFO_INSERT(query_ram.free_amount() / 1024); break;
                case "free-KiB"_fnv1a16: SYSINFO_INSERT(query_ram.free_amount()); break;
            }
        }
    }

    else
        die("Invalid module name: {}", moduleName);
}
