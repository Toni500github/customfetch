#ifndef _GUI_HPP
#define _GUI_HPP

#ifdef GUI_MODE

#include "config.hpp"
#include "gdkmm/pixbuf.h"
#include "gtkmm/alignment.h"
#include "gtkmm/box.h"
#include "gtkmm/container.h"
#include "gtkmm/image.h"
#include "gtkmm/label.h"
#include "gtkmm/overlay.h"
#include "gtkmm/window.h"

namespace GUI
{

class Window : public Gtk::Window
{
public:
    Window(const Config& config, const colors_t& colors, const std::string_view path);
    virtual ~Window();

private:
    Gtk::Overlay    m_overlay;
    Gtk::Box        m_box;
    Gtk::Alignment  m_alignment;
    Gtk::Label      m_label;
    Gtk::Image     *m_img, m_bg_image;
    Glib::RefPtr<Gdk::Pixbuf> m_original_pixbuf;

    void update_background_image(const int width, const int height)
    {
        if (m_original_pixbuf)
        {
            const Glib::RefPtr<Gdk::Pixbuf> scaled_pixbuf =
                m_original_pixbuf->scale_simple(width, height, Gdk::INTERP_BILINEAR);

            m_bg_image.set(scaled_pixbuf);
        }
    }

    // Signal handler for window resize
    void on_window_resized(Gtk::Allocation& allocation)
    {
        const int new_width  = allocation.get_width();
        const int new_height = allocation.get_height();

        // Update the background image with the new dimensions
        update_background_image(new_width, new_height);
    }
};

}  // namespace GUI

#endif  // GUI_MODE

#endif  // _GUI_HPP
