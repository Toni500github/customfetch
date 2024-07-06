#include <algorithm>
#include "util.hpp"
#include "query.hpp"

// https://github.com/fastfetch-cli/fastfetch/blob/a61765c8b1387777be67d967bc2f69031c8ca399/src/detection/displayserver/linux/wmde.c#L19
std::string parse_wm_env(void)
{
    const char* env;

    env = std::getenv("XDG_CURRENT_DESKTOP");
    if(env != NULL && env[0] != '\0')
        return env;

    env = std::getenv("XDG_SESSION_DESKTOP");
    if(env != NULL && env[0] != '\0')
        return env;

    env = std::getenv("CURRENT_DESKTOP");
    if(env != NULL && env[0] != '\0')
        return env;

    env = std::getenv("SESSION_DESKTOP");
    if(env != NULL && env[0] != '\0')
        return env;

    env = std::getenv("DESKTOP_SESSION");
    if(env != NULL && env[0] != '\0')
        return env;

    if(std::getenv("KDE_FULL_SESSION") != NULL || std::getenv("KDE_SESSION_UID") != NULL || std::getenv("KDE_SESSION_VERSION") != NULL)
        return "KDE";

    if(std::getenv("GNOME_DESKTOP_SESSION_ID") != NULL)
        return "GNOME";

    if(std::getenv("MATE_DESKTOP_SESSION_ID") != NULL)
        return "Mate";

    if(std::getenv("TDE_FULL_SESSION") != NULL)
        return "Trinity";

    if(std::getenv("HYPRLAND_CMD") != NULL)
        return "Hyprland";

    return UNKNOWN;
}

std::string prettify_wm_name(const std::string_view name) {
    if (name.find("kwin") != std::string::npos)
        return "Kwin";

    if (name.find("gnome-shell") != std::string::npos || 
        name.find("Mutter")      != std::string::npos ||
        name.find("gnome shell") != std::string::npos)
        return "Mutter";

    if (name.find("cinnamon") != std::string::npos ||
        name.find("Muffin") != std::string::npos)
        return "Muffin";

    if (name.find("Marco") != std::string::npos)
        return "Marco";

    std::array<std::string, 30> wms = {"dwm", "awesome", "i3", "i3wm", "xfwm4", "qtile", "bspwm", "herbstluftwm", "tinywm", "dtwm", "hyprland", "sway", "weston", "wayfire", "openbox", "xmonad", "icewm"};
    if (std::find(wms.begin(), wms.end(), str_tolower(name)) != wms.end())
        return name.data();

    return MAGIC_LINE;
}
