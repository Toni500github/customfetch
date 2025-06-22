#include "util.hpp"

bool download_git(const std::string_view url)
{
    const std::string_view repo = url.substr(url.rfind('/')+1);
    return taur_exec({ "git", "clone", url.data(), (getConfigDir() / "mods" / repo).string() });
}
