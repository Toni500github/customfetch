#include "dewm.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>

#include "rapidxml-1.13/rapidxml.hpp"
#include "util.hpp"

// https://github.com/fastfetch-cli/fastfetch/blob/a61765c8b1387777be67d967bc2f69031c8ca399/src/detection/displayserver/linux/wmde.c#L19
std::string parse_de_env(void) noexcept
{
    // maybe let's try to get the DE envs first, then try with the others
    if (std::getenv("KDE_FULL_SESSION") != NULL || std::getenv("KDE_SESSION_UID") != NULL ||
        std::getenv("KDE_SESSION_VERSION") != NULL)
        return "KDE";

    if (std::getenv("GNOME_DESKTOP_SESSION_ID") != NULL || std::getenv("GNOME_SETUP_DISPLAY") != NULL)
        return "GNOME";

    if (std::getenv("MATE_DESKTOP_SESSION_ID") != NULL)
        return "Mate";

    if (std::getenv("TDE_FULL_SESSION") != NULL)
        return "Trinity";

    if (std::getenv("HYPRLAND_CMD") != NULL)
        return "Hyprland";

    const char* env;

    env = std::getenv("XDG_CURRENT_DESKTOP");
    if (env != NULL && env[0] != '\0')
        return env;

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
    if (name.find("kwin") != std::string::npos)
        return "Kwin";

    if (name.find("gnome-shell") != std::string::npos || name.find("Mutter") != std::string::npos ||
        name.find("gnome shell") != std::string::npos)
        return "Mutter";

    if (name.find("cinnamon") != std::string::npos || name.find("Muffin") != std::string::npos)
        return "Muffin";

    if (name.find("Marco") != std::string::npos)
        return "Marco";

    // taken and ordered (for binary searching) from this list 
    // https://github.com/KittyKatt/screenFetch/blob/master/screenfetch-dev#L93
    // added some missing.
    // ngl this looks beatiful thanks to clang-format :D
    constexpr std::array<std::string_view, 55> wms = {
        "2bwm",     "9wm",           "awesome",   "beryl",   "blackbox", "bspwm",        "budgie-wm", "chromeos-wm",
        "cinnamon", "compiz",        "deepin-wm", "dminiwm", "dtwm",     "dwm",          "e16",       "echinus",
        "emerald",  "enlightenment", "finder",    "fluxbox", "flwm",     "flwm_topside", "fvwm",      "herbstluftwm",
        "howm",     "hyprland",      "i3",        "i3wm",    "icewm",    "kwin",         "metacity",  "monsterwm",
        "musca",    "mwm",           "notion",    "openbox", "pekwm",    "qtile",        "ratpoison", "sawfish",
        "scrotwm",  "spectrwm",      "stumpwm",   "subtle",  "sway",     "swm",          "tinywm",    "twin",
        "wayfire",  "weston",        "wmaker",    "wmfs",    "wmii",     "xfwm4",        "xmonad"
    };
    if (std::binary_search(wms.begin(), wms.end(), str_tolower(name.data())))
        return name.data();

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
        ret.erase(0, ret.rfind(' '));
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

std::string get_cinnamon_version()
{
    const char* env = std::getenv("CINNAMON_VERSION");
    if (env != nullptr && env[0] != '\0')
        return env;

    std::string   ret, line;
    std::ifstream f("/usr/share/applications/cinnamon.desktop", std::ios::in);
    if (!f.is_open())
    {
        read_exec({ "cinnamon", "--version" }, ret);
        ret.erase(0, "Cinnamon "_len);
        return ret;
    }

    while (std::getline(f, line))
    {
        if (hasStart(line, "X-GNOME-Bugzilla-Version"))
        {
            ret = line.substr(0, "X-GNOME-Bugzilla-Version"_len);
            return ret;
        }
    }
}

static std::string get_xfce4_version_lib()
{
    LOAD_LIBRARY("libxfce4util.so", return UNKNOWN)
    LOAD_LIB_SYMBOL(const char *, xfce_version_string, void)
    std::string ret = xfce_version_string();
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

