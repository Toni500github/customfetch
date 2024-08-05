#ifndef _GUI_HPP
#define _GUI_HPP

#ifdef GUI_MODE

#include "config.hpp"
#include "gtkmm/window.h"
#include "gtkmm/label.h"
#include "gtkmm/box.h"
#include "gtkmm/container.h"
#include "gtkmm/image.h"

namespace GUI {

class Window : public Gtk::Window {
public:
    Window(Config& config, colors_t& colors);
    virtual ~Window();

private:
    Gtk::Box m_box;
    Gtk::Label m_label;
    Gtk::Image *m_img;
};

}

#endif // GUI_MODE

#endif // _GUI_HPP
