#ifdef GUI_MODE

#define STB_IMAGE_IMPLEMENTATION
#include "gui.hpp"

#include <filesystem>
#include <fstream>

#include "config.hpp"
#include "display.hpp"
#include "fmt/ranges.h"
#include "gdkmm/pixbufanimation.h"
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
static std::vector<std::string>& render_with_image(Config& config, colors_t& colors)
{
    systemInfo_t systemInfo{};

    int image_width, image_height, channels;

    // load the image and get its width and height
    unsigned char* img = stbi_load(config.source_path.c_str(), &image_width, &image_height, &channels, 0);

    if (img)
        stbi_image_free(img);
    else
        die("Unable to load image '{}'", config.source_path);

    for (std::string& layout : config.layouts)
    {
        std::string _;
        layout = parse(layout, systemInfo, _, config, colors, true);
    }

    std::erase_if(config.layouts, [](const std::string_view str) { return str.find(MAGIC_LINE) != std::string::npos; });

    for (size_t i = 0; i < config.layouts.size(); i++)
    {
        for (size_t _ = 0; _ < config.offset; _++)  // I use _ because we don't need it
            config.layouts.at(i).insert(0, " ");
    }
    config.offset = 0;

    return config.layouts;
}

Window::Window(Config& config, colors_t& colors)
{
    set_title("customfetch - Higly customizable and fast neofetch like program");
    set_default_size(1000, 800);
    add(m_box);

    std::string path = config.m_display_distro ? Display::detect_distro(config) : config.source_path;
    if (!std::filesystem::exists(path) &&
        !std::filesystem::exists((path = config.data_dir + "/ascii/linux.txt")))
        die("'{}' doesn't exist. Can't load image/text file", path);

    bool useImage = false;

    debug("Window::Window analyzing file");
    std::ifstream f(path);
    unsigned char buffer[128];
    f.read((char*)(&buffer[0]), 128);
    if (is_file_image(buffer))
        useImage = true;

    // useImage can be either a gif or an image
    if (useImage && !config.m_disable_source)
    {
        Glib::RefPtr<Gdk::PixbufAnimation> img = Gdk::PixbufAnimation::create_from_file(path);
        m_img                                  = Gtk::manage(new Gtk::Image());
        m_img->set(img);
        m_img->set_alignment(Gtk::ALIGN_CENTER);
        m_box.pack_start(*m_img);
    }

    // https://stackoverflow.com/a/76372996
    auto                   context = m_label.get_pango_context();
    Pango::FontDescription font(config.font);
    debug("font family = {}", font.get_family().raw());
    debug("font style = {}", fmt::underlying(font.get_style()));
    debug("font weight = {}", fmt::underlying(font.get_weight()));
    context->set_font_description(font);

    /*Gdk::RGBA fg_color;
    style_context->lookup_color("theme_fg_color", fg_color);
    std::string fg_color_str = rgba_to_hexstr(fg_color);*/

    std::string markup_text;
    if (useImage)
    {
        if (!config.m_print_logo_only)
            markup_text = fmt::format("{}", fmt::join(render_with_image(config, colors), "\n"));
    }
    else
        markup_text = fmt::format("{}", fmt::join(Display::render(config, colors, true, path), "\n"));

    m_label.set_markup(markup_text);
    m_label.set_alignment(Gtk::ALIGN_CENTER);
    m_box.pack_start(m_label);

    show_all_children();
}

Window::~Window() {}

#endif  // GUI_MODE
