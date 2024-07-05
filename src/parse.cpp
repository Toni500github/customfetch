#include <unistd.h>
#include <array>
#include <chrono>
#include <string>

#include "parse.hpp"
#include "query.hpp"
#include "config.hpp"
#include "switch_fnv1a.hpp"
#include "util.hpp"

// using namespace Query;
std::array<std::string, 5> Query::System::m_os_release_vars;
struct utsname Query::System::m_uname_infos;
struct sysinfo Query::System::m_sysInfos;
struct passwd* Query::User::m_pPwd;
std::array<std::string, 3> Query::CPU::m_cpu_infos_str;
std::array<float, 4> Query::CPU::m_cpu_infos_t;
struct statvfs Query::Disk::m_statvfs;
std::string Query::Disk::m_typefs;
std::array<std::string, 6> Query::User::m_users_infos;
std::array<size_t, 6> Query::RAM::m_memory_infos;
std::array<std::string, 2> Query::GPU::m_gpu_infos;

bool Query::System::m_bInit = false;
bool Query::RAM::m_bInit = false;
bool Query::CPU::m_bInit = false;
bool Query::GPU::m_bInit = false;
bool Query::Disk::m_bInit = false;
bool Query::User::m_bInit = false;

static std::array<std::string, 3> get_ansi_color( const std::string_view str, colors_t& colors )
{
#define bgcolor "bgcolor"

    auto first_m = str.find( "m" );
    if ( first_m == std::string::npos )
        die( "Parser: failed to parse layout/ascii art: missing m while using ANSI color escape code" );

    std::string col = str.data();
    col.erase( first_m ); // 1;42
    std::string weight = hasStart( col, "1;" ) ? "bold" : "normal";
    std::string type   = "fgcolor"; // either fgcolor or bgcolor

    if ( hasStart(col, "1;") || hasStart(col, "0;") )
        col.erase(0, 2);
    
    debug("col = {}", col);
    int n = std::stoi( col );

    // copy paste ahh code
    // unfortunatly you can't do bold and light in pango
    switch (n) {
        case 90:
        case 30: col = colors.gui_black; 
            break;
        
        case 91:
        case 31: col = colors.gui_red; 
            break;
        
        case 92:
        case 32: col = colors.gui_green; 
            break;
        
        case 93:
        case 33: col = colors.gui_yellow; 
            break;
        
        case 94:
        case 34: col = colors.gui_blue; 
            break;

        case 95:
        case 35: col = colors.gui_magenta; 
            break;
        
        case 96:
        case 36: col = colors.gui_cyan; 
            break;
        
        case 97:
        case 37: col = colors.gui_white; 
            break;

        case 100:
        case 40: col = colors.gui_black;  type = bgcolor; break;
        
        case 101:
        case 41: col = colors.gui_red;    type = bgcolor; break;
        
        case 102:
        case 42: col = colors.gui_green;  type = bgcolor; break;
        
        case 103:
        case 43: col = colors.gui_yellow; type = bgcolor; break;
        
        case 104:
        case 44: col = colors.gui_blue;   type = bgcolor; break;
        
        case 105:
        case 45: col = colors.gui_magenta; type = bgcolor; break;
        
        case 106:
        case 46: col = colors.gui_cyan;   type = bgcolor; break;
        
        case 107:
        case 47: col = colors.gui_white;  type = bgcolor; break;
    }

    if ( col[0] != '#' )
        col.erase(0, col.find("#"));

    return { col, weight, type };
}

static std::string check_gui_ansi_clr(std::string& str) {
    if (hasStart(str, "\033") || hasStart(str, "\\e"))
        die("GUI colors can't be in ANSI escape sequence");

    return str;
}

static std::string getInfoFromName( const systemInfo_t& systemInfo, const std::string& name )
{
    const std::vector<std::string> sections = split( name, '.' );
    
    if (const auto it1 = systemInfo.find(sections[0]); it1 != systemInfo.end())
    {
        if (const auto it2 = it1->second.find(sections[1]); it2 != it1->second.end()) 
        {
            const variant& result = it2->second;

            if ( std::holds_alternative<size_t>(result) )
                return fmt::to_string(std::get<size_t>(result));

            else if ( std::holds_alternative<float>(result) )
                return fmt::format("{:.2f}", (std::get<float>(result)));

            else
                return std::get<std::string>(result);
        }
    }

    return "(unknown/invalid component)";;

}

static std::string _parse( const std::string& input, const systemInfo_t& systemInfo, std::string& pureOutput, const Config& config, colors_t& colors, bool parsingLaoyut )
{
    std::string output = input;
    pureOutput = output;

    size_t dollarSignIndex = 0;
    bool start = false;

    if (!config.sep_reset.empty() && parsingLaoyut) {
        size_t pos = output.find(config.sep_reset);
        if (pos != std::string::npos) {
            replace_str(output, config.sep_reset, "${0}" + config.sep_reset);
            replace_str(pureOutput, config.sep_reset, "${0}" + config.sep_reset);
        }
    }

    while ( true )
    {
        size_t oldDollarSignIndex = dollarSignIndex;
        dollarSignIndex           = output.find( '$', dollarSignIndex );

        if ( dollarSignIndex == std::string::npos || ( dollarSignIndex <= oldDollarSignIndex && start ) )
            break;

        start = true;

        // check for bypass
        // YOU CAN USE AND/NOT IN C++????
        // btw the second part checks if it has a \ before it and NOT a \ before the backslash, (check for escaped
        // backslash) example: \$ is bypassed, \\$ is NOT bypassed. this will not make an effort to check multiple
        // backslashes, thats your fault atp.
        if ( dollarSignIndex > 0 and ( output[dollarSignIndex - 1] == '\\' and
                                       ( dollarSignIndex == 1 or output[dollarSignIndex - 2] != '\\' ) ) )
            continue;

        std::string command         = "";
        size_t      endBracketIndex = -1;

        char type = ' ';  // ' ' = undefined, ')' = shell exec, 2 = ')' asking for a module
        char opentag = output[dollarSignIndex + 1];

        switch ( opentag )
        {
            case '(':
                type = ')';
                break;
            case '<':
                type = '>';
                break;
            case '{':
                type = '}';
                break;
            default:  // neither of them
                break;
        }

        if ( type == ' ' )
            continue;

        for ( size_t i = dollarSignIndex + 2; i < output.size(); i++ )
        {
            if ( output.at(i) == type && output[i - 1] != '\\' )
            {
                endBracketIndex = i;
                break;
            }
            else if ( output.at(i) == type )
                command.pop_back();

            command += output.at(i);
        }

        if ( static_cast<int>(endBracketIndex) == -1 )
            die( "PARSER: Opened tag is not closed at index {} in string {}", dollarSignIndex, output );

        std::string strToRemove = fmt::format("${}{}{}", opentag, command, type);
        size_t start_pos = 0;
        if((start_pos = pureOutput.find(strToRemove)) != std::string::npos) {
            pureOutput.erase(start_pos, strToRemove.length());
        }
        
        switch ( type )
        {
        case ')':
            output =
                output.replace( dollarSignIndex, ( endBracketIndex + 1 ) - dollarSignIndex, shell_exec( command ) );
            break;
        case '>':
            output = output.replace( dollarSignIndex, ( endBracketIndex + 1 ) - dollarSignIndex,
                                     getInfoFromName( systemInfo, command ) );
            break;
        case '}':  // please pay very attention when reading this unreadable code
            if ( command == "0" ) {   
                output = output.replace( dollarSignIndex, ( endBracketIndex + 1 ) - dollarSignIndex, config.gui ? "</span><span>" : NOCOLOR );
            }
            else
            {
                std::string str_clr;
                if ( config.gui )
                {
                    switch ( fnv1a32::hash(command) )
                    {
                        case "black"_fnv1a32:       str_clr = check_gui_ansi_clr(colors.gui_black);  break;   
                        case "red"_fnv1a32:         str_clr = check_gui_ansi_clr(colors.gui_red);    break;
                        case "blue"_fnv1a32:        str_clr = check_gui_ansi_clr(colors.gui_blue);   break;
                        case "green"_fnv1a32:       str_clr = check_gui_ansi_clr(colors.gui_green);  break;
                        case "cyan"_fnv1a32:        str_clr = check_gui_ansi_clr(colors.gui_cyan);   break;
                        case "yellow"_fnv1a32:      str_clr = check_gui_ansi_clr(colors.gui_yellow); break;
                        case "magenta"_fnv1a32:     str_clr = check_gui_ansi_clr(colors.gui_magenta);break;
                        case "white"_fnv1a32:       str_clr = check_gui_ansi_clr(colors.gui_white);  break;
                        default:
                            str_clr = command;
                            break;
                    }

                    if ( str_clr[0] == '!' && str_clr[1] == '#' )
                        output = output.replace(
                            dollarSignIndex, output.length() - dollarSignIndex,
                            fmt::format( "<span fgcolor='{}' weight='bold'>{}</span>", str_clr.replace( 0, 1, "" ),
                                         output.substr( endBracketIndex + 1 ) ) );

                    else if ( str_clr[0] == '#' )
                        output = output.replace( dollarSignIndex, output.length() - dollarSignIndex,
                                                 fmt::format( "<span fgcolor='{}'>{}</span>", str_clr,
                                                              output.substr( endBracketIndex + 1 ) ) );

                    else if ( hasStart(str_clr, "\\e") ||
                              hasStart(str_clr, "\033") )  // "\\e" is for checking in the ascii_art, \033 in the config
                    {
                        std::array<std::string, 3> clrs = get_ansi_color( (hasStart( str_clr, "\033" ) ? std::string_view(str_clr.substr(2)) : std::string_view(str_clr.substr(3))), colors );
                        std::string_view color  = clrs.at( 0 );
                        std::string_view weight = clrs.at( 1 );
                        std::string_view type   = clrs.at( 2 );
                        output = output.replace( dollarSignIndex, output.length() - dollarSignIndex,
                                                 fmt::format( "<span {}='{}' weight='{}'>{}</span>", type, color, weight, 
                                                 output.substr( endBracketIndex + 1 ) ) );
                    }

                    else
                        error( "PARSER: failed to parse line with color '{}'", str_clr );

                }

                else
                {
                    switch ( fnv1a32::hash(command) )
                    {
                        case "black"_fnv1a32:       str_clr = colors.black; break;
                        case "red"_fnv1a32:         str_clr = colors.red; break;
                        case "blue"_fnv1a32:        str_clr = colors.blue;break;
                        case "green"_fnv1a32:       str_clr = colors.green; break;
                        case "cyan"_fnv1a32:        str_clr = colors.cyan; break;
                        case "yellow"_fnv1a32:      str_clr = colors.yellow; break;
                        case "magenta"_fnv1a32:     str_clr = colors.magenta; break;
                        case "white"_fnv1a32:       str_clr = colors.white; break;
                        default:
                            str_clr = command;
                            break;
                    }

                    std::string formatted_replacement_string;
                    std::string unformatted_replacement_string;

                    if ( str_clr[0] == '!' && str_clr[1] == '#' )
                    {
                        fmt::rgb clr = hexStringToColor( str_clr.replace( 0, 1, "" ) );
                        unformatted_replacement_string = output.substr( endBracketIndex + 1 );
                        formatted_replacement_string = fmt::format( fmt::fg( clr ) | fmt::emphasis::bold, "{}",
                                                unformatted_replacement_string);
                    }

                    else if ( str_clr[0] == '#' )
                    {
                        fmt::rgb clr = hexStringToColor( str_clr );
                        unformatted_replacement_string = output.substr( endBracketIndex + 1 );
                        formatted_replacement_string = fmt::format( fmt::fg( clr ), "{}", 
                                            unformatted_replacement_string );
                    }

                    else if ( hasStart( str_clr, "\\e" ) || hasStart( str_clr, "\033" ) )
                    {
                        unformatted_replacement_string = output.substr( endBracketIndex + 1 );
                        formatted_replacement_string = fmt::format( "\x1B[{}{}",
                                         hasStart( str_clr, "\033" ) ? // "\\e" is for checking in the ascii_art, \033 in the config
                                         str_clr.substr(2) : str_clr.substr(3),
                                         unformatted_replacement_string );
                    }

                    else
                        die( "PARSER: failed to parse line with color '{}'", str_clr );
                
                    output = output.replace( dollarSignIndex, output.length() - dollarSignIndex, formatted_replacement_string);
                }
            }
            break;
        }
    }

    return output;
}

static std::string get_auto_uptime(size_t mins, size_t hours) {
    std::string ret;
    size_t local_mins = mins%60;
    
    // shut up pls
    if (hours != 0 && local_mins != 0)
        ret += fmt::format("{} hours, ", hours);
    else if (local_mins == 0)
        ret += fmt::format("{} hours", hours);

    if (local_mins != 0)
        ret += fmt::format("{} mins", local_mins);

    return ret;
}

std::string parse(const std::string& input, const systemInfo_t& systemInfo, std::string& pureOutput, const Config& config, colors_t& colors, bool parsingLaoyut) {
    return _parse(input, systemInfo, pureOutput, config, colors, parsingLaoyut);
}
std::string parse(const std::string& input, const systemInfo_t& systemInfo, const Config& config, colors_t& colors, bool parsingLaoyut) {
    std::string _;
    return _parse(input, systemInfo, _, config, colors, parsingLaoyut);
}

void addModuleValues(systemInfo_t& sysInfo, const std::string_view moduleName) {
    // yikes, here we go.

    if (moduleName == "os" || moduleName == "system") {
        Query::System query_system;
    
        std::chrono::seconds uptime_secs(query_system.uptime());
        auto uptime_mins = std::chrono::duration_cast<std::chrono::minutes>(uptime_secs);
        auto uptime_hours = std::chrono::duration_cast<std::chrono::hours>(uptime_secs);

        // stupid way to not construct query_system twice, more faster 
        if (moduleName == "system") {
            sysInfo.insert(
                {"system", {
                    {"host",         variant(fmt::format("{} {}", query_system.host_vendor(), query_system.host_modelname()))},
                    {"host_name",    variant(query_system.host_modelname())},
                    {"host_vendor",  variant(query_system.host_vendor())},
                    {"host_version", variant(query_system.host_version())},
                    
                    {"arch",         variant(query_system.arch())},
                }}
            );
        }
            
        else { 
            sysInfo.insert(
                {"os", {
                    {"name",           variant(query_system.os_pretty_name())},
                    {"version_id",     variant(query_system.os_versionid())},
                    {"version_codename", variant(query_system.os_version_codename())},
                    
                    {"uptime",         variant(get_auto_uptime(uptime_mins.count(), uptime_hours.count()))},
                    {"uptime_secs",    variant(static_cast<size_t>(uptime_secs.count()%60))},
                    {"uptime_mins",    variant(static_cast<size_t>(uptime_mins.count()%60))},
                    {"uptime_hours",   variant(static_cast<size_t>(uptime_hours.count()))},

                    {"kernel",         variant(fmt::format("{} {}", query_system.kernel_name(), query_system.kernel_version()))},
                    {"kernel_name",    variant(query_system.kernel_name())},
                    {"kernel_version", variant(query_system.kernel_version())},
                    
                    {"hostname",       variant(query_system.hostname())},
                }}
            );
        }

        return;
    }
    
    if (moduleName == "user") {
        Query::User query_user;

        sysInfo.insert(
            {moduleName.data(), {
                {"name",          variant(query_user.name())},

                {"shell",         variant(fmt::format("{} {}", query_user.shell_name(), query_user.shell_version()))},
                {"shell_name",    variant(query_user.shell_name())},
                {"shell_path",    variant(query_user.shell_path())},
                {"shell_version", variant(query_user.shell_version())},

                {"wm_name",       variant(query_user.wm_name())},
                {"de_name",       variant(query_user.de_name())}
            }}
        );

        return;
    }
    
    if (moduleName == "cpu") {
        Query::CPU query_cpu;

        sysInfo.insert(
            {moduleName.data(), {
                {"cpu",         variant(fmt::format("{} ({}) @ {:.2f} GHz", query_cpu.name(), query_cpu.nproc(), query_cpu.freq_max()))},
                {"name",        variant(query_cpu.name())},
                {"nproc",       variant(query_cpu.nproc())},
                {"freq_cur",    variant(query_cpu.freq_cur())},
                {"freq_max",    variant(query_cpu.freq_max())},
                {"freq_min",    variant(query_cpu.freq_min())},
                {"freq_bios_limit", variant(query_cpu.freq_bios_limit())}
             }}
        );
        return;
    }
    
    if (hasStart(moduleName, "gpu")) {
        u_short id = static_cast<u_short>(moduleName.length() > 3 ? std::stoi(std::string(moduleName).substr(3, 4)) : 0);
        Query::GPU query_gpu(id);

        sysInfo.insert(
            {moduleName.data(), {
                {"name",   variant(query_gpu.name())},
                {"vendor", variant(query_gpu.vendor())}
            }}
        );

        return;
    }
    
    if (hasStart(moduleName, "disk")) {
        debug("disk module name = {}", moduleName);
        if (moduleName.length() <= 5)
            die(" PARSER: invalid disk component name ({}), must be disk(/path/to/fs)", moduleName);

        std::string path = moduleName.data();
        path.erase(0, 5); // disk(
        path.pop_back(); // )
        debug("disk path = {}", path);

        Query::Disk query_disk(path);

        sysInfo.insert(
            {moduleName.data(), {
                {"disk",  variant(fmt::format("{:.2f} GB / {:.2f} GB - {}", query_disk.used_amount(), query_disk.total_amount(), query_disk.typefs()))},
                {"total", variant(query_disk.total_amount())},
                {"free",  variant(query_disk.free_amount())},
                {"used",  variant(query_disk.used_amount())},
                {"fs",    variant(query_disk.typefs())}
            }}
        );

        return;
    }

    if (moduleName == "ram") {
        Query::RAM query_ram;

        sysInfo.insert(
            {moduleName.data(), {
                {"ram",   variant(fmt::format("{} MB / {} MB", query_ram.used_amount(), query_ram.total_amount()))},
                {"used",  variant(query_ram.used_amount())},
                {"total", variant(query_ram.total_amount())},
                {"free",  variant(query_ram.free_amount())},
                
                {"swap_total", variant(query_ram.swap_total_amount())},
                {"swap_free",  variant(query_ram.swap_free_amount())},
            }}
        );

        return;
    }

    die("Invalid module name {}!", moduleName);
}

void addValueFromModule(systemInfo_t& sysInfo, const std::string& moduleName, const std::string& moduleValueName) {
    // yikes, here we go.
    auto module_hash = fnv1a32::hash(moduleValueName);
   
    // stupid way to not construct query_system twice, more faster 
    if (moduleName == "os" || moduleName == "system") {
        Query::System query_system;

        std::chrono::seconds uptime_secs(query_system.uptime());
        auto uptime_mins = std::chrono::duration_cast<std::chrono::minutes>(uptime_secs);
        auto uptime_hours = std::chrono::duration_cast<std::chrono::hours>(uptime_secs);

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert(
                {moduleName, { }}
            );
        
        if (moduleName == "os") {
            if (sysInfo[moduleName].find(moduleValueName) == sysInfo[moduleName].end()) {
                
                switch (module_hash) {
                    case "name"_fnv1a32:
                        sysInfo[moduleName].insert({moduleValueName, variant(query_system.os_pretty_name())}); break;
                
                    case "uptime_secs"_fnv1a32:
                        sysInfo[moduleName].insert({moduleValueName, variant((size_t)uptime_secs.count()%60)}); break;
                
                    case "uptime_mins"_fnv1a32:
                        sysInfo[moduleName].insert({moduleValueName, variant((size_t)uptime_mins.count()%60)}); break;
                
                    case "uptime_hours"_fnv1a32:
                        sysInfo[moduleName].insert({moduleValueName, variant((size_t)uptime_hours.count())}); break;
                
                    case "kernel_name"_fnv1a32:
                        sysInfo[moduleName].insert({moduleValueName, variant(query_system.kernel_name())}); break;
                
                    case "kernel_version"_fnv1a32:
                        sysInfo[moduleName].insert({moduleValueName, variant(query_system.kernel_version())}); break;
                
                    case "hostname"_fnv1a32:
                        sysInfo[moduleName].insert({moduleValueName, variant(query_system.hostname())}); break;
                
                    case "arch"_fnv1a32:
                        sysInfo[moduleName].insert({moduleValueName, variant(query_system.arch())}); break;
                }
            }
        } else {
            if (sysInfo[moduleName].find(moduleValueName) == sysInfo[moduleName].end()) {
                switch (module_hash) {
                    case "host_name"_fnv1a32:
                        sysInfo[moduleName].insert({moduleValueName, variant(query_system.host_modelname())}); break;

                    case "host_vendor"_fnv1a32:
                        sysInfo[moduleName].insert({moduleValueName, variant(query_system.host_vendor())}); break;

                    case "host_version"_fnv1a32:
                        sysInfo[moduleName].insert({moduleValueName, variant(query_system.host_version())}); break;
                }
            }
        }

        return;
    }

    if (moduleName == "user") {
        Query::User query_user;
        
        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert(
                {moduleName, { }}
            );

        if (sysInfo[moduleName].find(moduleValueName) == sysInfo[moduleName].end()) 
        {
            switch (module_hash) {
                case "name"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, variant(query_user.name())}); break;

                case "shell_name"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, variant(query_user.shell_name())}); break;

                case "shell_path"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, variant(query_user.shell_path())}); break;

                case "shell_version"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, variant(query_user.shell_version())}); break;
            }
        }

        return;

    }

    if (moduleName == "cpu") {
        Query::CPU query_cpu;

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert(
                {moduleName, { }}
            );
        
        if (sysInfo[moduleName].find(moduleValueName) == sysInfo[moduleName].end()) {
            
            switch (module_hash) {
                case "name"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, variant(query_cpu.name())}); break;
            
                case "nproc"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, variant(query_cpu.nproc())}); break;

                case "freq_bios_limit"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, variant(query_cpu.freq_bios_limit())}); break;

                case "freq_cur"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, variant(query_cpu.freq_cur())}); break;

                case "freq_max"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, variant(query_cpu.freq_max())}); break;

                case "freq_min"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, variant(query_cpu.freq_min())}); break;
            }
        }

        return;
    }

    if (hasStart(moduleName, "gpu")) {
        u_short id = static_cast<u_short>(moduleName.length() > 3 ? std::stoi(std::string(moduleName).substr(3, 4)) : 0);
        Query::GPU query_gpu(id);

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert(
                {moduleName, { }}
            );
        
        if (sysInfo[moduleName].find(moduleValueName) == sysInfo[moduleName].end()) {
            
            if (moduleValueName == "name")
                sysInfo[moduleName].insert({moduleValueName, variant(query_gpu.name())});
            
            if (moduleValueName == "vendor")
                sysInfo[moduleName].insert({moduleValueName, variant(query_gpu.vendor())});
        }

        return;
    }

    if (hasStart(moduleName, "disk")) {
        if (moduleName.length() <= 5)
            die(" PARSER: invalid disk component name ({}), must be disk(/path/to/fs)", moduleName);

        std::string path = moduleName.data();
        path.erase(0, 5); // disk(
        path.pop_back(); // )
        debug("disk path = {}", path);

        Query::Disk query_disk(path);

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert(
                {moduleName, { }}
            );

        if (sysInfo[moduleName].find(moduleValueName) == sysInfo[moduleName].end()) {
            switch (module_hash) {
                case "used"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, variant(query_disk.used_amount())}); break;
                
                case "total"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, variant(query_disk.total_amount())}); break;
                
                case "free"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, variant(query_disk.free_amount())}); break;

                case "fs"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, variant(query_disk.typefs())}); break;
            }
        }

        return;
    }
    
    if (moduleName == "ram") {
        Query::RAM query_ram;

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert(
                {moduleName, { }}
            );
        
        if (sysInfo[moduleName].find(moduleValueName) == sysInfo[moduleName].end()) {
            switch (module_hash) {
                case "used"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, variant(query_ram.used_amount())}); break;
                
                case "total"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, variant(query_ram.total_amount())}); break;
                
                case "free"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, variant(query_ram.free_amount())}); break;
            }
        }

        return;
    }

    die("Invalid include module name {}!", moduleName);
}
