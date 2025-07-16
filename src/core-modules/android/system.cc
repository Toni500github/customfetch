#include "platform.hpp"

#if CF_ANDROID
#include <string>
#include <string_view>

#include "cufetch/common.hh"
#include "core-modules.hh"
#include "util.hpp"

static constexpr std::array<std::string_view, 9> vendors_prop_names = {
        "ro.product.marketname",    "ro.vendor.product.display", "ro.vivo.market.name", "ro.config.devicename",
        "ro.config.marketing_name", "ro.product.vendor.model", "ro.product.oppo_model", "ro.oppo.market.name",  "ro.product.brand"
    };

MODFUNC(host)
{
    return host_vendor(NULL) + " " + host_name(NULL) + " " + host_version(NULL);
}

MODFUNC(host_name)
{
    for (const std::string_view name : vendors_prop_names)
    {
        const std::string& model_name = get_android_property(name);
        if (!model_name.empty())
            return model_name;
    }

    return UNKNOWN;
}

MODFUNC(host_version)
{
    return get_android_property("ro.product.model");
}

MODFUNC(host_vendor)
{
    return get_android_property("ro.product.manufacturer");
}

MODFUNC(arch)
{ return g_uname_infos.machine; }

#endif
