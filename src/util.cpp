#include "util.hpp"
#include "fmt/color.h"
#include "pci.ids.hpp"

#include <cerrno>
#include <string>
#include <sstream>
#include <string_view>
#include <vector>

// https://stackoverflow.com/questions/874134/find-out-if-string-ends-with-another-string-in-c#874160
bool hasEnding(const std::string_view fullString, const std::string_view ending) {
    if (ending.length() > fullString.length())
        return false;
    return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
}

bool hasStart(const std::string_view fullString, const std::string_view start) {
    if (start.length() > fullString.length())
        return false;
    return (fullString.substr(0, start.size()) == start);
}

std::vector<std::string> split(const std::string_view text, char delim) {
    std::string              line;
    std::vector<std::string> vec;
    std::stringstream        ss(text.data());
    while (std::getline(ss, line, delim)) {
        vec.push_back(line);
    }
    return vec;
}

/** Replace special symbols such as ~ and $ in std::strings
 * @param str The string
 * @return The modified string
 */
std::string expandVar(const std::string_view str) {
    std::string ret = str.data();
    const char *env;
    if (ret[0] == '~') {
#ifdef CF_WINDOWS
	env = getenv("USERPROFILE");
#else
        env = getenv("HOME");
#endif
        if (env == nullptr)
            die("FATAL: $HOME enviroment variable is not set (how?)");

        ret.replace(0, 1, env); // replace ~ with the $HOME value
    } else if (str[0] == '$') {
        ret.erase(0, 1);
        
        std::string temp;
        size_t pos = ret.find('/');
        if (pos != std::string::npos) {
            temp = str.substr(pos+1);
            ret.erase(pos);
        }

        env = getenv(ret.c_str());
        if (env == nullptr)
            die("No such enviroment variable: {}", str);

        ret = env;
        ret += temp;
    }

    return ret;
}

std::string read_by_syspath(const std::string_view path) {
    std::ifstream f_drm(path.data());
    if (!f_drm.is_open()) {
        error("Failed to open {}", path);
        return UNKNOWN;
    }

    std::string ret;
    std::getline(f_drm, ret);
    return ret;
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
    
    // https://stackoverflow.com/a/25385766
    const char* ws = " \t\n\r\f\v";
    input.erase(input.find_last_not_of(ws) + 1);
    input.erase(0, input.find_first_not_of(ws));
}

fmt::rgb hexStringToColor(const std::string_view hexstr) {
    std::string _hexstr = hexstr.substr(1).data();
    // convert the hexadecimal string to individual components
    std::stringstream ss;
    ss << std::hex << _hexstr;

    int intValue;
    ss >> intValue;

    int red   = (intValue >> 16) & 0xFF;
    int green = (intValue >> 8) & 0xFF;
    int blue  = intValue & 0xFF;

    return fmt::rgb(red, green, blue);
}

void replace_str(std::string& str, const std::string& from, const std::string& to)
{
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
}

std::string str_tolower(const std::string_view str) {
    std::string ret = str.data();
    for (auto& x : ret)
        x = std::tolower(x); 
    
    return ret;
}

std::string str_toupper(const std::string_view str) {
    std::string ret = str.data();
    for (auto& x : ret)
        x = std::toupper(x); 
    
    return ret;
}

// Function to perform binary search on the pci vendors array to find a device from a vendor.
std::string binarySearchPCIArray(const std::string_view vendor_id_s, const std::string_view pci_id_s) {
    std::string vendor_id = hasStart(vendor_id_s, "0x") ? vendor_id_s.substr(2).data() : vendor_id_s.data();
    std::string pci_id    = hasStart(pci_id_s, "0x") ? pci_id_s.substr(2).data() : pci_id_s.data();

    size_t left = 0, right = pci_vendors_array.size(), mid;

    while (right >= left) {
        mid = left + (right - left) / 2;

        // If the element is present at the middle
        // itself
        if (pci_vendors_array[mid] == vendor_id)
            break;

        // If element is smaller than mid, then
        // it can only be present in left subarray
        if (pci_vendors_array[mid] > vendor_id)
            right = mid - 1;

        // Else the element can only be present
        // in right subarray
        if (pci_vendors_array[mid] < vendor_id)
            left = mid + 1;
    }

    size_t approx_vendors_location = pci_vendors_location_array[mid];
    size_t vendor_location = all_ids.find(vendor_id, approx_vendors_location);
    size_t device_location = all_ids.find(pci_id, vendor_location);

    if (vendor_location == std::string::npos || device_location == std::string::npos)
        return UNKNOWN;

    // Here we use find from vendors_location because as it turns out, lower_bound doesn't return WHERE it is, but where "val" can be placed without affecting the order of the string (binary search stuff)
    // so we have to find from the point onwards to find the actual line, it is still a shortcut, better than searching from 0.
    return vendor_from_entry(vendor_location, vendor_id) + " " + name_from_entry(device_location);
}

// Function to perform binary search on the pci vendors array to find a vendor.
std::string binarySearchPCIArray(const std::string_view vendor_id_s) {
    std::string vendor_id = hasStart(vendor_id_s, "0x") ? vendor_id_s.substr(2).data() : vendor_id_s.data();

    size_t left = 0, right = pci_vendors_array.size(), mid;

    while (right >= left) {
        mid = left + (right - left) / 2;

        // If the element is present at the middle
        // itself
        if (pci_vendors_array[mid] == vendor_id)
            break;

        // If element is smaller than mid, then
        // it can only be present in left subarray
        if (pci_vendors_array[mid] > vendor_id)
            right = mid - 1;

        // Else the element can only be present
        // in right subarray
        left = mid + 1;
    }

    size_t approximate_vendors_location = pci_vendors_location_array[mid];
    size_t vendors_location = all_ids.find(vendor_id, approximate_vendors_location);

    if (vendors_location == std::string::npos)
        return UNKNOWN;

    // Here we use find from vendors_location because as it turns out, lower_bound doesn't return WHERE it is, but where "val" can be placed without affecting the order of the string (binary search stuff)
    // so we have to find from the point onwards to find the actual line, it is still a shortcut, better than searching from 0.
    return vendor_from_entry(vendors_location, vendor_id);
}

// http://stackoverflow.com/questions/478898/ddg#478960
std::string shell_exec(const std::string_view cmd) {
    std::array<char, 128>  buffer;
    std::string            result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.data(), "r"), pclose);

    if (!pipe)
        die("popen() failed: {}", errno);

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        result += buffer.data();

    // why there is a '\n' at the end??
    if (!result.empty() && result[result.length() - 1] == '\n')
        result.pop_back();
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

std::string vendor_from_entry(size_t vendor_entry_pos, const std::string_view vendor_id) {
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
#ifdef CF_WINDOWS
	char *home = getenv("USERPROFILE");
#else
        char *home = getenv("HOME");
#endif
        if (home == nullptr)
            die("Failed to find $HOME, set it to your home directory!");

        return std::string(home) + "/.config";
    }
}

/*
 * Get the customfetch config directory 
 * where we'll have both "config.toml" and "theme.toml"
 * from Config::getHomeConfigDir()
 * @return TabAUR's config directory
 */
std::string getConfigDir() {
    return getHomeConfigDir() + "/customfetch";
}
