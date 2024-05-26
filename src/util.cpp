#include "util.hpp"
#include "pci.ids.hpp"
#include <algorithm>

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

// Function to perform binary search on the pci vendors array to find a vendor.
// Also looks for a device after that.
std::string binarySearchPCIArray(std::string_view vendor_id_s, std::string_view pci_id_s) {
    std::string_view vendor_id = hasStart(vendor_id_s, "0x") ? vendor_id_s.substr(2) : vendor_id_s;
    std::string_view pci_id = hasStart(pci_id_s, "0x") ? pci_id_s.substr(2) : pci_id_s;

    int left = 0, right = pci_vendors_array.size() - 1;
    
    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (pci_vendors_array[mid] == vendor_id)
            return name_from_entry(all_ids.find(pci_id, pci_vendors_location_array[mid])); // returns the index of the device.
        else if (pci_vendors_array[mid] < vendor_id)
            left = mid + 1;
        else
            right = mid - 1;
    }

    return "";
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

std::string vendor_from_id(const std::string& pci_ids, const std::string& id_str) {
    std::string id = hasStart(id_str, "0x") ? std::string(id_str.begin() + 2, id_str.end()) : id_str;

    // Step 1: Find the position of the ID in the pci_ids string
    size_t id_pos = pci_ids.find(id);
    if (id_pos == std::string::npos)
        return UNKNOWN; // ID not found

    // Step 2: Find the end of the line (newline character) starting from the ID position
    size_t end_line_pos = pci_ids.find('\n', id_pos);
    if (end_line_pos == std::string::npos)
        end_line_pos = pci_ids.length(); // If no newline is found, set to end of string

    // Step 3: Extract the substring from the ID position to the end of the line
    std::string line = pci_ids.substr(id_pos, end_line_pos - id_pos);

    // Step 4: Find the position after the ID (ID length plus possible spaces)
    size_t after_id_pos = line.find(id) + id.length();
    std::string description = line.substr(after_id_pos);

    size_t first = description.find_first_not_of(' ');
    if (first == std::string::npos)
        return UNKNOWN;

    size_t last = description.find_last_not_of(' ');
    return description.substr(first, (last - first + 1));
}
