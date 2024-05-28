#include "util.hpp"
#include "pci.ids.hpp"
#include <algorithm>
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
    std::string            line;
    std::vector<std::string>    vec;
    std::stringstream ss(text.data());
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

void parse(std::string& input) {
  size_t dollarSignIndex = 0;

  while (true) {
      size_t oldDollarSignIndex = dollarSignIndex;
      dollarSignIndex = input.find('$', dollarSignIndex);

      if (dollarSignIndex == std::string::npos || dollarSignIndex <= oldDollarSignIndex)
          break;

      // check for bypass
      if (dollarSignIndex > 0 and input[dollarSignIndex - 1] == '\\')
          continue;

      std::string command = "";
      size_t endBracketIndex = -1;

      char type = ' '; // ' ' = undefined, ')' = shell exec, 2 = ')' asking for a module

      switch (input[dollarSignIndex+1]) {
          case '(':
              type = ')';
              break;
          case '<':
              die("PARSER: Not implemented: module types (<gpu.name>)");
              type = '>';
              break;
          default: // neither of them
              break;
      }

      if (type == ' ')
          continue;

      for (size_t i = dollarSignIndex+2; i < input.size(); i++) {
          if (input[i] == type && input[i-1] != '\\') {
              endBracketIndex = i;
              break;
          } else if (input[i] == type)
              command.erase(command.size()-1, 1);

          command += input[i];
      }

      if (endBracketIndex == -1)
          die("PARSER: Opened tag is not closed at index {} in string {}.", dollarSignIndex, input);
      
      switch (type) {
          case ')':
              input = input.replace(dollarSignIndex, (endBracketIndex+1)-dollarSignIndex, shell_exec(command));
              break;
          case '>':
              input = input.replace(dollarSignIndex, (endBracketIndex+1)-dollarSignIndex, shell_exec(command));
              break;
      }
  }
}

// Function to perform binary search on the pci vendors array to find a vendor.
// Also looks for a device after that.
std::string binarySearchPCIArray(std::string_view vendor_id_s, std::string_view pci_id_s) {
    std::string_view vendor_id = hasStart(vendor_id_s, "0x") ? vendor_id_s.substr(2) : vendor_id_s;
    std::string_view pci_id = hasStart(pci_id_s, "0x") ? pci_id_s.substr(2) : pci_id_s;

    long location_array_index = std::distance(pci_vendors_array.begin(), std::lower_bound(pci_vendors_array.begin(), pci_vendors_array.end(), vendor_id));
    
    return name_from_entry(all_ids.find(pci_id, pci_vendors_location_array[location_array_index]));
}

// http://stackoverflow.com/questions/478898/ddg#478960
std::string shell_exec(std::string_view cmd) {
    std::array<char, 128> buffer;
    std::string result;
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
