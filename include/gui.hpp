/*
 * Copyright 2024 Toni500git
 * 
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _GUI_HPP
#define _GUI_HPP

#if GUI_MODE && !ANDROID_APP

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
    /**
     * Initialize and create everything and parse layout with source path.
     * @param config The config class
     * @param colors The non-alias colors struct
     * @param path The logo source path
     */
    Window(const Config& config, const colors_t& colors, const std::string_view path);
    // Destroy the window, handled by GTK
    virtual ~Window();

private:
    Gtk::Overlay                           m_overlay;
    Gtk::Box                               m_box;
    Gtk::Alignment                         m_alignment;
    Gtk::Label                             m_label;
    Gtk::Image *                           m_img, m_bg_image;
    Glib::RefPtr<Gdk::PixbufAnimationIter> m_iter;
    Glib::RefPtr<Gdk::PixbufAnimation>     m_bg_animation;
    Glib::RefPtr<Gdk::Pixbuf>              m_bg_static_image;
    int                                    m_width, m_height;

    // Update background image size (gif or static)
    // on window resize
    void on_window_resize(const Gtk::Allocation& allocation)
    {
        m_width  = allocation.get_width();
        m_height = allocation.get_height();

        if (m_bg_static_image)
            // static image: update to fit the new window size
            update_static_image();
        else if (m_iter)
            // gif: update the current frame
            update_frame();
    }

    // Update background gif size
    bool on_update_animation()
    {
        if (!m_iter)
            m_iter = m_bg_animation->get_iter(nullptr);
        else
            m_iter->advance();

        update_frame();
        return true;  // continue the timer
    }

    // Update background image size
    void update_static_image()
    {
        // scale the static image to fit the window size
        const Glib::RefPtr<Gdk::Pixbuf> scaled_image =
            m_bg_static_image->scale_simple(m_width, m_height, Gdk::INTERP_BILINEAR);
        m_bg_image.set(scaled_image);
    }

    // Update background gif size in the current frame
    void update_frame()
    {
        // scale the current frame of the animation to fit the window size
        const Glib::RefPtr<Gdk::Pixbuf> current_frame = m_iter->get_pixbuf();
        if (current_frame)
        {
            const Glib::RefPtr<Gdk::Pixbuf> scaled_frame =
                current_frame->scale_simple(m_width, m_height, Gdk::INTERP_BILINEAR);
            m_bg_image.set(scaled_frame);
        }
    }
};

}  // namespace GUI

#endif  // GUI_MODE && !ANDROID_APP

#endif  // _GUI_HPP
