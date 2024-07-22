#include "util.hpp"
#include "fmt/color.h"
#include "pci.ids.hpp"

#include <sys/wait.h>
#include <array>
#include <filesystem>
#include <fcntl.h>
#include <fmt/ranges.h>
#include <cerrno>
#include <cstring>
#include <memory>
#include <fstream>
#include <sstream>
#include <string>
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
        env = std::getenv("HOME");
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

        env = std::getenv(ret.c_str());
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

bool is_file_image(const unsigned char *bytes) {
    
    // https://stackoverflow.com/a/49683945
    constexpr std::array<unsigned char, 3> jpeg = {0xff, 0xd8, 0xff};
    constexpr std::array<unsigned char, 8> png = {0x89, 0x50, 0x4e, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    constexpr std::array<unsigned char, 6> gif89a = {0x47, 0x49, 0x46, 0x38, 0x39, 0x61};
    constexpr std::array<unsigned char, 6> gif87a = {0x47, 0x49, 0x46, 0x38, 0x37, 0x61};
    constexpr std::array<unsigned char, 2> bmp =  {0x42, 0x4D};
    constexpr std::array<unsigned char, 4> tiffI = {0x49, 0x49, 0x2A, 0x00};
    constexpr std::array<unsigned char, 4> tiffM = {0x4D, 0x4D, 0x00, 0x2A};

    if (std::memcmp(bytes, png.data(), png.size()) == 0)
        return true;
    else if (std::memcmp(bytes, jpeg.data(), jpeg.size()) == 0)
        return true;
    else if (std::memcmp(bytes, gif89a.data(), gif89a.size()) == 0)
        return true;
    else if (std::memcmp(bytes, gif87a.data(), gif87a.size()) == 0)
        return true;
    else if (std::memcmp(bytes, tiffM.data(), tiffM.size()) == 0)
        return true;
    else if (std::memcmp(bytes, tiffI.data(), tiffI.size()) == 0)
        return true;
    else if (std::memcmp(bytes, bmp.data(), bmp.size()) == 0)
        return true;



    return false;
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
    // convert the hexadecimal string to individual components
    std::stringstream ss;
    ss << std::hex << hexstr.substr(1).data();

    uint intValue;
    ss >> intValue;

    uint red   = (intValue >> 16) & 0xFF;
    uint green = (intValue >> 8) & 0xFF;
    uint blue  = intValue & 0xFF;

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

bool read_exec(std::vector<const char *> cmd, std::string& output, bool useStdErr) {
    int pipeout[2];

    if (pipe(pipeout) < 0)
        die("pipe() failed: {}", strerror(errno));

    int pid = fork();

    if (pid > 0) { // we wait for the command to finish then start executing the rest
        close(pipeout[1]);

        int status;
        waitpid(pid, &status, 0); // Wait for the child to finish

        if (WIFEXITED(status) && (WEXITSTATUS(status) == 0 || useStdErr)) {
            // read stdout
            debug("reading stdout");
            char c;
            while (read(pipeout[0], &c, 1) == 1) {
                output += c;
            }

            close(pipeout[0]);
            if (!output.empty() && output.at(output.length()-1) == '\n')
                output.pop_back();

            return true;
        } else {
            if (!useStdErr)
                error("Failed to execute the command: {}", fmt::join(cmd, " "));
        }
    } else if (pid == 0) {
        int nullFile = open("/dev/null", O_WRONLY | O_CLOEXEC);
        dup2(pipeout[1], useStdErr ? STDERR_FILENO : STDOUT_FILENO);
        dup2(nullFile, useStdErr ? STDOUT_FILENO : STDERR_FILENO);

        setenv("LANG", "C", 1);
        cmd.push_back(nullptr);
        execvp(cmd[0], const_cast<char *const *>(cmd.data()));

        die("An error has occurred: {}", strerror(errno));
    } else {
        close(pipeout[0]);
        close(pipeout[1]);
        die("fork() failed: {}", strerror(errno));
    }

    close(pipeout[0]);
    close(pipeout[1]);

    return false;
}

std::string str_tolower(std::string str) {
    for (char& x : str)
        x = std::tolower(x); 
    
    return str;
}

std::string str_toupper(std::string str) {
    for (char& x : str)
        x = std::toupper(x); 
    
    return str;
}

// Function to perform binary search on the pci vendors array to find a device from a vendor.
std::string binarySearchPCIArray(const std::string_view vendor_id_s, const std::string_view pci_id_s) {
    const std::string_view vendor_id = hasStart(vendor_id_s, "0x") ? vendor_id_s.substr(2) : vendor_id_s;
    const std::string_view pci_id    = hasStart(pci_id_s, "0x") ? pci_id_s.substr(2) : pci_id_s;

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
    const std::string_view vendor_id = hasStart(vendor_id_s, "0x") ? vendor_id_s.substr(2) : vendor_id_s;

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
    dev_entry_pos += 6; // Offset from the first character to the actual name that we want (xxxx  <device name>)

    std::string name = all_ids.substr(dev_entry_pos, all_ids.find('\n', dev_entry_pos) - dev_entry_pos);

    size_t bracket_open_pos = name.find('[');
    size_t bracket_close_pos = name.find(']');
    if (bracket_open_pos != std::string::npos && bracket_close_pos != std::string::npos)
        name = name.substr(bracket_open_pos + 1, bracket_close_pos - bracket_open_pos - 1);

    return name;
}

std::string vendor_from_entry(size_t vendor_entry_pos, const std::string_view vendor_id) {
    size_t end_line_pos = all_ids.find('\n', vendor_entry_pos);
    if (end_line_pos == std::string::npos)
        end_line_pos = all_ids.length(); // If no newline is found, set to end of string

    std::string line = all_ids.substr(vendor_entry_pos, end_line_pos - vendor_entry_pos);

    size_t      after_id_pos = line.find(vendor_id) + 4;
    std::string description  = line.substr(after_id_pos);

    size_t first = description.find_first_not_of(' ');
    if (first == std::string::npos)
        return UNKNOWN;

    size_t last = description.find_last_not_of(' ');

    std::string vendor = description.substr(first, (last - first + 1));
    
    if (vendor == "Advanced Micro Devices, Inc.")
        vendor = "AMD";
    else if (vendor == "Intel Corporation")
        vendor = "Intel";
    else if (vendor == "NVIDIA Corporation")
        vendor = "NVIDIA";

    return vendor;
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
