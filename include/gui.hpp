#ifndef _GUI_HPP
#define _GUI_HPP

#ifdef GUI_SUPPORT

#include "gtkmm/window.h"
#include "gtkmm/label.h"
#include "gtkmm/box.h"
#include "gtkmm/container.h"
#include "gtkmm/image.h"

namespace GUI {

class Window : public Gtk::Window {
public:
    Window();
    virtual ~Window();

private:
    Gtk::Box m_box;
    Gtk::Label m_label;
    Gtk::Image *m_img;
};

}

#endif // GUI_SUPPORT

#endif // _GUI_HPP
