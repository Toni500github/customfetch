#include "util.hpp"
#include "config.hpp"
#include "fmt/color.h"
#include "pci.ids.hpp"
#include <algorithm>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

// https://stackoverflow.com/questions/874134/find-out-if-string-ends-with-another-string-in-c#874160
bool hasEnding(std::string_view fullString, std::string_view ending) {
    if (ending.length() > fullString.length())
        return false;
    return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
}

bool hasStart(std::string_view fullString, std::string_view start) {
    if (start.length() > fullString.length())
        return false;
    return (0 == fullString.compare(0, start.length(), start));
}

std::vector<std::string> split(std::string_view text, char delim) {
    std::string              line;
    std::vector<std::string> vec;
    std::stringstream        ss(text.data());
    while (std::getline(ss, line, delim)) {
        vec.push_back(line);
    }
    return vec;
}

/**
 * remove all white spaces (' ', '\t', '\n') from start and end of input
 * inplace!
 * @param input
 * Original https://github.com/lfreist/hwinfo/blob/main/include/hwinfo/utils/stringutils.h#L50
 */
void strip(std::string& input) {
    if (input.empty()) {
        return;
    }
    
    // optimization for input size == 1
    if (input.size() == 1) {
        if (input[0] == ' ' || input[0] == '\t' || input[0] == '\n') {
            input = "";
            return;
        } else {
            return;
        }
    }
    
    size_t start_index = 0;
    while (true) {
        char c = input[start_index];
        if (c != ' ' && c != '\t' && c != '\n') {
            break;
        }
        start_index++;
    }

    size_t end_index = input.size() - 1;
    while (true) {
        char c = input[end_index];
        if (c != ' ' && c != '\t' && c != '\n') {
            break;
        }
        end_index--;
    }
    
    if (end_index < start_index) {
        input.assign("");
        return;
    }
    
    input.assign(input.begin() + start_index, input.begin() + end_index + 1);
}

std::string getInfoFromName(systemInfo_t &systemInfo, const std::string &name) {
    std::vector<std::string> sections = split(name, '.');

    try {
        if (!systemInfo.contains(sections[0]))
            throw std::out_of_range("genius");
        if (!systemInfo[sections[0]].contains(sections[1]))
            throw std::out_of_range("genius");

        auto result = systemInfo[sections[0]][sections[1]];
        std::string stringResult;
        if (std::holds_alternative<size_t>(result))
            stringResult = std::to_string(std::get<size_t>(result));
        else
            stringResult = std::get<std::string>(result);

        return stringResult;
    } catch (const std::out_of_range &err) {
        return "<unknown/invalid>";
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
                //die("PARSER: Not implemented: module types (<gpu.name>)");
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
                if (pureOutput)
                    *pureOutput = pureOutput->replace(dollarSignIndex-pureOutputOffset, (endBracketIndex + 1) - dollarSignIndex, "");
                pureOutputOffset += endBracketIndex - dollarSignIndex + 1;
                break;
            case '>':
                output = output.replace(dollarSignIndex, (endBracketIndex + 1) - dollarSignIndex, getInfoFromName(systemInfo, command));
                if (pureOutput)
                    *pureOutput = pureOutput->replace(dollarSignIndex-pureOutputOffset, (endBracketIndex + 1) - dollarSignIndex, "");
                pureOutputOffset += endBracketIndex - dollarSignIndex + 1;
                break;
            case '}':
                if (command == "0") {
                    output = output.replace(dollarSignIndex, (endBracketIndex + 1) - dollarSignIndex, NOCOLOR);
                    if (pureOutput)
                        *pureOutput = pureOutput->replace(dollarSignIndex-pureOutputOffset, (endBracketIndex + 1) - dollarSignIndex, "");
                    pureOutputOffset += endBracketIndex - dollarSignIndex + 1;
                } else {
                    // yeah you can't do a switch case with strings in C/C++
                    // hope it doesn't hurt performance too much
                    std::string str_clr = 
                        command == "red"     ? color.red    : 
                        command == "blue"    ? color.blue   : 
                        command == "green"   ? color.green  :
                        command == "cyan"    ? color.cyan   :
                        command == "yellow"  ? color.yellow :
                        command == "magenta" ? color.magenta:
                        command;
                    
                    fmt::rgb clr;
                    if (str_clr[0] == '#') {
                        clr = hexStringToColor(str_clr);
                        output = output.replace(dollarSignIndex, output.length()-dollarSignIndex, fmt::format(fmt::fg(clr), "{}", output.substr(endBracketIndex + 1)));
                    } else if (hasStart(str_clr, "\\e") || hasStart(str_clr, "\033")) { // what?
                        output = output.replace(dollarSignIndex, output.length()-dollarSignIndex, fmt::format("{:c}[{}{}", 0x1B, hasStart(str_clr, "\033") ? // "\\e" is for checking in the ascii_art, \033 in the config
                                                                                                                                str_clr.substr(2) : str_clr.substr(3), output.substr(endBracketIndex + 1)));
                    }

                    if (pureOutput)
                        *pureOutput = pureOutput->replace(dollarSignIndex-pureOutputOffset, endBracketIndex - dollarSignIndex + 1, "");
                    
                    pureOutputOffset += endBracketIndex - dollarSignIndex + 1;
                }

                break;
        }
    }

    return output;
}

fmt::rgb hexStringToColor(std::string_view hexstr) {
    hexstr = hexstr.substr(1);
    // convert the hexadecimal string to individual components
    std::stringstream ss;
    ss << std::hex << hexstr;

    int intValue;
    ss >> intValue;

    int red   = (intValue >> 16) & 0xFF;
    int green = (intValue >> 8) & 0xFF;
    int blue  = intValue & 0xFF;

    return fmt::rgb(red, green, blue);
}

// Function to perform binary search on the pci vendors array to find a device from a vendor.
std::string binarySearchPCIArray(std::string_view vendor_id_s, std::string_view pci_id_s) {
    std::string_view vendor_id = hasStart(vendor_id_s, "0x") ? vendor_id_s.substr(2) : vendor_id_s;
    std::string_view pci_id    = hasStart(pci_id_s, "0x") ? pci_id_s.substr(2) : pci_id_s;

    long location_array_index = std::distance(pci_vendors_array.begin(), std::lower_bound(pci_vendors_array.begin(), pci_vendors_array.end(), vendor_id));
    size_t approx_vendors_location = pci_vendors_location_array[location_array_index];
    size_t vendors_location = all_ids.find(pci_id, approx_vendors_location);

    if (vendors_location == std::string::npos)
        return UNKNOWN;

    // Here we use find from vendors_location because as it turns out, lower_bound doesn't return WHERE it is, but where "val" can be placed without affecting the order of the string (binary search stuff)
    // so we have to find from the point onwards to find the actual line, it is still a shortcut, better than searching from 0.
    return name_from_entry(vendors_location);
}

// Function to perform binary search on the pci vendors array to find a vendor.
std::string binarySearchPCIArray(std::string_view vendor_id_s) {
    std::string_view vendor_id = hasStart(vendor_id_s, "0x") ? vendor_id_s.substr(2) : vendor_id_s;

    long location_array_index = std::distance(pci_vendors_array.begin(), std::lower_bound(pci_vendors_array.begin(), pci_vendors_array.end(), vendor_id));
    size_t approx_vendors_location = pci_vendors_location_array[location_array_index];
    size_t vendors_location = all_ids.find(vendor_id, approx_vendors_location);

    if (vendors_location == std::string::npos)
        return UNKNOWN;

    // Here we use find from vendors_location because as it turns out, lower_bound doesn't return WHERE it is, but where "val" can be placed without affecting the order of the string (binary search stuff)
    // so we have to find from the point onwards to find the actual line, it is still a shortcut, better than searching from 0.
    return vendor_from_entry(vendors_location, vendor_id);
}

// http://stackoverflow.com/questions/478898/ddg#478960
std::string shell_exec(std::string_view cmd) {
    std::array<char, 128>  buffer;
    std::string            result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.data(), "r"), pclose);

    if (!pipe)
        die(_("popen() failed!"));

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        result += buffer.data();

    // why there is a '\n' at the end??
    if (!result.empty() && result[result.length() - 1] == '\n')
        result.erase(result.length() - 1);
    return result;
}

std::string name_from_entry(size_t dev_entry_pos) {
    // Step 1: Find the position of the opening square bracket after the ID
    size_t bracket_open_pos = all_ids.find('[', dev_entry_pos);
    if (bracket_open_pos == std::string::npos)
        return UNKNOWN; // Opening bracket not found

    // Step 2: Find the position of the closing square bracket after the opening bracket
    size_t bracket_close_pos = all_ids.find(']', bracket_open_pos);
    if (bracket_close_pos == std::string::npos)
        return UNKNOWN; // Closing bracket not found

    // Step 3: Extract the substring between the brackets
    return all_ids.substr(bracket_open_pos + 1, bracket_close_pos - bracket_open_pos - 1);
}

std::string vendor_from_entry(size_t vendor_entry_pos, std::string_view vendor_id) {
    // Step 1: Find the end of the line (newline character) starting from the ID position
    size_t end_line_pos = all_ids.find('\n', vendor_entry_pos);
    if (end_line_pos == std::string::npos)
        end_line_pos = all_ids.length(); // If no newline is found, set to end of string

    // Step 2: Extract the substring from the ID position to the end of the line
    std::string line = all_ids.substr(vendor_entry_pos, end_line_pos - vendor_entry_pos);

    // Step 3: Find the position after the ID (ID length plus possible spaces)
    size_t      after_id_pos = line.find(vendor_id) + 4;
    std::string description  = line.substr(after_id_pos);

    size_t first = description.find_first_not_of(' ');
    if (first == std::string::npos)
        return UNKNOWN;

    size_t last = description.find_last_not_of(' ');
    return description.substr(first, (last - first + 1));
}

/*
* Get the user config directory
* either from $XDG_CONFIG_HOME or from $HOME/.config/
* @return user's config directory  
*/
std::string getHomeConfigDir() {
    char *dir = getenv("XDG_CONFIG_HOME");
    if (dir != NULL && dir[0] != '\0' && std::filesystem::exists(dir)) {
        std::string str_dir(dir);
        return hasEnding(str_dir, "/") ? str_dir.substr(0, str_dir.rfind('/')) : str_dir;
    } else {
        char *home = getenv("HOME");
        if (home == nullptr)
            die(_("Failed to find $HOME, set it to your home directory!"));

        return std::string(home) + "/.config";
    }
}

/*
 * Get the TabAUR config directory 
 * where we'll have both "config.toml" and "theme.toml"
 * from Config::getHomeConfigDir()
 * @return TabAUR's config directory
 */
std::string getConfigDir() {
    return getHomeConfigDir() + "/customfetch";
}
