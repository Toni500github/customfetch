#include "boxd.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <locale>
#include <codecvt>
#include <cwchar>
#include <iostream>
#include <iterator>

#include "util.hpp"
#include "utf8/checked.h"

// Anonymous namespace to keep helpers local to this file
namespace {

// --- Data Structures ---

// Holds the calculated dimensions for a single room.
struct RoomLayoutInfo {
    size_t pin_position = 0; // The column where the $<pin> marker should align.
    size_t total_width = 0;  // The total visual width of the fully rendered room.
};

// Represents a single room defined by $<room> and $<endroom>.
struct Room {
    std::vector<std::string> lines; // The raw lines of content inside the room.
    RoomLayoutInfo layout_info;     // The calculated layout dimensions.
};


// --- Utility Functions ---

static void trim_ws(std::string& s) {
    static constexpr const char* WS = " \t\n\r\f\v";
    const size_t start = s.find_first_not_of(WS);
    if (start == std::string::npos) {
        s.clear();
        return;
    }
    const size_t end = s.find_last_not_of(WS);
    s = s.substr(start, end - start + 1);
}

static size_t get_visual_width(const std::string& s) {
    if (s.empty()) {
        return 0;
    }
    std::wstring wstr;
    try {
        utf8::utf8to16(s.begin(), s.end(), std::back_inserter(wstr));
    } catch (const utf8::exception&) {
        return s.length(); // Fallback on invalid UTF-8
    }
    
    int width = wcswidth(wstr.c_str(), wstr.length());
    
    return (width < 0) ? wstr.length() : static_cast<size_t>(width);
}


static std::string strip_ansi(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    bool in_escape = false;
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '\033' && i + 1 < str.size() && str[i + 1] == '[') {
            in_escape = true;
            i++;
            continue;
        }
        if (in_escape) {
            if (str[i] == 'm') in_escape = false;
            continue;
        }
        result += str[i];
    }
    return result;
}


// --- Core Logic ---

// Calculates the required layout dimensions for a room based on the $<pin> marker.
RoomLayoutInfo calculate_room_layout(const std::vector<std::string>& room_lines) {
    RoomLayoutInfo info;
    size_t max_left_width = 0;
    size_t max_right_width = 0;

    for (const auto& line : room_lines) {
        size_t pin_pos = line.find("$<pin>");
        if (pin_pos != std::string::npos) {
            std::string left_part = line.substr(0, pin_pos);
            std::string right_part = line.substr(pin_pos + 6); // length of "$<pin>"

            max_left_width = std::max(max_left_width, get_visual_width(strip_ansi(left_part)));
            max_right_width = std::max(max_right_width, get_visual_width(strip_ansi(right_part)));
        }
    }

    info.pin_position = max_left_width;
    info.total_width = max_left_width + max_right_width;
    
    return info;
}

// Processes a single line from a room, applying all formatting.
std::string process_room_line(std::string line, const Config& config, const RoomLayoutInfo& layout_info) {
    // 1. Handle $<pin> alignment
    size_t pin_pos = line.find("$<pin>");
    if (pin_pos != std::string::npos) {
        std::string left_part = line.substr(0, pin_pos);
        std::string right_part = line.substr(pin_pos + 6); // length of "$<pin>"

        size_t left_width = get_visual_width(strip_ansi(left_part));
        size_t right_width = get_visual_width(strip_ansi(right_part));

        size_t left_padding = (layout_info.pin_position > left_width) ? (layout_info.pin_position - left_width) : 0;

        size_t max_right_width = layout_info.total_width - layout_info.pin_position;
        size_t right_padding = (max_right_width > right_width) ? (max_right_width - right_width) : 0;

        line = left_part + std::string(left_padding, ' ') + right_part + std::string(right_padding, ' ');
    }

    // 2. Handle $<fill> expansion
    while (true) {
        size_t fill_pos = line.find("$<fill>");
        if (fill_pos == std::string::npos) break;

        std::string temp_line = line;
        temp_line.erase(fill_pos, 7); // Remove the "$<fill>" marker
        size_t static_width = get_visual_width(strip_ansi(temp_line));
        
        size_t fill_needed = (layout_info.total_width > static_width) ? (layout_info.total_width - static_width) : 0;
        
        std::string fill_str;
        if (!config.box_chars.horizontal.empty()) {
            for (size_t i = 0; i < fill_needed; ++i) {
                fill_str += config.box_chars.horizontal;
            }
        }
        line.replace(fill_pos, 7, fill_str);
    }

    return line;
}

} // anonymous namespace

// Main function to apply the box drawing logic.
std::vector<std::string> apply_box_drawing(std::vector<std::string> layout, const Config& config) noexcept
{
    if (!config.box_drawing_enabled) {
        return layout;
    }

    std::vector<std::string> final_layout;
    
    for (size_t i = 0; i < layout.size(); ++i) {
        std::string current_line = layout[i];
        trim_ws(current_line);
        
        if (current_line.find("$<room>") != std::string::npos) {
            // Found the start of a room, now find the end.
            Room current_room;
            size_t room_end_idx = i;
            bool room_found = false;

            for (size_t j = i + 1; j < layout.size(); ++j) {
                std::string inner_line = layout[j];
                trim_ws(inner_line);
                if (inner_line.find("$<endroom>") != std::string::npos) {
                    room_end_idx = j;
                    room_found = true;
                    break;
                }
                current_room.lines.push_back(layout[j]);
            }

            if (room_found) {
                // Calculate the layout for this specific room using the pin logic.
                current_room.layout_info = calculate_room_layout(current_room.lines);
                
                // Process and add each line of the room to the final layout.
                for (const auto& line : current_room.lines) {
                    final_layout.push_back(process_room_line(line, config, current_room.layout_info));
                }
                
                // Skip the main loop ahead to the end of the processed room.
                i = room_end_idx;
            } else {
                // No $<endroom> found, treat $<room> as a literal line.
                final_layout.push_back(layout[i]);
            }
        } else {
            // This line is not a room marker, add it as is.
            final_layout.push_back(layout[i]);
        }
    }
    
    return final_layout;
}
