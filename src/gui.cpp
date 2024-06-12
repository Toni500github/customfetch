#ifdef GUI_SUPPORT

#include "gui.hpp"
#include "config.hpp"
#include "display.hpp"
#include "util.hpp"
#include "fmt/ranges.h"

using namespace GUI;

MyWindow::MyWindow() {
    set_title("GTKmm Example with fmt");
    set_default_size(1000, 800);

    // Create colored text using fmt library
    std::string colored_text = fmt::format("{}", fmt::join(Display::render(), "\n")); //create_colored_text();

    // Set the Pango markup text to the label
    m_label.set_markup(colored_text);

    // Add the label to the window
    add(m_label);

    // Make the label visible
    m_label.show();
}

MyWindow::~MyWindow() { }

std::string MyWindow::create_colored_text() {
    
    std::string red_text = fmt::format("<span foreground='{}'>Red text</span>", color.red);
    std::string green_text = fmt::format("<span foreground='{}'>Green Text</span>", color.green);
    std::string blue_text = fmt::format("<span foreground='{}'>Blue Text</span>", color.blue);

    // Concatenate the styled strings with proper line breaks
    std::string result = red_text + green_text + blue_text;
    return result;
}

#endif // GUI_SUPPORT
