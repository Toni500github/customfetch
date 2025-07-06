/*
 * Copyright 2025 Toni500git
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 * following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote
 * products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#if GUI_APP

#define STB_IMAGE_IMPLEMENTATION
#include "gui.hpp"

#include <unistd.h>

#include <array>
#include <filesystem>
#include <fstream>

#include "cufetch/config.hh"
#include "display.hpp"
#include "fmt/ranges.h"
#include "parse.hpp"
#include "query.hpp"
#include "stb_image.h"
#include "util.hpp"

#include "gdkmm/pixbufanimation.h"
#include "glibmm/refptr.h"
#include "gtkmm/cssprovider.h"
#include "glibmm/main.h"
#include "gtkmm/enums.h"

using namespace GUI;

// https://www.codespeedy.com/convert-rgb-to-hex-color-code-in-cpp/
/*static std::string rgba_to_hexstr(const Gdk::RGBA& color) {
    int red = color.get_red() * 255;
    int green = color.get_green() * 255;
    int blue = color.get_blue() * 255;

    std::stringstream ss;
    ss << "#" << std::hex << (red << 16 | green << 8 | blue);
    return ss.str();
}*/

// Display::render but only for images on GUI
static std::vector<std::string> render_with_image(const Config& config)
{
    std::string              path{ Display::detect_distro(config) };
    moduleMap_t              modulesInfo{};
    std::vector<std::string> layout{ config.args_layout.empty() ? config.layout : config.args_layout };

    int image_width, image_height, channels;

    // load the image and get its width and height
    unsigned char* img = stbi_load(config.source_path.c_str(), &image_width, &image_height, &channels, 0);

    if (!img)
        die(_("Unable to load image '{}'"), config.source_path);

    stbi_image_free(img);

    if (!config.ascii_logo_type.empty())
    {
        const size_t& pos = path.rfind('.');

        if (pos != std::string::npos)
            path.insert(pos, "_" + config.ascii_logo_type);
        else
            path += "_" + config.ascii_logo_type;
    }

    // this is just for parse() to auto add the distro colors
    std::ifstream            file(path, std::ios::binary);
    std::string              line, _;
    std::vector<std::string> tmp_layout;
    parse_args_t             parse_args{ modulesInfo, _, layout, tmp_layout, config, false };
    while (std::getline(file, line))
    {
        parse(line, parse_args);
        parse_args.no_more_reset = false;
    }

    parse_args.parsingLayout = true;
    for (size_t i = 0; i < layout.size(); ++i)
    {
        layout[i]                = parse(layout[i], parse_args);
        parse_args.no_more_reset = false;

        if (!tmp_layout.empty())
        {
            layout.erase(layout.begin() + i);
            layout.insert(layout.begin() + i, tmp_layout.begin(), tmp_layout.end());
            tmp_layout.clear();
        }
    }

    // erase each element for each instance of MAGIC_LINE
    layout.erase(std::remove_if(layout.begin(), layout.end(),
                                [](const std::string_view str) { return str.find(MAGIC_LINE) != std::string::npos; }),
                 layout.end());

    const unsigned int offset =
        (config.offset.back() == '%')
            ? Display::calc_perc(std::stof(config.offset.substr(0, config.offset.size() - 1)), image_width, 0)
            : std::stoi(config.offset);

    for (size_t i = 0; i < layout.size(); i++)
        for (size_t _ = 0; _ < offset; _++)  // I use _ because we don't need it
            layout.at(i).insert(0, " ");

    return layout;
}

bool Window::set_layout_markup()
{
    if (m_isImage)
    {
        if (!m_config.args_print_logo_only)
            m_label.set_markup(fmt::format("{}", fmt::join(render_with_image(m_config), "\n")));
    }
    else
    {
        m_label.set_markup(fmt::format("{}", fmt::join(Display::render(m_config, true, m_path, m_moduleMap), "\n")));
    }
    return true;
}

Window::Window(const Config& config, const std::filesystem::path& path, const moduleMap_t& moduleMap)
    : m_config(config), m_path(path), m_moduleMap(moduleMap), m_isImage(false)
{
    set_title("customfetch - Higly customizable and fast neofetch like program");
    set_default_size(1000, 600);
    set_position(Gtk::WIN_POS_CENTER_ALWAYS);
    set_icon_from_file(ICONPREFIX "/customfetch/Thumbnail.png");

    debug("Window::Window analyzing file");
    std::ifstream                 f(path.c_str());
    std::array<unsigned char, 32> buffer;
    f.read(reinterpret_cast<char*>(&buffer.at(0)), buffer.size());
    if (is_file_image(buffer.data()))
        m_isImage = true;

    // useImage can be either a gif or an image
    if (m_isImage && !config.args_disable_source)
    {
        const auto& img = Gdk::PixbufAnimation::create_from_file(path.c_str());
        m_img           = Gtk::manage(new Gtk::Image(img));
        m_img->set(img);
        m_img->set_alignment(Gtk::ALIGN_CENTER);
        m_box.pack_start(*m_img, Gtk::PACK_SHRINK);
    }

    m_box.set_orientation(Gtk::ORIENTATION_HORIZONTAL);

    this->set_layout_markup();
    if (is_live_mode)
        Glib::signal_timeout().connect(sigc::mem_fun(*this, &Window::set_layout_markup), config.loop_ms);

    if (config.gui_bg_image != "disable")
    {
        if (!std::filesystem::exists(config.gui_bg_image))
            die(_("Background image path '{}' doesn't exist"), config.gui_bg_image);

        m_bg_animation = Gdk::PixbufAnimation::create_from_file(config.gui_bg_image);
        if (m_bg_animation->is_static_image())
        {
            // set it initially and resize only on window resize
            m_bg_static_image = m_bg_animation->get_static_image();
            update_static_image();
        }
        else
        {
            // animated image: use timer to update frames
            const int duration = m_bg_animation->get_iter(nullptr)->get_delay_time();
            Glib::signal_timeout().connect(sigc::mem_fun(*this, &Window::on_update_animation), duration);
        }
        m_overlay.add_overlay(m_bg_image);
    }

    if (config.gui_css_file != "disable")
    {
        if (!std::filesystem::exists(config.gui_css_file))
            die(_("Path to gtk css file '{}' doesn't exist"), config.gui_css_file);

        Glib::RefPtr<Gtk::CssProvider> css_provider = Gtk::CssProvider::create();
        Glib::RefPtr<Gdk::Screen> screen = Gdk::Screen::get_default();
        try
        {
            css_provider->load_from_path(config.gui_css_file);
        }
        catch (const Glib::Error& ex)
        {
            die(_("Failed to load CSS: {}"), ex.gobj()->message);
        }

        m_overlay.get_style_context()->add_provider_for_screen(screen, css_provider, GTK_STYLE_PROVIDER_PRIORITY_USER);
    }

    if (Display::ascii_logo_fd != -1)
    {
        ::remove(path.c_str());
        ::close(Display::ascii_logo_fd);
    }

    m_box.pack_start(m_label, Gtk::PACK_SHRINK);
    m_alignment.set(0.5, 0.5, 0, 0);
    m_alignment.add(m_box);
    m_overlay.add_overlay(m_alignment);
    add(m_overlay);

    signal_size_allocate().connect(sigc::mem_fun(*this, &Window::on_window_resize));

    show_all_children();
}

Window::~Window() {}

#endif  // GUI_APP
