#ifndef _GUI_HPP
#define _GUI_HPP

#ifdef GUI_MODE

#include "config.hpp"
#include "gdkmm/pixbuf.h"
#include "gdkmm/pixbufanimation.h"
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
    Glib::RefPtr<Gdk::PixbufAnimationIter> m_iter;
    Glib::RefPtr<Gdk::PixbufAnimation> m_bg_animation;
    Glib::RefPtr<Gdk::Pixbuf> m_bg_static_image;
    int m_width, m_height;

    void on_window_resize(Gtk::Allocation& allocation)
    {
        m_width = allocation.get_width();
        m_height = allocation.get_height();

        if (m_bg_static_image)
            // static image: Update to fit the new window size
            update_static_image();
        else if (m_iter)
            // animated image: Update the current frame
            update_frame();
    }

    bool on_update_animation()
    {
        if (!m_iter)
            m_iter = m_bg_animation->get_iter(nullptr);
        else
            m_iter->advance();

        update_frame();
        return true;  // continue the timer
    }

    void update_static_image()
    {
        // scale the static image to fit the window size
        const Glib::RefPtr<Gdk::Pixbuf> scaled_image = m_bg_static_image->scale_simple(m_width, m_height, Gdk::INTERP_BILINEAR);
        m_bg_image.set(scaled_image);
    }

    void update_frame()
    {
        // scale the current frame of the animation to fit the window size
        const Glib::RefPtr<Gdk::Pixbuf> current_frame = m_iter->get_pixbuf();
        if (current_frame)
        {
            const Glib::RefPtr<Gdk::Pixbuf> scaled_frame = current_frame->scale_simple(m_width, m_height, Gdk::INTERP_BILINEAR);
            m_bg_image.set(scaled_frame);
        }
    }

};

}  // namespace GUI

#endif  // GUI_MODE

#endif  // _GUI_HPP
