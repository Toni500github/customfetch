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

#if GUI_MODE && !ANDROID_APP

#define STB_IMAGE_IMPLEMENTATION
#include "gui.hpp"

#include <array>
#include <filesystem>
#include <fstream>

#include "config.hpp"
#include "display.hpp"
#include "fmt/ranges.h"
#include "gdkmm/pixbufanimation.h"
#include "gtkmm/enums.h"
#include "glibmm/main.h"
#include "pangomm/fontdescription.h"
#include "parse.hpp"
#include "query.hpp"
#include "stb_image.h"
#include "util.hpp"

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
static std::vector<std::string> render_with_image(const Config& config, const colors_t& colors)
{
    std::string              path{ Display::detect_distro(config) };
    systemInfo_t             systemInfo{};
    std::vector<std::string> layout{ config.m_args_layout.empty() ? config.layout : config.m_args_layout };

    int image_width, image_height, channels;

    // load the image and get its width and height
    unsigned char* img = stbi_load(config.source_path.c_str(), &image_width, &image_height, &channels, 0);

    if (!img)
        die("Unable to load image '{}'", config.source_path);

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
    std::ifstream file(path, std::ios::binary);
    std::string   line, _;
    parse_args_t parse_args{ systemInfo, _, config, colors, false, true };

    while (std::getline(file, line))
        parse(line, parse_args);

    parse_args.parsingLayout = true;
    for (std::string& layout : layout)
        layout = parse(layout, parse_args);

    // erase each element for each instance of MAGIC_LINE
    layout.erase(std::remove_if(layout.begin(), layout.end(),
                                [](const std::string_view str) { return str.find(MAGIC_LINE) != std::string::npos; }),
                 layout.end());

    for (size_t i = 0; i < layout.size(); i++)
        for (size_t _ = 0; _ < config.offset; _++)  // I use _ because we don't need it
            layout.at(i).insert(0, " ");

    return layout;
}

Window::Window(const Config& config, const colors_t& colors, const std::string_view path)
{
    set_title("customfetch - Higly customizable and fast neofetch like program");
    set_default_size(1000, 600);
    set_position(Gtk::WIN_POS_CENTER_ALWAYS);

    bool useImage = false;

    debug("Window::Window analyzing file");
    std::ifstream                 f(path.data());
    std::array<unsigned char, 32> buffer;
    f.read(reinterpret_cast<char*>(&buffer.at(0)), buffer.size());
    if (is_file_image(buffer.data()))
        useImage = true;

    // useImage can be either a gif or an image
    if (useImage && !config.m_disable_source)
    {
        const auto& img = Gdk::PixbufAnimation::create_from_file(path.data());
        m_img           = Gtk::manage(new Gtk::Image(img));
        m_img->set(img);
        m_img->set_alignment(Gtk::ALIGN_CENTER);
        m_box.pack_start(*m_img, Gtk::PACK_SHRINK);
    }

    m_box.set_orientation(Gtk::ORIENTATION_HORIZONTAL);

    // https://stackoverflow.com/a/76372996
    Glib::RefPtr<Pango::Context> context = m_label.get_pango_context();
    Pango::FontDescription       font(config.font);
    debug("font family = {}", font.get_family().raw());
    debug("font style = {}", fmt::underlying(font.get_style()));
    debug("font weight = {}", fmt::underlying(font.get_weight()));
    context->set_font_description(font);

    /*Gdk::RGBA fg_color;
    style_context->lookup_color("theme_fg_color", fg_color);
    std::string fg_color_str = rgba_to_hexstr(fg_color);*/

    if (useImage)
    {
        if (!config.m_print_logo_only)
            m_label.set_markup(fmt::format("{}", fmt::join(render_with_image(config, colors), "\n")));
    }
    else
    {
        m_label.set_markup(fmt::format("{}", fmt::join(Display::render(config, colors, true, path), "\n")));
    }

    if (config.gui_bg_image != "disable")
    {
        if (!std::filesystem::exists(config.gui_bg_image))
            die("Background image path '{}' doesn't exist", config.gui_bg_image);

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

    m_box.pack_start(m_label, Gtk::PACK_SHRINK);
    m_alignment.set(0.5, 0.5, 0, 0);
    m_alignment.add(m_box);
    m_overlay.add_overlay(m_alignment);
    add(m_overlay);

    signal_size_allocate().connect(sigc::mem_fun(*this, &Window::on_window_resize));

    show_all_children();
}

Window::~Window() {}

#endif  // GUI_MODE && !ANDROID_APP
