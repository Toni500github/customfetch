#include "boxd.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

#include "util.hpp"
#include "utf8/checked.h"
#include <locale>
#include <codecvt>
#include <cwchar>

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
static size_t get_visual_width(const std::string& input) {
    try {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::wstring wide = converter.from_bytes(input);
        size_t width = 0;
        for (wchar_t ch : wide) {
            int w = wcwidth(ch);
            width += (w < 0 ? 1 : w);
        }
        return width;
    } catch (...) {
        return input.length();
    }
}

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
    struct RowData {
        std::vector<std::string> cells; // coloured cells (data rows)
        bool is_data = false;           // true if row contains '|'
        bool has_vertical_macro = false; // true if row contains '$vertical'
        std::string raw;               // original line text
    };

    std::vector<RowData> rows;
    std::vector<size_t> max_col_width;

    auto ensure_cols = [&](size_t n) {
        if (max_col_width.size() < n)
            max_col_width.resize(n, 0);
    };

    for (const std::string& raw : layout) {
        RowData rd;
        rd.raw = raw;
        
        rd.has_vertical_macro = (raw.find("$vertical") != std::string::npos);
        rd.is_data = rd.has_vertical_macro || (raw.find('|') != std::string::npos) || (raw.find("$fill") == std::string::npos);

        if (rd.is_data) {
            std::string line_to_process = raw;
            if (rd.has_vertical_macro) {
                size_t pos = line_to_process.find("$vertical");
                line_to_process.replace(pos, 9, "|");
            }

            std::string token;
            bool in_esc = false;
            for (char ch : line_to_process) {
                if (ch == '\033') in_esc = true;
                if (!in_esc && ch == '|') {
                    rd.cells.emplace_back(std::move(token));
                    token.clear();
                    continue;
                }
                token.push_back(ch);
                if (in_esc && ch == 'm') in_esc = false;
            }
            rd.cells.emplace_back(std::move(token));
            ensure_cols(rd.cells.size());

            for (size_t c = 0; c < rd.cells.size(); ++c) {
                std::string clean = strip_ansi(rd.cells[c]);
                trim_ws(clean);
                size_t vw = get_visual_width(clean);
                max_col_width[c] = std::max(max_col_width[c], vw);
            }
        }
        rows.push_back(std::move(rd));
    }

    for (auto& w : max_col_width)
        w += std::max<int>(0, config.box_extra_padding);

    const size_t cols = max_col_width.size();
    if (cols == 0) return layout;

    auto repeat = [&](const std::string& unit, size_t n) {
        if (n == 0 || unit.empty()) return std::string();
        std::string out;
        out.reserve(unit.size() * n);
        for (size_t i = 0; i < n; ++i) out += unit;
        return out;
    };

    std::vector<std::string> rendered(rows.size());
    size_t max_inner_width = 0;

    for (size_t r = 0; r < rows.size(); ++r) {
        if (!rows[r].is_data) continue;

        std::string line;
        if (!rows[r].has_vertical_macro) line += bc.vertical;

        for (size_t c = 0; c < cols; ++c) {
            std::string cell = (c < rows[r].cells.size()) ? rows[r].cells[c] : "";
            trim_ws(cell);
            std::string clean = strip_ansi(cell);
            size_t vis = get_visual_width(clean);
            size_t pad = (max_col_width[c] > vis) ? (max_col_width[c] - vis) : 0;

            line += ' ';
            line += cell;
            line.append(pad, ' ');
            line += ' ';
            
            if (c < cols - 1) {
                 if (rows[r].has_vertical_macro && c == 0) {
                    line += bc.vertical;
                 } else if (!rows[r].has_vertical_macro) {
                    line += bc.vertical;
                 }
            }
        }
        if (!rows[r].has_vertical_macro) line += bc.vertical;
        rendered[r] = line;
        size_t inner_vis = get_visual_width(strip_ansi(line)) - (rows[r].has_vertical_macro ? 0 : 2);
        max_inner_width = std::max(max_inner_width, inner_vis);
    }

    const size_t target_total = max_inner_width + 2; // borders included
    std::vector<std::string> output;
    output.reserve(rows.size());

    auto expand_fill = [&](std::string line) {
        while (true) {
            size_t pos = line.find("$fill");
            if (pos == std::string::npos) break;

            std::string before = strip_ansi(line.substr(0, pos));
            std::string after  = strip_ansi(line.substr(pos + 5));
            size_t current = get_visual_width(before) + get_visual_width(after);
            size_t need = (current < target_total) ? (target_total - current) : 0;
            line.replace(pos, 5, repeat(bc.horizontal, need));
        }
        // unescape $
        size_t sp = 0;
        while ((sp = line.find("\\$", sp)) != std::string::npos) {
            line.replace(sp, 2, "$");
        }
        return line;
    };

    for (size_t i = 0; i < rows.size(); ++i) {
        if (rows[i].is_data) {
            output.push_back(rendered[i]);
        } else {
            output.push_back(expand_fill(rows[i].raw));
        }
    }
    return output;
}
