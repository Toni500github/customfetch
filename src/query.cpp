#include <unistd.h>
#include <array>
#include <stdexcept>
#include <string>

#include "query.hpp"
#include "config.hpp"
#include "switch_fnv1a.hpp"

// using namespace Query;

static std::array<std::string, 3> get_ansi_color( std::string str )
{
#define bgcolor "bgcolor"

    auto first_m = str.find( "m" );
    if ( first_m == std::string::npos )
        die( "Parser: failed to parse layout/ascii art: missing m while using ANSI color escape code" );

    std::string col    = str.erase( first_m ); // 1;42
    std::string weight = hasStart( col, "1;" ) ? "bold" : "normal";
    std::string type   = "fgcolor"; // either fgcolor or bgcolor

    if ( hasStart(col, "1;") || hasStart(col, "0;") )
        col.erase(0, 2);

    int n = std::stoi( col );

    // copy paste ahh code
    // unfortunatly you can't do bold and light in pango
    switch (n) {
        case 90:
        case 30: col = color.gui_black; 
            break;
        
        case 91:
        case 31: col = color.gui_red; 
            break;
        
        case 92:
        case 32: col = color.gui_green; 
            break;
        
        case 93:
        case 33: col = color.gui_yellow; 
            break;
        
        case 94:
        case 34: col = color.gui_blue; 
            break;

        case 95:
        case 35: col = color.gui_magenta; 
            break;
        
        case 96:
        case 36: col = color.gui_cyan; 
            break;
        
        case 97:
        case 37: col = color.gui_white; 
            break;

        case 100:
        case 40: col = color.gui_black;  type = bgcolor; break;
        
        case 101:
        case 41: col = color.gui_red;    type = bgcolor; break;
        
        case 102:
        case 42: col = color.gui_green;  type = bgcolor; break;
        
        case 103:
        case 43: col = color.gui_yellow; type = bgcolor; break;
        
        case 104:
        case 44: col = color.gui_blue;   type = bgcolor; break;
        
        case 105:
        case 45: col = color.gui_magenta; type = bgcolor; break;
        
        case 106:
        case 46: col = color.gui_cyan;   type = bgcolor; break;
        
        case 107:
        case 47: col = color.gui_white;  type = bgcolor; break;
    }

    if ( col[0] != '#' )
        col.erase(0, col.find("#"));

    return { col, weight, type };
}

static std::string getInfoFromName( systemInfo_t& systemInfo, const std::string& name )
{
    std::vector<std::string> sections = split( name, '.' );

    try
    {
        if ( systemInfo.find( sections[0] ) == systemInfo.end() )
            throw std::out_of_range( "genius" );
        if ( systemInfo[sections[0]].find( sections[1] ) == systemInfo[sections[0]].end() )
            throw std::out_of_range( "genius" );

        auto result = systemInfo[sections[0]][sections[1]];
        std::string stringResult;

        if ( std::holds_alternative<size_t>( result ) )
            stringResult = std::to_string( std::get<size_t>( result ) );
        else
            stringResult = std::get<std::string>( result );

        return stringResult;
    }
    catch ( const std::out_of_range& err )
    {
        return "<unknown/invalid component>";
    };
}

std::string parse( std::string& input, systemInfo_t& systemInfo, std::unique_ptr<std::string>& pureOutput )
{
    std::string output = input;
    if ( pureOutput )
        *pureOutput = output;

    size_t dollarSignIndex = 0;
    size_t pureOutputOffset = 0;
    bool start = false;

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

        switch ( output[dollarSignIndex + 1] )
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
            if ( output[i] == type && output[i - 1] != '\\' )
            {
                endBracketIndex = i;
                break;
            }
            else if ( output[i] == type )
                command.erase( command.size() - 1, 1 );

            command += output[i];
        }

        if ( (int)endBracketIndex == -1 )
            die( "PARSER: Opened tag is not closed at index {} in string {}", dollarSignIndex, output );

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
            if ( command == "0" )
            {   
                output   = output.replace( dollarSignIndex, ( endBracketIndex + 1 ) - dollarSignIndex, config.gui ? "</span><span>" : NOCOLOR );
                if ( pureOutput )
                    *pureOutput = pureOutput->replace( pureOutput->size() /*dollarSignIndex-pureOutputOffset*/,
                                                       ( endBracketIndex + 1 ) - dollarSignIndex, "" );

                pureOutputOffset += endBracketIndex - dollarSignIndex + 1;
            }
            else
            {
                std::string str_clr;
                if ( config.gui )
                {
                    switch ( fnv1a32::hash(command) )
                    {
                        case "black"_fnv1a32:       str_clr = color.gui_black; break;   
                        case "red"_fnv1a32:         str_clr = color.gui_red;break;
                        case "blue"_fnv1a32:        str_clr = color.gui_blue; break;
                        case "green"_fnv1a32:       str_clr = color.gui_green; break;
                        case "cyan"_fnv1a32:        str_clr = color.gui_cyan; break;
                        case "yellow"_fnv1a32:      str_clr = color.gui_yellow; break;
                        case "magenta"_fnv1a32:     str_clr = color.gui_magenta; break;
                        case "white"_fnv1a32:       str_clr = color.gui_white; break;
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
                        std::array<std::string, 3> clrs = get_ansi_color( hasStart( str_clr, "\033" ) ? str_clr.substr( 2 ) : str_clr.substr( 3 ) );
                        std::string color  = clrs.at( 0 );
                        std::string weight = clrs.at( 1 );
                        std::string type   = clrs.at( 2 );
                        output = output.replace( dollarSignIndex, output.length() - dollarSignIndex,
                                                 fmt::format( "<span {}='{}' weight='{}'>{}</span>", type, color, weight, 
                                                 output.substr( endBracketIndex + 1 ) ) );
                    }

                    else
                        error( "PARSER: failed to parse line with color '{}'", str_clr );

                    if ( pureOutput )
                        *pureOutput = pureOutput->replace( pureOutput->size() /*dollarSignIndex - pureOutputOffset*/,
                                                        endBracketIndex - dollarSignIndex + 1, "" );

                    pureOutputOffset += endBracketIndex - dollarSignIndex + 1;
                }

                else
                {
                    switch ( fnv1a32::hash(command) )
                    {
                        case "black"_fnv1a32:       str_clr = color.black; break;
                        case "red"_fnv1a32:         str_clr = color.red; break;
                        case "blue"_fnv1a32:        str_clr = color.blue;break;
                        case "green"_fnv1a32:       str_clr = color.green; break;
                        case "cyan"_fnv1a32:        str_clr = color.cyan; break;
                        case "yellow"_fnv1a32:      str_clr = color.yellow; break;
                        case "magenta"_fnv1a32:     str_clr = color.magenta; break;
                        case "white"_fnv1a32:       str_clr = color.white; break;
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

                    if ( pureOutput )
                        *pureOutput = pureOutput->replace(dollarSignIndex - pureOutputOffset,
                                                        endBracketIndex - dollarSignIndex + 1, "" );

                    pureOutputOffset += formatted_replacement_string.length() - unformatted_replacement_string.length() - 1;
                }
            }
            break;
        }
    }

    return output;
}

void addModuleValues(systemInfo_t &sysInfo, std::string &moduleName) {
    // yikes, here we go.

    if (moduleName == "os") {
        Query::System query_system;
    
        std::chrono::seconds uptime_secs(query_system.uptime());
        auto uptime_mins = std::chrono::duration_cast<std::chrono::minutes>(uptime_secs);
        auto uptime_hours = std::chrono::duration_cast<std::chrono::hours>(uptime_secs);

        sysInfo.insert(
            {"os", {
                {"name",           VARIANT(query_system.os_pretty_name())},
                {"username",       VARIANT(query_system.username())},
                {"uptime_secs",    VARIANT((size_t)uptime_secs.count()%60)},
                {"uptime_mins",    VARIANT((size_t)uptime_mins.count()%60)},
                {"uptime_hours",   VARIANT((size_t)uptime_hours.count())},
                {"kernel_name",    VARIANT(query_system.kernel_name())},
                {"kernel_version", VARIANT(query_system.kernel_version())},
                {"hostname",       VARIANT(query_system.hostname())},
                {"arch",           VARIANT(query_system.arch())},
            }}
        );

        return;
    }
    if (moduleName == "cpu") {
        Query::CPU query_cpu;

        sysInfo.insert(
            {"cpu", {
                {"name", VARIANT(query_cpu.name())},
            }}
        );

        return;
    }
    if (hasStart(moduleName, "gpu")) {
        u_short id = moduleName.length() > 3 ? std::stoi(moduleName.substr(3, 4)) : 0;
        Query::GPU query_gpu(pac, id);

        sysInfo.insert(
            {"gpu", {
                {"name",   VARIANT(query_gpu.name())},
                {"vendor", VARIANT(query_gpu.vendor())}
            }}
        );

        return;
    }
    if (moduleName == "ram") {
        Query::RAM query_ram;

        sysInfo.insert(
            {"ram", {
                {"used",  VARIANT(query_ram.used_amount())},
                {"total", VARIANT(query_ram.total_amount())},
                {"free",  VARIANT(query_ram.free_amount())}
            }}
        );

        return;
    }

    die("Invalid module name {}!", moduleName);
}

void addValueFromModule(systemInfo_t& sysInfo, std::string& moduleName, std::string& moduleValueName) {
    // yikes, here we go.
    if (moduleName == "os") {
        Query::System query_system;

        std::chrono::seconds uptime_secs(query_system.uptime());
        auto uptime_mins = std::chrono::duration_cast<std::chrono::minutes>(uptime_secs);
        auto uptime_hours = std::chrono::duration_cast<std::chrono::hours>(uptime_secs);

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert(
                {moduleName, { }}
            );
        
        if (sysInfo[moduleName].find(moduleValueName) == sysInfo[moduleName].end()) {
            
            // this string switch case library name is so damn shitty
            // thanks god clangd has an auto completer
            switch (fnv1a32::hash(moduleValueName)) {
                case "name"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, VARIANT(query_system.os_pretty_name())}); break;
            
                case "username"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, VARIANT(query_system.username())}); break;
            
                case "uptime_secs"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, VARIANT((size_t)uptime_secs.count()%60)}); break;
            
                case "uptime_mins"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, VARIANT((size_t)uptime_mins.count()%60)}); break;
            
                case "uptime_hours"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, VARIANT((size_t)uptime_hours.count())}); break;
            
                case "kernel_name"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, VARIANT(query_system.kernel_name())}); break;
            
                case "kernel_version"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, VARIANT(query_system.kernel_version())}); break;
            
                case "hostname"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, VARIANT(query_system.hostname())}); break;
            
                case "arch"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, VARIANT(query_system.arch())}); break;
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
            if (moduleValueName == "name")
                sysInfo[moduleName].insert({moduleValueName,    VARIANT(query_cpu.name())});
        }

        return;
    }

    if (hasStart(moduleName, "gpu")) {
        u_short id = moduleName.length() > 3 ? std::stoi(moduleName.substr(3, 4)) : 0;
        Query::GPU query_gpu(pac, id);

        if (sysInfo.find(moduleName) == sysInfo.end())
            sysInfo.insert(
                {moduleName, { }}
            );
        
        if (sysInfo[moduleName].find(moduleValueName) == sysInfo[moduleName].end()) {
            
            if (moduleValueName == "name")
                sysInfo[moduleName].insert({moduleValueName, VARIANT(query_gpu.name())});
            
            if (moduleValueName == "vendor")
                sysInfo[moduleName].insert({moduleValueName, VARIANT(query_gpu.vendor())});
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
            switch (fnv1a32::hash(moduleValueName)) {
                case "used"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, VARIANT(query_ram.used_amount())}); break;
                
                case "total"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, VARIANT(query_ram.total_amount())}); break;
                
                case "free"_fnv1a32:
                    sysInfo[moduleName].insert({moduleValueName, VARIANT(query_ram.free_amount())}); break;
            }
        }

        return;
    }

    die("Invalid include module name {}!", moduleName);
}
