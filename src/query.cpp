#include "query.hpp"
#include "switch_fnv1a.hpp"
#include "config.hpp"
#include <stdexcept>

//using namespace Query;

static std::string getInfoFromName(systemInfo_t& systemInfo, const std::string& name) {
    std::vector<std::string> sections = split(name, '.');

    try {
        if (systemInfo.find(sections[0]) == systemInfo.end())
            throw std::out_of_range("genius");
        if (systemInfo[sections[0]].find(sections[1]) == systemInfo[sections[0]].end())
            throw std::out_of_range("genius");

        auto result = systemInfo[sections[0]][sections[1]];
        std::string stringResult;
        if (std::holds_alternative<size_t>(result))
            stringResult = std::to_string(std::get<size_t>(result));
        else
            stringResult = std::get<std::string>(result);

        return stringResult;
    } catch (const std::out_of_range &err) {
        return "<unknown/invalid module>";
    };
}

std::string parse(std::string& input, systemInfo_t &systemInfo, std::unique_ptr<std::string> &pureOutput) {
    std::string output = input;
    if (pureOutput)
        *pureOutput = output;

    size_t dollarSignIndex = 0;
    size_t pureOutputOffset = 0;
    bool start = false;

    while (true) {
        size_t oldDollarSignIndex = dollarSignIndex;
        dollarSignIndex           = output.find('$', dollarSignIndex);

        if (dollarSignIndex == std::string::npos || (dollarSignIndex <= oldDollarSignIndex && start))
          break;

        start = true;

        // check for bypass
        // YOU CAN USE AND/NOT IN C++????
        // btw the second part checks if it has a \ before it and NOT a \ before the backslash, (check for escaped backslash)
        // example: \$ is bypassed, \\$ is NOT bypassed.
        // this will not make an effort to check multiple backslashes, thats your fault atp.
        if (dollarSignIndex > 0 and (output[dollarSignIndex - 1] == '\\' and (dollarSignIndex == 1 or output[dollarSignIndex - 2] != '\\')))
          continue;

        std::string command         = "";
        size_t      endBracketIndex = -1;

        char        type = ' '; // ' ' = undefined, ')' = shell exec, 2 = ')' asking for a module

        switch (output[dollarSignIndex + 1]) {
            case '(':
                type = ')';
                break;
            case '<':
                type = '>';
                break;
            case '{':
                type = '}';
                break;
            default: // neither of them
                break;
        }

        if (type == ' ')
            continue;

        for (size_t i = dollarSignIndex + 2; i < output.size(); i++) {
            if (output[i] == type && output[i - 1] != '\\') {
                endBracketIndex = i;
                break;
            } else if (output[i] == type)
                command.erase(command.size() - 1, 1);

            command += output[i];
        }

        if ((int)endBracketIndex == -1)
            die("PARSER: Opened tag is not closed at index {} in string {}", dollarSignIndex, output);

        switch (type) {
            case ')':
                output = output.replace(dollarSignIndex, (endBracketIndex + 1) - dollarSignIndex, shell_exec(command));
                break;
            case '>':
                output = output.replace(dollarSignIndex, (endBracketIndex + 1) - dollarSignIndex, getInfoFromName(systemInfo, command));
                break;
            case '}':
                if (command == "0") {
                    output = output.replace(dollarSignIndex, (endBracketIndex + 1) - dollarSignIndex, NOCOLOR);
                    if (pureOutput)
                        *pureOutput = pureOutput->replace(dollarSignIndex-pureOutputOffset, (endBracketIndex + 1) - dollarSignIndex, "");
                    pureOutputOffset += endBracketIndex - dollarSignIndex + 1;
                } else {
                    // hope it doesn't hurt performance too much
                    std::string str_clr; /*= 
                        command == "red"     ? color.red    : 
                        command == "blue"    ? color.blue   : 
                        command == "green"   ? color.green  :
                        command == "cyan"    ? color.cyan   :
                        command == "yellow"  ? color.yellow :
                        command == "magenta" ? color.magenta:
                        command;*/
                    switch (fnv1a32::hash(command)) {
                        case "red"_fnv1a32:
                            str_clr = color.red; break;
                        case "blue"_fnv1a32:
                            str_clr = color.blue; break;
                        case "green"_fnv1a32:
                            str_clr = color.green; break;
                        case "cyan"_fnv1a32:
                            str_clr = color.cyan; break;
                        case "yellow"_fnv1a32:
                            str_clr = color.yellow; break;
                        case "magenta"_fnv1a32:
                            str_clr = color.magenta; break;
                        default:
                            str_clr = command; break;
                    }
                    
                    fmt::rgb clr;
                    if (config.gui) {
                        if (str_clr[0] == '#') {
                            output = output.replace(dollarSignIndex, output.length()-dollarSignIndex, fmt::format("<span foreground='{}'>{}</span>", str_clr, output.substr(endBracketIndex + 1)));
                        } else if (hasStart(str_clr, "\\e") || hasStart(str_clr, "\033")) { // what?
                            warn("bash color not supported on GUI mode");
                            //output = output.replace(dollarSignIndex, output.length()-dollarSignIndex, fmt::format("{:c}[{}{}", 0x1B, hasStart(str_clr, "\033") ? // "\\e" is for checking in the ascii_art, \033 in the config
                            //                                                                                                        str_clr.substr(2) : str_clr.substr(3), output.substr(endBracketIndex + 1)));
                        }
                    }
                    else {
                        if (str_clr[0] == '#') {
                            clr = hexStringToColor(str_clr);
                            output = output.replace(dollarSignIndex, output.length()-dollarSignIndex, fmt::format(fmt::fg(clr), "{}", output.substr(endBracketIndex + 1)));
                        } else if (hasStart(str_clr, "\\e") || hasStart(str_clr, "\033")) { // what?
                            output = output.replace(dollarSignIndex, output.length()-dollarSignIndex, fmt::format("{:c}[{}{}", 0x1B, hasStart(str_clr, "\033") ? // "\\e" is for checking in the ascii_art, \033 in the config
                                                                                                                                    str_clr.substr(2) : str_clr.substr(3), output.substr(endBracketIndex + 1)));
                        }
                    }
                    //debug("dollarSignIndex = {}\npureOutputOffset = {}", dollarSignIndex, pureOutputOffset);
                    if (pureOutput)
                        *pureOutput = pureOutput->replace(pureOutput->size()/*dollarSignIndex - pureOutputOffset*/, endBracketIndex - dollarSignIndex + 1, "");
                    
                    pureOutputOffset += endBracketIndex - dollarSignIndex + 1;
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
                {"name",           VARIANT(query_system.os_name())},
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
                    sysInfo[moduleName].insert({moduleValueName, VARIANT(query_system.os_name())}); break;
            
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
            switch (fnv1a32::hash(moduleName)) {
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
