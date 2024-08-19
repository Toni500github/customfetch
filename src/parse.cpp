#include "parse.hpp"

#include <unistd.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <string>
#include <vector>

#include "config.hpp"
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

static std::array<std::string, 3> get_ansi_color(const std::string& str, const colors_t& colors)
{
    if (hasStart(str, "38") || hasStart(str, "48"))
        die("Can't convert \\e[38; or \\e[48; codes in GUI. Please use #hexcode colors instead.");

    size_t first_m = str.find('m');
    if (first_m == std::string::npos)
        die("Parser: failed to parse layout/ascii art: missing m while using ANSI color escape code");

    std::string col = str;
    col.erase(first_m);  // 1;42
    std::string weight = hasStart(col, "1;") ? "bold" : "normal";
    std::string type   = "fgcolor";  // either fgcolor or bgcolor

    if (hasStart(col, "1;") || hasStart(col, "0;"))
        col.erase(0, 2);

    debug("col = {}", col);
    int n = std::stoi(col);

    // unfortunatly you can't do bold and light in pango
    if ((n >= 100 && n <= 107) || (n >= 90 && n <= 97))
        weight = "light";

    if ((n >= 100 && n <= 107) || (n >= 40 && n <= 47))
        type = "bgcolor";

    // last number
    // https://stackoverflow.com/a/5030086
    n = col.back() - '0';

    switch (n)
    {
        case 0: col = colors.gui_black;   break;
        case 1: col = colors.gui_red;     break;
        case 2: col = colors.gui_green;   break;
        case 3: col = colors.gui_yellow;  break;
        case 4: col = colors.gui_blue;    break;
        case 5: col = colors.gui_magenta; break;
        case 6: col = colors.gui_cyan;    break;
        case 7: col = colors.gui_white;   break;
    }

    if (col[0] != '#')
        col.erase(0, col.find('#'));

    return { col, weight, type };
}

static const std::string& check_gui_ansi_clr(const std::string& str)
{
    if (hasStart(str, "\033") || hasStart(str, "\\e"))
        die("GUI colors can't be in ANSI escape sequence");

    return str;
}

std::string getInfoFromName(const systemInfo_t& systemInfo, const std::string_view moduleName,
                                   const std::string_view moduleValueName)
{
    if (const auto& it1 = systemInfo.find(moduleName.data()); it1 != systemInfo.end())
    {
        if (const auto& it2 = it1->second.find(moduleValueName.data()); it2 != it1->second.end())
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
                  colors_t& colors, bool parsingLaoyut)
{
    std::string output = input.data();
    pureOutput         = output;

    size_t dollarSignIndex = 0;
    size_t oldDollarSignIndex = 0;
    bool   start           = false;
    static std::vector<std::string> auto_colors;

    if (!config.sep_reset.empty() && parsingLaoyut)
    {
        replace_str(output, config.sep_reset, "${0}" + config.sep_reset);
        replace_str(pureOutput, config.sep_reset, "${0}" + config.sep_reset);
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
            dollarSignIndex = output.find('$', dollarSignIndex+1);
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
            case '{': type = '}'; break;
            default:  // neither of them
                break;
        }

        if (type == ' ')
            continue;

        for (size_t i = dollarSignIndex + 2; i < output.size(); i++)
        {
            if (output.at(i) == type && output[i - 1] != '\\')
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

        std::string strToRemove = fmt::format("${}{}{}", opentag, command, type);
        size_t      start_pos   = pureOutput.find(strToRemove);
        if (start_pos != std::string::npos)
            pureOutput.erase(start_pos, strToRemove.length());

        switch (type)
        {
            case ')':
                output = output.replace(dollarSignIndex, (endBracketIndex + 1) - dollarSignIndex, shell_exec(command));
                break;
            case '>':
            {
                const size_t      dot_pos = command.find('.');
                if (dot_pos == std::string::npos)
                    die("module name '{}' doesn't have a dot '.' for separiting module name and submodule", command);

                const std::string moduleName(command.substr(0, dot_pos));
                const std::string moduleValueName(command.substr(dot_pos + 1));
                addValueFromModule(systemInfo, moduleName, moduleValueName, config);

                output = output.replace(dollarSignIndex, (endBracketIndex + 1) - dollarSignIndex,
                                        getInfoFromName(systemInfo, moduleName, moduleValueName));
            }
            break;
            case '}':  // please pay very attention when reading this unreadable code
                if (hasStart(command, "auto"))
                {
                    std::uint8_t ver = static_cast<std::uint8_t>(command.length() > 4 ? std::stoi(command.substr(4)) - 1 : 0);
                    if (ver >= auto_colors.size() || ver < 1)
                        ver = 0;

                    if (auto_colors.empty())
                        auto_colors.push_back("\033[0m\033[1m");

                    output = output.replace(dollarSignIndex, (endBracketIndex + 1) - dollarSignIndex,
                                            auto_colors.at(ver));
                }

                else if (command == "1")
                    output = output.replace(dollarSignIndex, (endBracketIndex + 1) - dollarSignIndex,
                                            config.gui ? "</span><span weight='bold'>" : fmt::format("{}\033[1m", NOCOLOR)); 
                else if (command == "0")
                    output = output.replace(dollarSignIndex, (endBracketIndex + 1) - dollarSignIndex,
                                            config.gui ? "</span><span>" : NOCOLOR);
                else
                {
                    std::string str_clr;
                    if (config.gui)
                    {
                        // if at end there a '$', it will make the end output "$</span>" and so it will confuse addValueFromModule()
                        // and so let's make it "$ </span>"
                        // this geniunenly stupid
                        if (output.back() == '$')
                                output += ' ';

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

                        if (str_clr[0] == '!' && str_clr[1] == '#')
                        {
                            //debug("output.substr(endBracketIndex + 1) = {}", output.substr(endBracketIndex + 1));
                            output = output.replace(dollarSignIndex, output.length() - dollarSignIndex,
                                                    fmt::format("<span fgcolor='{}' weight='bold'>{}</span>",
                                                                str_clr.substr(1), output.substr(endBracketIndex + 1)));
                            
                        }

                        else if (str_clr[0] == '#')
                        {
                            output = output.replace(dollarSignIndex, output.length() - dollarSignIndex,
                                                    fmt::format("<span fgcolor='{}'>{}</span>", 
                                                                str_clr, output.substr(endBracketIndex + 1)));
                        }

                        else if (hasStart(str_clr, "\\e") ||
                                 hasStart(str_clr,
                                          "\033"))  // "\\e" is for checking in the ascii_art, \033 in the config
                        {
                            std::array<std::string, 3> clrs = get_ansi_color((hasStart(str_clr, "\033") ? str_clr.substr(2)
                                                                                                        : str_clr.substr(3)),
                                                                                                        colors);
                            const std::string_view color  = clrs.at(0);
                            const std::string_view weight = clrs.at(1);
                            const std::string_view type   = clrs.at(2);
                            output = output.replace(dollarSignIndex, output.length() - dollarSignIndex,
                                                    fmt::format("<span {}='{}' weight='{}'>{}</span>", type, color, weight,
                                                    output.substr(endBracketIndex + 1)));
                        }

                        else
                            error("PARSER: failed to parse line with color '{}'", str_clr);
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
                        std::string unformatted_replacement_string;

                        if (str_clr[0] == '!' && str_clr[1] == '#')
                        {
                            fmt::rgb clr                   = hexStringToColor(str_clr.substr(1));
                            unformatted_replacement_string = output.substr(endBracketIndex + 1);
                            formatted_replacement_string =
                                fmt::format(fmt::fg(clr) | fmt::emphasis::bold, "{}", unformatted_replacement_string);
                        }

                        else if (str_clr[0] == '#')
                        {
                            fmt::rgb clr                   = hexStringToColor(str_clr);
                            unformatted_replacement_string = output.substr(endBracketIndex + 1);
                            formatted_replacement_string =
                                fmt::format(fmt::fg(clr), "{}", unformatted_replacement_string);
                        }

                        else if (hasStart(str_clr, "\\e") || hasStart(str_clr, "\033"))
                        {
                            unformatted_replacement_string = output.substr(endBracketIndex + 1);
                            formatted_replacement_string =
                                fmt::format("\x1B[{}{}",
                                            // "\\e" is for checking in the ascii_art, \033 in the config
                                            hasStart(str_clr, "\033")
                                                ? str_clr.substr(2)
                                                : str_clr.substr(3),
                                            unformatted_replacement_string);
                        }

                        else
                            error("PARSER: failed to parse line with color '{}'", str_clr);

                        output = output.replace(dollarSignIndex, output.length() - dollarSignIndex,
                                                formatted_replacement_string);
                    }

                    if (!parsingLaoyut && std::find(auto_colors.begin(), auto_colors.end(), str_clr) == auto_colors.end())
                        auto_colors.push_back(str_clr);
                }
        }
    }

    // https://github.com/dunst-project/dunst/issues/900
    // pango markup doesn't like '<' if it's not a tag
    // workaround: just put "\<" in the config, e.g "$<os.kernel> \<- Kernel"
    // "But.. what if I want '<<<<<-' " just put \ on each one of < :D
    // sorry, not my problem, but pangos
    if (config.gui)
        replace_str(output, "\\<", "&lt;");
    else
        replace_str(output, "\\<", "<");

    return output;
}

static std::string get_auto_uptime(unsigned short days, unsigned short hours, unsigned short mins, unsigned short secs,
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

static std::string get_auto_gtk_format(const std::string_view gtk2, const std::string_view gtk3, const std::string_view gtk4)
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

void addValueFromModule(systemInfo_t& sysInfo, const std::string& moduleName, const std::string& moduleValueName,
                        const Config& config)
{
#define SYSINFO_INSERT(x) sysInfo[moduleName].insert({ moduleValueName, variant(x) })
    // yikes, here we go.
    auto moduleValue_hash = fnv1a16::hash(moduleValueName);
    static std::vector<std::uint16_t> queried_gpus;
    static std::vector<std::string_view> queried_disks;
    static std::vector<std::string> queried_themes_names;
    static systemInfo_t queried_themes;

    if (moduleName == "os")
    {
        Query::System query_system;

        std::chrono::seconds uptime_secs(query_system.uptime());
        auto uptime_mins  = std::chrono::duration_cast<std::chrono::minutes>(uptime_secs);
        auto uptime_hours = std::chrono::duration_cast<std::chrono::hours>(uptime_secs);
        auto uptime_days  = std::chrono::duration_cast<std::chrono::days>(uptime_secs);

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo[moduleName].find(moduleValueName) == sysInfo[moduleName].end())
        {
            switch (moduleValue_hash)
            {
                case "name"_fnv1a16: SYSINFO_INSERT(query_system.os_pretty_name()); break;

                case "uptime"_fnv1a16:
                    SYSINFO_INSERT(get_auto_uptime(uptime_days.count(), uptime_hours.count() % 24,
                                                   uptime_mins.count() % 60, uptime_secs.count() % 60, config));
                    break;

                case "uptime_secs"_fnv1a16: SYSINFO_INSERT(static_cast<size_t>(uptime_secs.count() % 60)); break;

                case "uptime_mins"_fnv1a16: SYSINFO_INSERT(static_cast<size_t>(uptime_mins.count() % 60)); break;

                case "uptime_hours"_fnv1a16: SYSINFO_INSERT(static_cast<size_t>(uptime_hours.count()) % 24); break;

                case "uptime_days"_fnv1a16: SYSINFO_INSERT(static_cast<size_t>(uptime_days.count())); break;

                case "kernel"_fnv1a16:
                    SYSINFO_INSERT(query_system.kernel_name() + ' ' + query_system.kernel_version());
                    break;

                case "kernel_name"_fnv1a16: SYSINFO_INSERT(query_system.kernel_name()); break;

                case "kernel_version"_fnv1a16: SYSINFO_INSERT(query_system.kernel_version()); break;

                case "pkgs"_fnv1a16: SYSINFO_INSERT(query_system.pkgs_installed(config)); break;

                case "initsys_name"_fnv1a16: SYSINFO_INSERT(query_system.os_initsys_name()); break;

                case "hostname"_fnv1a16: SYSINFO_INSERT(query_system.hostname()); break;
            }
        }
    }

    else if (moduleName == "system")
    {
        Query::System query_system;

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo[moduleName].find(moduleValueName) == sysInfo[moduleName].end())
        {
            switch (moduleValue_hash)
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

    else if (moduleName == "user")
    {
        Query::User query_user;

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo[moduleName].find(moduleValueName) == sysInfo[moduleName].end())
        {
            switch (moduleValue_hash)
            {
                case "name"_fnv1a16: SYSINFO_INSERT(query_user.name()); break;

                case "shell"_fnv1a16:
                    SYSINFO_INSERT(query_user.shell_name() + ' ' + query_user.shell_version(query_user.shell_name()));
                    break;

                case "shell_name"_fnv1a16: SYSINFO_INSERT(query_user.shell_name()); break;

                case "shell_path"_fnv1a16: SYSINFO_INSERT(query_user.shell_path()); break;

                case "shell_version"_fnv1a16: SYSINFO_INSERT(query_user.shell_version(query_user.shell_name())); break;

                case "de_name"_fnv1a16:
                    SYSINFO_INSERT(
                        query_user.de_name(query_user.m_bDont_query_dewm, query_user.term_name(),
                                           query_user.wm_name(query_user.m_bDont_query_dewm, query_user.term_name())));
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
                    SYSINFO_INSERT(prettify_term_name(query_user.term_name()) + ' ' + query_user.term_version(query_user.term_name()));
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

        if (sysInfo[moduleName].find(moduleValueName) == sysInfo[moduleName].end())
        {
            switch (moduleValue_hash)
            {
                case "cursor"_fnv1a16: SYSINFO_INSERT(query_theme.cursor()); break;
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

        if (sysInfo[moduleName].find(moduleValueName) == sysInfo[moduleName].end())
        {
            switch (moduleValue_hash)
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

        if (sysInfo[moduleName].find(moduleValueName) == sysInfo[moduleName].end())
        {
            switch (moduleValue_hash)
            {
                case "name"_fnv1a16:   SYSINFO_INSERT(query_theme.gtk_theme()); break;
                case "icons"_fnv1a16:  SYSINFO_INSERT(query_theme.gtk_icon_theme()); break;
                case "font"_fnv1a16:   SYSINFO_INSERT(query_theme.gtk_font()); break;
            }
        }
    }

    else if (moduleName == "cpu")
    {
        Query::CPU query_cpu;

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo[moduleName].find(moduleValueName) == sysInfo[moduleName].end())
        {
            switch (moduleValue_hash)
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

        if (sysInfo[moduleName].find(moduleValueName) == sysInfo[moduleName].end())
        {
            if (moduleValueName == "name")
                SYSINFO_INSERT(query_gpu.name());

            else if (moduleValueName == "vendor")
                SYSINFO_INSERT(query_gpu.vendor());
        }
    }

    else if (hasStart(moduleName, "disk"))
    {
        if (moduleName.length() < 7)
            die(" PARSER: invalid disk module name ({}), must be disk(/path/to/fs) e.g: disk(/)", moduleName);

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

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo[moduleName].find(moduleValueName) == sysInfo[moduleName].end())
        {
            std::array<byte_units_t, 3> byte_units;
            byte_units.at(TOTAL) = auto_devide_bytes(query_disk.total_amount());
            byte_units.at(USED)  = auto_devide_bytes(query_disk.used_amount());
            byte_units.at(FREE)  = auto_devide_bytes(query_disk.free_amount());

            switch (moduleValue_hash)
            {
                    // clang-format off
                case "disk"_fnv1a16:
                    SYSINFO_INSERT(fmt::format("{:.2f} {} / {:.2f} {} - {}", 
                                               byte_units.at(USED).num_bytes, byte_units.at(USED).unit,
                                               byte_units.at(TOTAL).num_bytes,byte_units.at(TOTAL).unit, 
                                               query_disk.typefs()));
                    break;
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

                case "used-GiB"_fnv1a16: SYSINFO_INSERT(query_disk.used_amount() / 1073741824); break;
                case "used-MiB"_fnv1a16: SYSINFO_INSERT(query_disk.used_amount() / 1048576); break;

                case "total-GiB"_fnv1a16: SYSINFO_INSERT(query_disk.total_amount() / 1073741824); break;
                case "total-MiB"_fnv1a16: SYSINFO_INSERT(query_disk.total_amount() / 1048576); break;

                case "free-GiB"_fnv1a16: SYSINFO_INSERT(query_disk.free_amount() / 1073741824); break;
                case "free-MiB"_fnv1a16: SYSINFO_INSERT(query_disk.free_amount() / 1048576); break;

                case "fs"_fnv1a16: SYSINFO_INSERT(query_disk.typefs()); break;
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
            SWAP_FREE,
            SWAP_USED,
            SWAP_TOTAL
        };
        std::array<byte_units_t, 6> byte_units;

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert({ moduleName, {} });

        if (sysInfo[moduleName].find(moduleValueName) == sysInfo[moduleName].end())
        {
            byte_units.at(USED)       = auto_devide_bytes(query_ram.used_amount() * 1024);
            byte_units.at(TOTAL)      = auto_devide_bytes(query_ram.total_amount() * 1024);
            byte_units.at(FREE)       = auto_devide_bytes(query_ram.free_amount() * 1024);
            byte_units.at(SWAP_FREE)  = auto_devide_bytes(query_ram.swap_free_amount() * 1024);
            byte_units.at(SWAP_USED)  = auto_devide_bytes(query_ram.swap_used_amount() * 1024);
            byte_units.at(SWAP_TOTAL) = auto_devide_bytes(query_ram.swap_total_amount() * 1024);

            switch (moduleValue_hash)
            {
                case "ram"_fnv1a16:
                    // clang-format off
                    SYSINFO_INSERT(fmt::format("{:.2f} {} / {:.2f} {}", 
                                               byte_units.at(USED).num_bytes, byte_units.at(USED).unit, 
                                               byte_units.at(TOTAL).num_bytes,byte_units.at(TOTAL).unit));
                    break;
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

                case "swap"_fnv1a16:
                    // clang-format off
                    SYSINFO_INSERT(fmt::format("{:.2f} {} / {:.2f} {}", 
                                               byte_units.at(SWAP_USED).num_bytes, byte_units.at(SWAP_USED).unit, 
                                               byte_units.at(SWAP_TOTAL).num_bytes,byte_units.at(SWAP_TOTAL).unit));
                    break;
                    // clang-format on

                case "swap_free"_fnv1a16:
                    SYSINFO_INSERT(
                        fmt::format("{:.2f} {}", byte_units.at(SWAP_FREE).num_bytes, byte_units.at(SWAP_FREE).unit));
                    break;

                case "swap_total"_fnv1a16:
                    SYSINFO_INSERT(
                        fmt::format("{:.2f} {}", byte_units.at(SWAP_TOTAL).num_bytes, byte_units.at(SWAP_TOTAL).unit));
                    break;

                case "swap_used"_fnv1a16:
                    SYSINFO_INSERT(
                        fmt::format("{:.2f} {}", byte_units.at(SWAP_USED).num_bytes, byte_units.at(SWAP_USED).unit));
                    break;

                case "used-GiB"_fnv1a16: SYSINFO_INSERT(query_ram.used_amount() / 1048576); break;
                case "used-MiB"_fnv1a16: SYSINFO_INSERT(query_ram.used_amount() / 1024); break;

                case "total-GiB"_fnv1a16: SYSINFO_INSERT(query_ram.total_amount() / 1048576); break;
                case "total-MiB"_fnv1a16: SYSINFO_INSERT(query_ram.total_amount() / 1024); break;

                case "free-GiB"_fnv1a16: SYSINFO_INSERT(query_ram.free_amount() / 1048576); break;
                case "free-MiB"_fnv1a16: SYSINFO_INSERT(query_ram.free_amount() / 1024); break;

                case "swap_free-GiB"_fnv1a16: SYSINFO_INSERT(query_ram.swap_free_amount() / 1048576); break;
                case "swap_free-MiB"_fnv1a16: SYSINFO_INSERT(query_ram.swap_free_amount() / 1024); break;

                case "swap_used-GiB"_fnv1a16: SYSINFO_INSERT(query_ram.swap_used_amount() / 1048576); break;
                case "swap_used-MiB"_fnv1a16: SYSINFO_INSERT(query_ram.swap_used_amount() / 1024); break;

                case "swap_total-GiB"_fnv1a16: SYSINFO_INSERT(query_ram.swap_total_amount() / 1048576); break;
                case "swap_total-MiB"_fnv1a16: SYSINFO_INSERT(query_ram.swap_total_amount() / 1024); break;
            }
        }
    }
    else
        die("Invalid module name: {}", moduleName);
}
