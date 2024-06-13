#ifdef GUI_SUPPORT

#include "gui.hpp"
#include "display.hpp"
#include "fmt/ranges.h"

using namespace GUI;

MyWindow::MyWindow() {
    set_title("Testing");
    set_default_size(800, 600);
    
    // https://stackoverflow.com/a/76372996
    auto context = m_label.get_pango_context();
    auto font = context->get_font_description();
    font.set_family("Liberation Mono");
    //font.set_weight(Pango::WEIGHT_BOLD);
    font.set_size(12 * PANGO_SCALE);
    context->set_font_description(font);
    //m_label.set_halign(Gtk::ALIGN_CENTER);

    std::string colored_text = fmt::format("{}", fmt::join(Display::render(get_fgcolor()), "\n")); //create_colored_text();

    m_label.set_markup(colored_text);
    add(m_label);
    m_label.show();
}

MyWindow::~MyWindow() { }


static std::string rgba_to_hexstr(const Gdk::RGBA& color) {
    int red = color.get_red() * 255;
    int green = color.get_green() * 255;
    int blue = color.get_blue() * 255;
    
    std::stringstream ss;
    ss << "#" << std::hex << (red << 16 | green << 8 | blue );    
    return ss.str();
}

std::string MyWindow::get_fgcolor() {
    auto style_context = m_label.get_style_context();
    Gdk::RGBA fg_color;
    style_context->lookup_color("theme_fg_color", fg_color);
    
    return rgba_to_hexstr(fg_color);
}

#endif // GUI_SUPPORT
