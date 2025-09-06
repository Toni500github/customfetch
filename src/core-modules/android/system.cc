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

#include "platform.hpp"

#if CF_ANDROID
#include <string>
#include <string_view>

#include "core-modules.hh"
#include "libcufetch/common.hh"
#include "util.hpp"

static constexpr std::array<std::string_view, 9> vendors_prop_names = {
    "ro.product.marketname", "ro.vendor.product.display", "ro.vivo.market.name",
    "ro.config.devicename",  "ro.config.marketing_name",  "ro.product.vendor.model",
    "ro.product.oppo_model", "ro.oppo.market.name",       "ro.product.brand"
};

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

// clang-format off
MODFUNC(host_version)
{ return get_android_property("ro.product.model"); }

MODFUNC(host_vendor)
{ return get_android_property("ro.product.manufacturer"); }

MODFUNC(host)
{ return host_vendor(NULL) + " " + host_name(NULL) + " " + host_version(NULL); }

MODFUNC(arch)
{ return g_uname_infos.machine; }

#endif
