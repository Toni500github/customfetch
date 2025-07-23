#include "platform.hpp"
#if CF_MACOS

#include <sys/sysctl.h>

#include <fstream>
#include <string>

#include "core-modules.hh"
#include "libcufetch/common.hh"
#include "rapidxml-1.13/rapidxml.hpp"
#include "switch_fnv1a.hpp"
#include "util.hpp"

std::ifstream            f("/System/Library/CoreServices/SystemVersion.plist", std::ios::in);
std::string              buffer(std::istreambuf_iterator<char>{ f }, std::istreambuf_iterator<char>{});
rapidxml::xml_document<> doc;

static std::string get_codename(const std::string_view os_version_id)
{
    std::string major;
    std::string minor{ UNKNOWN };
    size_t      pos1 = os_version_id.find('.');
    if (pos1 != os_version_id.npos)
    {
        major       = os_version_id.substr(0, pos1);
        size_t pos2 = os_version_id.find('.', pos1 + 1);
        if (pos2 != os_version_id.npos)
            minor = os_version_id.substr(pos1 + 1, pos2);
    }

    switch (fnv1a16::hash(major))
    {
        case "15"_fnv1a16: return "Sequoia";
        case "14"_fnv1a16: return "Sonoma";
        case "13"_fnv1a16: return "Ventura";
        case "12"_fnv1a16: return "Monterey";
        case "11"_fnv1a16: return "Big Sur";
        case "10"_fnv1a16:
        {
            switch (fnv1a16::hash(minor))
            {
                case "16"_fnv1a16: return "Big Sur";
                case "15"_fnv1a16: return "Catalina";
                case "14"_fnv1a16: return "Mojave";
                case "13"_fnv1a16: return "High Sierra";
                case "12"_fnv1a16: return "Sierra";
                case "11"_fnv1a16: return "El Capitan";
                case "10"_fnv1a16: return "Yosemite";
                case "9"_fnv1a16:  return "Mavericks";
                case "8"_fnv1a16:  return "Mountain Lion";
                case "7"_fnv1a16:  return "Lion";
                case "6"_fnv1a16:  return "Snow Leopard";
                case "5"_fnv1a16:  return "Leopard";
                case "4"_fnv1a16:  return "Tiger";
                case "3"_fnv1a16:  return "Panther";
                case "2"_fnv1a16:  return "Jaguar";
                case "1"_fnv1a16:  return "Puma";
                case "0"_fnv1a16:  return "Cheetah";
            }
        }
    }

    return UNKNOWN;
}

static void assert_doc()
{
    if (!doc.first_node("plist"))
    {
        buffer.push_back('\0');
        doc.parse<0>(&buffer[0]);
    }
}

static std::string get_plist_value(const std::string_view name)
{
    assert_doc();
    rapidxml::xml_node<>* root_node = doc.first_node("plist")->first_node("dict")->first_node("key");
    for (; root_node; root_node = root_node->next_sibling())
    {
        const std::string_view key   = root_node->value();  // <key>ProductName</key>
        root_node                    = root_node->next_sibling();
        const std::string_view value = root_node->value();  // <string>macOS</string>
        if (key == name)
            return value.data();
    }
    return UNKNOWN;
}

MODFUNC(os_pretty_name)
{
    const std::string& codename = os_version_codename(nullptr);
    if (codename != UNKNOWN)
        return os_name(nullptr) + " " + os_version_id(nullptr) + " (" + codename + ")";
    return os_name(nullptr) + " " + os_version_id(nullptr);
}

unsigned long os_uptime()
{
    struct timeval boot_time;
    size_t         size   = sizeof(boot_time);
    int            name[] = { CTL_KERN, KERN_BOOTTIME };
    if (sysctl(name, 2, &boot_time, &size, NULL, 0) != 0)
        die(_("failed to get uptime"));

    return time(NULL) - boot_time.tv_sec;
}

// clang-format off
MODFUNC(os_name)
{ return get_plist_value("ProductName"); }

MODFUNC(os_name_id)
{ return "macos"; }

MODFUNC(os_version_id)
{ return get_plist_value("ProductUserVisibleVersion"); }

MODFUNC(os_version_codename)
{ return get_codename(os_version_id(nullptr)); }

MODFUNC(os_kernel_name)
{ return g_uname_infos.sysname; }

MODFUNC(os_kernel_version)
{ return g_uname_infos.release; }

MODFUNC(os_hostname)
{ return g_uname_infos.nodename; }

MODFUNC(os_initsys_name)
{ return MAGIC_LINE; }

MODFUNC(os_initsys_version)
{ return UNKNOWN; }

#endif
