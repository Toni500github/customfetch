#include "boxd.hpp"

#include <algorithm>
#include <string>
#include <vector>
#include <locale>
#include <codecvt>
#include <cwchar>

#include "util.hpp"
#include "utf8/checked.h" // i fixed the warnings here btw

namespace {
// the concept of our "room" begans here:
// this Holds the calculated dimensions for a room, ensuring all lines align correctly.
struct RoomLayoutInfo {
    // The column position where the content after the separator should start.
    size_t pin_position = 0; 
    // The total visual width of the entire room, from the start of the line to the end.
    size_t total_width = 0;
};

struct Room {
    std::vector<std::string> lines;
    RoomLayoutInfo layout_info;
};

// trim whitespace ..etc
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

/**
 * @brief Calculates the visual width of a UTF-8 string in a terminal.
 * This function handles multi-byte characters, emojis, and NerdFont glyphs
 * that may occupy more than one column. It converts the string to a wide string
 * and uses wcswidth for an accurate measurement.
 * @param s The UTF-8 string to measure.
 * @return The number of columns the string will occupy.
 */
static size_t get_visual_width(const std::string& s) {
    if (s.empty()) {
        return 0;
    }

    // Set the locale to ensure wcswidth works correctly with the system's character set.
    // An empty string for the locale uses the environment's default settings(your terminal usally).
    std::setlocale(LC_ALL, "");

    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    std::wstring wstr;
    try {
        wstr = conv.from_bytes(s);
    } catch (const std::range_error&) {
        // Fallback for invalid UTF-8: return byte length
        return s.length();
    }

    // wcswidth calculates the number of columns required for a wide-character string.
    // It returns -1 for non-printable characters.
    int width = wcswidth(wstr.c_str(), wstr.length());

    if (width == -1) {
        // Fallback for strings containing non-printable characters (e.g., control codes).
        return s.length(); // A simple fallback, just in case
    }

    return static_cast<size_t>(width);
}

// cleans ansi escape codes (e.g., for colors) from a string.
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
            if (std::isalpha(str[i])) { // Terminal codes end with a letter, usually 'm'
                in_escape = false;
            }
            continue;
        }
        result += str[i];
    }
    return result;
}

// this one cleans both ansi codes and layout markers like $<pin> and $<fill>.
static std::string strip_ansi_and_markers(const std::string& str) {
    std::string result = strip_ansi(str);
    
    auto erase_substr = [&](const std::string& sub) {
        size_t pos;
        while ((pos = result.find(sub)) != std::string::npos) {
            result.erase(pos, sub.length());
        }
    };

    erase_substr("$<pin>");
    erase_substr("$<fill>");

    return result;
}

/**
 * @brief Calculates room dimensions based on content, handling variable-width characters.
 *
 * This function robustly determines the required width of the room and the alignment
 * point for $<pin> markers by finding the maximum left and right parts independently.
 *
 * @param room_lines The lines of content within a $<room> block.
 * @return A RoomLayoutInfo struct with the calculated pin_position and total_width.
 */
RoomLayoutInfo calculate_room_layout(const std::vector<std::string>& room_lines) {
    RoomLayoutInfo info;
    size_t max_left_width = 0;
    size_t max_right_width = 0;
    size_t max_non_pinned_width = 0;

    //Finds the max widths for left/right parts of pinned lines, and for non-pinned lines
    for (const auto& line : room_lines) {
        size_t pin_pos = line.find("$<pin>");
        if (pin_pos != std::string::npos) {
            std::string left_part = line.substr(0, pin_pos);
            std::string right_part = line.substr(pin_pos + 6); // length of "$<pin>"

            max_left_width = std::max(max_left_width, get_visual_width(strip_ansi_and_markers(left_part)));
            max_right_width = std::max(max_right_width, get_visual_width(strip_ansi_and_markers(right_part)));
        } else {
            // For lines without a pin (like borders), their width contributes to the total room width.
            max_non_pinned_width = std::max(max_non_pinned_width, get_visual_width(strip_ansi_and_markers(line)));
        }
    }

    // The pin position is determined by the widest left part. This is where the right content will start.
    info.pin_position = max_left_width;
    
    // The total width is the maximum of either the widest non-pinned line (e.g., a border)
    // or the combined width of the widest left and right parts of the pinned lines.
    info.total_width = std::max(max_non_pinned_width, max_left_width + max_right_width);
    
    return info;
}

/**
 * @brief Formats a single line from a room, applying padding and fills.
 *
 * This function uses the pre-calculated layout info to correctly align content
 * and expand $<fill> markers to the full width of the room.
 *
 * @param line The raw line to process.
 * @param config The application configuration (for box characters).
 * @param layout_info The pre-calculated dimensions of the room.
 * @return The fully formatted and aligned string.
 */
std::string process_room_line(std::string line, const Config& config, const RoomLayoutInfo& layout_info) {
    // handle $<pin> alignment
    size_t pin_pos = line.find("$<pin>");
    if (pin_pos != std::string::npos) {
        std::string left_part = line.substr(0, pin_pos);
        std::string right_part = line.substr(pin_pos + 6); // length of "$<pin>"

        size_t left_width = get_visual_width(strip_ansi_and_markers(left_part));
        size_t right_width = get_visual_width(strip_ansi_and_markers(right_part));

        // calculate padding to align the separator (the end of the left part).
        size_t left_padding = (layout_info.pin_position > left_width) ? (layout_info.pin_position - left_width) : 0;

        // calculate padding to align the right edge of the box.
        size_t current_line_width_if_packed = layout_info.pin_position + right_width;
        size_t right_padding = (layout_info.total_width > current_line_width_if_packed) ? (layout_info.total_width - current_line_width_if_packed) : 0;

        // Reconstruct the line. The padding is inserted between the left and right parts.
        line = left_part + std::string(left_padding, ' ') + std::string(right_padding, ' ') + right_part;
    }

    // handle $<fill> expansion 
    size_t fill_count = 0;
    size_t search_pos = 0;
    while ((search_pos = line.find("$<fill>", search_pos)) != std::string::npos) {
        fill_count++;
        search_pos += 7; // length of "$<fill>"
    }

    if (fill_count > 0) {
        std::string line_no_fills = line;
        size_t pos;
        while ((pos = line_no_fills.find("$<fill>")) != std::string::npos) {
            line_no_fills.erase(pos, 7);
        }
        
        size_t static_width = get_visual_width(strip_ansi(line_no_fills));

        size_t total_fill_needed = (layout_info.total_width > static_width) ? (layout_info.total_width - static_width) : 0;
        size_t fill_per_marker = total_fill_needed / fill_count;
        size_t remainder = total_fill_needed % fill_count;

        for (size_t i = 0; i < fill_count; ++i) {
            size_t current_fill_pos = line.find("$<fill>");
            if (current_fill_pos == std::string::npos) break; // Should not happen!!!

            size_t current_fill_size = fill_per_marker + (i < remainder ? 1 : 0);
            
            std::string fill_str;
            if (!config.box_chars.horizontal.empty()) {
                for (size_t j = 0; j < current_fill_size; ++j) {
                    fill_str += config.box_chars.horizontal;
                }
            }
            line.replace(current_fill_pos, 7, fill_str);
        }
    }

    return line;
}

}

// Main function to process the entire layout, finding and formatting $<room> blocks.
std::vector<std::string> apply_box_drawing(std::vector<std::string> layout, const Config& config) noexcept {
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

            // Find the corresponding $<endroom>
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
                // Calculate layout dimensions for the entire room first.
                current_room.layout_info = calculate_room_layout(current_room.lines);
                
                // Then process each line using those consistent dimensions.
                for (const auto& line : current_room.lines) {
                    final_layout.push_back(process_room_line(line, config, current_room.layout_info));
                }
                
                // Skip the main loop past the processed room block.
                i = room_end_idx;
            } else {
                // No $<endroom> found, treat $<room> as literal text.
                final_layout.push_back(layout[i]);
            }
        } else {
            final_layout.push_back(layout[i]);
        }
    }
    
    return final_layout;
}
