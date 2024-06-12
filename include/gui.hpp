#ifndef _GUI_HPP
#define _GUI_HPP

#ifdef GUI_SUPPORT

#include <gtkmm-3.0/gtkmm/window.h>
#include <gtkmm-3.0/gtkmm/label.h>

namespace GUI {

class MyWindow : public Gtk::Window {
public:
    MyWindow();
    virtual ~MyWindow();
private:
    Gtk::Label m_label;

    std::string create_colored_text();
};

}

#endif // GUI_SUPPORT

#endif // _GUI_HPP
