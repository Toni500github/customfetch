#include "boxd.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

#include "util.hpp"
#include "utf8/checked.h"

namespace {
inline void trim_ws(std::string& s)
{
    static constexpr const char* WS = " \t\n\r\f\v";
    const size_t start = s.find_first_not_of(WS);
    if (start == std::string::npos) {
        s.clear();
        return;
    }
    const size_t end = s.find_last_not_of(WS);
    s = s.substr(start, end - start + 1);
}
} 

size_t get_visual_width(const std::string& input);

static std::string strip_ansi(const std::string& str)
{
    std::string result;
    result.reserve(str.size());
    bool in_escape = false;

    for (size_t i = 0; i < str.size(); ++i)
    {
        if (str[i] == '\033' && i + 1 < str.size() && str[i + 1] == '[')
        {
            in_escape = true;
            i++; // Skip the '['
            continue;
        }
        if (in_escape)
        {
            if (str[i] == 'm')
                in_escape = false;
            continue;
        }
        result += str[i];
    }
    return result;
}

std::vector<std::string> apply_box_drawing(std::vector<std::string> layout,
                                           const Config& config) noexcept
{
    if (layout.empty())
        return layout;

    const auto& bc = config.box_chars;

    // Pass 1: tokenise rows, compute max width per column
    struct Row {
        std::vector<std::string> tokens; // coloured tokens per column
        bool has_separator = false;
        std::string original_line;
    };

    std::vector<Row> rows;
    std::vector<size_t> max_widths; // visual width per column

    auto ensure_columns = [&](size_t count) {
        if (max_widths.size() < count)
            max_widths.resize(count, 0);
    };

    for (const std::string& raw_line : layout)
    {
        Row row;
        row.original_line = raw_line;

        const bool contains_fill = raw_line.find("$fill") != std::string::npos;
        if (raw_line.find('|') == std::string::npos) {
            // Treat as single-column data row when no $fill present, else template
            if (!contains_fill) {
                row.tokens.push_back(raw_line);
                ensure_columns(1);

                std::string clean = strip_ansi(raw_line);
                trim_ws(clean);
                if (max_widths.empty()) max_widths.push_back(0);
                max_widths[0] = std::max(max_widths[0], get_visual_width(clean));
            }
            rows.push_back(std::move(row));
            continue;
        }

        row.has_separator = true;
        std::string token;
        bool in_escape = false;
        for (char ch : raw_line)
        {
            if (ch == '\033') in_escape = true;
            if (!in_escape && ch == '|') {
                row.tokens.push_back(token);
                token.clear();
                continue;
            }
            token.push_back(ch);
            if (in_escape && ch == 'm') in_escape = false;
        }
        row.tokens.push_back(token);
        ensure_columns(row.tokens.size());

        for (size_t col = 0; col < row.tokens.size(); ++col)
        {
            std::string clean = strip_ansi(row.tokens[col]);
            trim_ws(clean);
            max_widths[col] = std::max(max_widths[col], get_visual_width(clean));
        }
        rows.emplace_back(std::move(row));
    }

    for(size_t c=0; c<max_widths.size(); ++c)
        max_widths[c] += std::max<int>(0, config.box_extra_padding);

    const size_t cols = max_widths.size();
    if (cols == 0) { // No columns found, just expand macros and return
        std::vector<std::string> result;
        for(const auto& l : layout) {
            // A minimal expander for template-only layouts
            std::string expanded = l;
            size_t start_pos = 0;
            while((start_pos = expanded.find("\$", start_pos)) != std::string::npos) {
                expanded.replace(start_pos, 2, "$");
                start_pos += 1;
            }
            result.push_back(expanded);
        }
        return result;
    }
    
    // Pass 2: build macro replacement table
    std::unordered_map<std::string, std::string> macros;
    auto repeat = [&](const std::string& unit, size_t n) {
        if (unit.empty() || n == 0) return std::string();
        std::string out;
        out.reserve(get_visual_width(unit) * n);
        for (size_t i = 0; i < n; ++i) out += unit;
        return out;
    };

    size_t inner_width = 0;
    for(size_t c = 0; c < cols; ++c) {
        inner_width += max_widths[c] + 2; // content + 2 spaces padding
    }
    inner_width += (cols > 1 ? cols - 1 : 0); // vertical separators

    macros["$width"] = repeat(bc.horizontal, inner_width);
    size_t models_seg = max_widths[0] + 2;
    macros["$models.width"] = repeat(bc.horizontal, models_seg);
    size_t rest_seg_len = (inner_width > models_seg) ? (inner_width - models_seg - 1) : 0; // -1 for the '┬' or '┴' junction char
    macros["$rest.width"] = macros["$remaining.width"] = repeat(bc.horizontal, rest_seg_len);

    size_t accumulated = 0;
    for (size_t c = 0; c < cols; ++c) {
        size_t col_seg = max_widths[c] + 2;
        macros["$col" + std::to_string(c) + ".width"] = repeat(bc.horizontal, col_seg);
        accumulated += col_seg;
        // separators before next column equal to c (0-based) so far
        size_t consumed_junctions = c; // number of delimiters already passed
        size_t after_width = (inner_width > (accumulated + consumed_junctions)) ? (inner_width - accumulated - consumed_junctions) : 0;
        macros["$after" + std::to_string(c) + ".width"] = repeat(bc.horizontal, after_width);
    }

    auto macro_expand = [&](std::string line) {
        // Replace all occurrences of known macros
        for (const auto& pair : macros) {
            if(pair.first == "$fill") continue; // skip special dynamic token
            size_t start_pos = 0;
            while((start_pos = line.find(pair.first, start_pos)) != std::string::npos) {
                line.replace(start_pos, pair.first.length(), pair.second);
                start_pos += pair.second.length();
            }
        }
        // handle $fill dynamic horizontals (greedy left-to-right)
        while(true) {
            size_t fpos = line.find("$fill");
            if(fpos == std::string::npos) break;

            // visual width of everything before this $fill
            std::string prefix = line.substr(0, fpos);
            size_t prefix_vis = get_visual_width(strip_ansi(prefix));

            // static suffix width (remove all remaining $fill placeholders)
            std::string suffix = line.substr(fpos + 5);
            size_t s_pos = 0;
            while((s_pos = suffix.find("$fill", s_pos)) != std::string::npos) {
                suffix.erase(s_pos, 5);
            }
            size_t suffix_vis = get_visual_width(strip_ansi(suffix));

            size_t target_total = inner_width + 2;
            size_t needed = (prefix_vis + suffix_vis < target_total) ? (target_total - prefix_vis - suffix_vis) : 0;
            std::string rep = repeat(bc.horizontal, needed);
            line.replace(fpos, 5, rep);
        }
        // Handle escaped dollar signs
        size_t start_pos = 0;
        while((start_pos = line.find("\$", start_pos)) != std::string::npos) {
            line.replace(start_pos, 2, "$");
            start_pos += 1;
        }
        return line;
    };

    // Pass 3: build final layout
    std::vector<std::string> boxed;
    for (const Row& row : rows)
    {
        if (row.tokens.empty()) {
            boxed.push_back(macro_expand(row.original_line));
            continue;
        }

        std::string line = bc.vertical;
        for (size_t c = 0; c < cols; ++c)
        {
            std::string cell = (c < row.tokens.size()) ? row.tokens[c] : "";
            trim_ws(cell);

            std::string clean_cell = strip_ansi(cell);
            size_t visual_width = get_visual_width(clean_cell);
            
            size_t padding = (max_widths[c] > visual_width) ? (max_widths[c] - visual_width) : 0;

            std::string seg = " ";
            seg += cell;
            seg.append(padding, ' ');
            seg += ' ';

            line += seg;
            line += (c == cols - 1 ? bc.vertical : bc.vertical);
        }
        // ensure alignment of the last vertical bar by padding if visual width is short
        {
            std::string line_no_ansi = strip_ansi(line);
            size_t visual = get_visual_width(line_no_ansi);
            size_t expected_total = inner_width + 2; // starting and ending vertical bars
            if (visual < expected_total)
            {
                size_t diff = expected_total - visual;
                // insert spaces right before the final vertical character
                size_t insert_pos = line.size() - bc.vertical.size();
                line.insert(insert_pos, diff, ' ');
            }
        }
        boxed.push_back(macro_expand(line));
    }

    return boxed;
}


