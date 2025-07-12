#include "boxd.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <locale>
#include <cwchar>
#include <iostream>
#include <iterator>

#include "util.hpp"
#include "utf8/checked.h"

namespace {

struct RoomLayoutInfo {
    size_t pin_position = 0;
    size_t total_width = 0;
};

struct Room {
    std::vector<std::string> lines;
    RoomLayoutInfo layout_info;
};

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

// Calculates visual width of string, handling UTF-8 and wide characters.
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

// Strips ANSI escape codes and custom color tags from a string.
static std::string strip_ansi(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    bool in_escape = false;
    bool in_custom_color = false;
    for (size_t i = 0; i < str.size(); ++i) {
        // Handle ANSI escape codes (e.g., \u001b[...m)
        if (str[i] == '\u001b' && i + 1 < str.size() && str[i + 1] == '[') {
            in_escape = true;
            i++;
            continue;
        }
        if (in_escape) {
            if (str[i] == 'm') in_escape = false;
            continue;
        }

        // Handle custom color tags (e.g., ${red}, ${0})
        if (str[i] == '
 && i + 1 < str.size() && str[i + 1] == '{') {
            in_custom_color = true;
            i++;
            continue;
        }
        if (in_custom_color) {
            if (str[i] == '}') in_custom_color = false;
            continue;
        }
        result += str[i];
    }
    return result;
}

// Calculates room dimensions based on $<pin> for alignment.
RoomLayoutInfo calculate_room_layout(const std::vector<std::string>& room_lines) {
    RoomLayoutInfo info;
    size_t max_left_width = 0;
    size_t max_right_width = 0;

    for (const auto& line : room_lines) {
        size_t pin_pos = line.find("$<pin>");
        if (pin_pos != std::string::npos) {
            std::string left_part = line.substr(0, pin_pos);
            std::string right_part = line.substr(pin_pos + 6);

            max_left_width = std::max(max_left_width, get_visual_width(strip_ansi(left_part)));
            max_right_width = std::max(max_right_width, get_visual_width(strip_ansi(right_part)));
        }
    }

    info.pin_position = max_left_width;
    info.total_width = max_left_width + max_right_width;
    
    return info;
}

// Processes a single line within a room, applying $<pin> alignment and $<fill> expansion.
std::string process_room_line(std::string line, const Config& config, const RoomLayoutInfo& layout_info) {
    // Align content based on $<pin> marker.
    size_t pin_pos = line.find("$<pin>");
    if (pin_pos != std::string::npos) {
        std::string left_part = line.substr(0, pin_pos);
        std::string right_part = line.substr(pin_pos + 6);

        size_t left_width = get_visual_width(strip_ansi(left_part));
        size_t right_width = get_visual_width(strip_ansi(right_part));

        size_t left_padding = (layout_info.pin_position > left_width) ? (layout_info.pin_position - left_width) : 0;

        size_t max_right_width = layout_info.total_width - layout_info.pin_position;
        size_t right_padding = (max_right_width > right_width) ? (max_right_width - right_width) : 0;

        line = left_part + std::string(left_padding, ' ') + right_part + std::string(right_padding, ' ');
    }

    // Expand $<fill> with horizontal box characters.
    while (true) {
        size_t fill_pos = line.find("$<fill>");
        if (fill_pos == std::string::npos) break;

        std::string temp_line = line;
        temp_line.erase(fill_pos, 7);
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

// Applies box drawing logic to the layout.
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
                current_room.layout_info = calculate_room_layout(current_room.lines);
                
                for (const auto& line : current_room.lines) {
                    final_layout.push_back(process_room_line(line, config, current_room.layout_info));
                }
                
                i = room_end_idx;
            } else {
                final_layout.push_back(layout[i]);
            }
        } else {
            final_layout.push_back(layout[i]);
        }
    }
    
    return final_layout;
}
