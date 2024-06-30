#ifdef GUI_SUPPORT

#define STB_IMAGE_IMPLEMENTATION
#include "gui.hpp"
#include "config.hpp"
#include "display.hpp"
#include "util.hpp"
#include "query.hpp"
#include "parse.hpp"
#include "fmt/ranges.h"
#include "stb_image.h"

#ifdef CF_UNIX
# include <magic.h>
#endif

#include "pangomm/fontdescription.h"
#include "gdkmm/pixbufanimation.h"

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
static std::vector<std::string>& render_with_image(Config& config, colors_t& colors) {
    systemInfo_t systemInfo{};

    int image_width, image_height, channels;
    
    // load the image and get its width and height
    unsigned char* img = stbi_load(config.source_path.c_str(), &image_width, &image_height, &channels, 0);

    if (img)
        stbi_image_free(img);
    else
        die("Unable to load image '{}'", config.source_path);

    for (std::string& include : config.includes) {
        std::vector<std::string> include_nodes = split(include, '.');

        switch (std::count(include.begin(), include.end(), '.')) 
        {   
            // only 1 element
            case 0:
                addModuleValues(systemInfo, include);
                break;
            case 1:
                addValueFromModule(systemInfo, include_nodes[0], include_nodes[1]);
                break;
            default:
                die("Include has too many namespaces!");
        }
    }

    for (std::string& layout : config.layouts) {
        layout = parse(layout, systemInfo, config, colors, true);
    }

    for (size_t i = 0; i < config.layouts.size(); i++) {
        for (size_t _ = 0; _ < config.offset; _++) // I use _ because we don't need it 
            config.layouts.at(i).insert(0, " ");
    }
    config.offset = 0;

    return config.layouts;
}

Window::Window(Config& config, colors_t& colors) {
    set_title("customfetch - Higly customizable and fast neofetch like program");
    set_default_size(800, 600);
    add(m_box);

    std::string path = config.m_display_distro ? Display::detect_distro(config) : config.source_path;
    if (!std::filesystem::exists(path))
        die("'{}' doesn't exist. Can't load image/text file", path);
    
    // check if the file is an image
    // using the same library that "file" uses
    // No extra bloatware nice
    magic_t myt = magic_open(MAGIC_CONTINUE|MAGIC_ERROR|MAGIC_MIME);
    magic_load(myt, NULL);
    std::string file_type = magic_file(myt, path.c_str());
    bool useImage = ((file_type.find("text") == std::string::npos) && !config.m_disable_source);
    magic_close(myt);
    
    // useImage can be either a gif or an image
    if (useImage && !config.m_disable_source) {
        Glib::RefPtr<Gdk::PixbufAnimation> img = Gdk::PixbufAnimation::create_from_file(path);
        m_img = Gtk::manage(new Gtk::Image());
        m_img->set(img);
        m_img->set_alignment(Gtk::ALIGN_CENTER);
        m_box.pack_start(*m_img);
    }

    // https://stackoverflow.com/a/76372996
    auto context = m_label.get_pango_context();
    Pango::FontDescription font(config.font);
    debug("font family = {}", font.get_family().raw());
    debug("font style = {}", fmt::underlying(font.get_style()));
    debug("font weight = {}", fmt::underlying(font.get_weight()));
    context->set_font_description(font);

    /*Gdk::RGBA fg_color;
    style_context->lookup_color("theme_fg_color", fg_color);
    std::string fg_color_str = rgba_to_hexstr(fg_color);*/
    
    std::string colored_text;
    if (useImage)
        colored_text = fmt::format("{}", fmt::join(render_with_image(config, colors), "\n"));
    else
        colored_text = fmt::format("{}", fmt::join(Display::render(config, colors), "\n"));

    m_label.set_markup(colored_text);
    m_label.set_alignment(Gtk::ALIGN_CENTER);
    m_box.pack_start(m_label);

    show_all_children();
}

Window::~Window() { }

#endif // GUI_SUPPORT
