#ifdef GUI_SUPPORT

#include "gui.hpp"
#include "config.hpp"
#include "display.hpp"
#include "util.hpp"
#include "fmt/ranges.h"

using namespace GUI;

MyWindow::MyWindow() {
    set_title("Testing");
    set_default_size(1000, 800);

    std::string colored_text = fmt::format("{}", fmt::join(Display::render(), "\n")); //create_colored_text();

    m_label.set_markup(colored_text);
    add(m_label);
    m_label.show();
}

MyWindow::~MyWindow() { }

std::string MyWindow::create_colored_text() {
    
    std::string red_text = fmt::format("<span foreground='{}'>Red text</span>", color.red);
    std::string green_text = fmt::format("<span foreground='{}'>Green Text</span>", color.green);
    std::string blue_text = fmt::format("<span foreground='{}'>Blue Text</span>", color.blue);

    std::string result = red_text + green_text + blue_text;
    return result;
}

#endif // GUI_SUPPORT
