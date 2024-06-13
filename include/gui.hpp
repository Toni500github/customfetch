#ifndef _GUI_HPP
#define _GUI_HPP

#ifdef GUI_SUPPORT

#include <gtkmm-3.0/gtkmm/window.h>
#include <gtkmm-3.0/gtkmm/label.h>

namespace GUI {

class MyWindow : public Gtk::Window {
public:
    std::string get_fgcolor();

    MyWindow();
    virtual ~MyWindow();
    Gtk::Label m_label;
    
};

}

#endif // GUI_SUPPORT

#endif // _GUI_HPP
