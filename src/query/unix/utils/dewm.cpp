#include "dewm.hpp"

#include <cstdlib>
#include <fstream>

#include "rapidxml-1.13/rapidxml.hpp"
#include "switch_fnv1a.hpp"
#include "util.hpp"

// https://github.com/fastfetch-cli/fastfetch/blob/a61765c8b1387777be67d967bc2f69031c8ca399/src/detection/displayserver/linux/wmde.c#L19
std::string parse_de_env(void) noexcept
{
    const char* env;

    env = std::getenv("XDG_CURRENT_DESKTOP");
    if (env != NULL && env[0] != '\0')
        return env;

    // maybe let's try to get the DE envs first, then try with the others
    if (std::getenv("KDE_FULL_SESSION") != NULL || std::getenv("KDE_SESSION_UID") != NULL ||
        std::getenv("KDE_SESSION_VERSION") != NULL)
        return "KDE";

    if (std::getenv("CINNAMON_VERSION") != NULL)
        return "Cinnamon";

    if (std::getenv("GNOME_DESKTOP_SESSION_ID") != NULL || std::getenv("GNOME_SETUP_DISPLAY") != NULL)
        return "GNOME";

    if (std::getenv("MATE_DESKTOP_SESSION_ID") != NULL)
        return "Mate";

    if (std::getenv("TDE_FULL_SESSION") != NULL)
        return "Trinity";

    if (std::getenv("HYPRLAND_CMD") != NULL)
        return "Hyprland";

    env = std::getenv("XDG_SESSION_DESKTOP");
    if (env != NULL && env[0] != '\0')
        return env;

    env = std::getenv("CURRENT_DESKTOP");
    if (env != NULL && env[0] != '\0')
        return env;

    env = std::getenv("SESSION_DESKTOP");
    if (env != NULL && env[0] != '\0')
        return env;

    env = std::getenv("DESKTOP_SESSION");
    if (env != NULL && env[0] != '\0')
        return env;

    return MAGIC_LINE;
}

std::string prettify_wm_name(const std::string_view name) noexcept
{
    if (hasStart(name, "kwin"))
        return "Kwin";

    // taken from this list
    // https://github.com/KittyKatt/screenFetch/blob/master/screenfetch-dev#L93
    // added some missing.
    // ngl this looks beatiful thanks to clang-format :D
    switch (fnv1a16::hash(str_tolower(name.data())))
    {
        case "2bwm"_fnv1a16:          return "2bwm";
        case "9wm"_fnv1a16:           return "9wm";
        case "awesome"_fnv1a16:       return "awesome";
        case "beryl"_fnv1a16:         return "beryl";
        case "blackbox"_fnv1a16:      return "blackbox";
        case "bspwm"_fnv1a16:         return "bspwm";
        case "budgie-wm"_fnv1a16:     return "budgie-wm";
        case "chromeos-wm"_fnv1a16:   return "chromeos-wm";
        case "cinnamon"_fnv1a16:      return "Muffin";
        case "compiz"_fnv1a16:        return "compiz";
        case "deepin-wm"_fnv1a16:     return "deepin-wm";
        case "dminiwm"_fnv1a16:       return "dminiwm";
        case "dtwm"_fnv1a16:          return "dtwm";
        case "dwm"_fnv1a16:           return "dwm";
        case "e16"_fnv1a16:           return "e16";
        case "echinus"_fnv1a16:       return "echinus";
        case "emerald"_fnv1a16:       return "emerald";
        case "enlightenment"_fnv1a16: return "enlightenment";
        case "finder"_fnv1a16:        return "finder";
        case "fluxbox"_fnv1a16:       return "fluxbox";
        case "flwm"_fnv1a16:          return "flwm";
        case "flwm_topside"_fnv1a16:  return "flwm_topside";
        case "fvwm"_fnv1a16:          return "fvwm";

        case "gnome-shell"_fnv1a16:
        case "gnome-session-binary"_fnv1a16:
        case "mutter"_fnv1a16:        return "Mutter";

        case "herbstluftwm"_fnv1a16: return "herbstluftwm";
        case "howm"_fnv1a16:         return "howm";
        case "hyprland"_fnv1a16:     return "Hyprland";
        case "i3"_fnv1a16:           return "i3";
        case "i3wm"_fnv1a16:         return "i3wm";
        case "icewm"_fnv1a16:        return "icewm";
        case "kwin"_fnv1a16:         return "Kwin";
        case "marco"_fnv1a16:        return "Marco";
        case "metacity"_fnv1a16:     return "Metacity";
        case "monsterwm"_fnv1a16:    return "monsterwm";
        case "muffin"_fnv1a16:       return "Muffin";
        case "musca"_fnv1a16:        return "musca";
        case "mwm"_fnv1a16:          return "mwm";
        case "notion"_fnv1a16:       return "notion";
        case "openbox"_fnv1a16:      return "Openbox";
        case "pekwm"_fnv1a16:        return "pekwm";
        case "qtile"_fnv1a16:        return "Qtile";
        case "ratpoison"_fnv1a16:    return "ratpoison";
        case "sawfish"_fnv1a16:      return "sawfish";
        case "scrotwm"_fnv1a16:      return "scrotwm";
        case "spectrwm"_fnv1a16:     return "spectrwm";
        case "stumpwm"_fnv1a16:      return "stumpwm";
        case "subtle"_fnv1a16:       return "subtle";
        case "sway"_fnv1a16:         return "sway";
        case "swm"_fnv1a16:          return "swm";
        case "tinywm"_fnv1a16:       return "tinywm";
        case "twin"_fnv1a16:         return "twin";
        case "wayfire"_fnv1a16:      return "wayfire";
        case "weston"_fnv1a16:       return "weston";
        case "wmaker"_fnv1a16:       return "wmaker";
        case "wmfs"_fnv1a16:         return "wmfs";
        case "wmii"_fnv1a16:         return "wmii";
        case "xfwm4"_fnv1a16:        return "Xfwm4";
        case "xmonad"_fnv1a16:       return "XMonad";
    }

    return MAGIC_LINE;
}

std::string get_mate_version()
{
    constexpr std::string_view path = "/usr/share/mate-about/mate-version.xml";
    std::ifstream              f(path.data(), std::ios::in);
    if (!f.is_open())
    {
        std::string ret;
        read_exec({ "mate-session", "--version" }, ret);

        // erase doesn't remove the nth character, only the ones before it, so we have to add 1.
        ret.erase(0, ret.rfind(' ') + 1);
        return ret;
    }

    std::string buffer((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    buffer.push_back('\0');

    rapidxml::xml_document<> doc;
    doc.parse<0>(&buffer[0]);
    rapidxml::xml_node<>* root_node = doc.first_node("mate-version");
    if (!root_node)
    {
        error("Root node <mate-version> not found");
        return UNKNOWN;
    }

    const std::string_view major = root_node->first_node("platform")->value();
    const std::string_view minor = root_node->first_node("minor")->value();
    const std::string_view micro = root_node->first_node("micro")->value();

    return fmt::format("{}.{}.{}", major, minor, micro);
}

std::string get_kwin_version()
{
    std::string ret;

    if (std::getenv("WAYLAND_DISPLAY") != NULL)
        read_exec({ "kwin_wayland", "--version" }, ret);
    else
        read_exec({ "kwin_x11", "--version" }, ret);

    // erase doesn't remove the nth character, only the ones before it, so we have to add 1.
    ret.erase(0, ret.rfind(' ') + 1);
    return ret;
}

static std::string get_cinnamon_version_binary()
{
    std::ifstream f(which("cinnamon"), std::ios::binary);
    if (!f.is_open())
        return UNKNOWN;

    std::string line, ret;
    while (read_binary_file(f, line))
    {
        // if you run `strings $(which cinnamon)`
        // and then analyze every string, you'll see there is a string with
        // "Cinnamon %s" and above it's version
        // so let's do it
        if (line == "Cinnamon %s")
            return ret;

        // save the above position
        ret = line;
    }

    return UNKNOWN;
}

std::string get_cinnamon_version()
{
    const char* env = std::getenv("CINNAMON_VERSION");
    if (env != nullptr && env[0] != '\0')
        return env;

    std::string   line;
    std::ifstream f("/usr/share/applications/cinnamon.desktop", std::ios::in);
    if (!f.is_open())
    {
        std::string ret = get_cinnamon_version_binary();
        if (ret != UNKNOWN)
            return ret;

        ret.clear();
        read_exec({ "cinnamon", "--version" }, ret);
        ret.erase(0, "Cinnamon "_len);
        return ret;
    }

    while (std::getline(f, line))
    {
        if (hasStart(line, "X-GNOME-Bugzilla-Version="))
            return line.substr("X-GNOME-Bugzilla-Version="_len);
    }

    return "";
}

static std::string get_xfce4_version_lib()
{
    LOAD_LIBRARY("libxfce4util.so", return UNKNOWN)
    LOAD_LIB_SYMBOL(const char*, xfce_version_string, void)
    const std::string& ret = xfce_version_string();
    UNLOAD_LIBRARY()
    return ret;
}

std::string get_xfce4_version()
{
    std::string ret = get_xfce4_version_lib();
    if (ret != UNKNOWN)
        return ret;

    read_exec({ "xfce4-session", "--version" }, ret);
    ret.erase(0, "xfce4-session"_len + 1);
    ret.erase(ret.find(' '));
    return ret;
}

bool get_fast_xfwm4_version(std::string& ret, const std::string& exec_path)
{
    std::ifstream f(exec_path, std::ios::binary);
    if (!f.is_open())
        return false;

    std::string line;
    while (read_binary_file(f, line))
    {
        if (line == "using GTK+-%d.%d.%d.")
            return true;

        // previous line, which will eventually be the version
        ret = line;
    }

    return false;
}
