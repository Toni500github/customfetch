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

#pragma once

#include <functional>
#include <string>
#include <vector>

#include "libcufetch/parse.hh"

/* A linked list including module arguments. An argument may be specified for any part of the module path (e.g.
 * `disk(/).used(GiB)`, `test.hi(a)`) */
struct moduleArgs_t
{
    struct moduleArgs_t* prev = nullptr;

    std::string name;
    std::string value;

    struct moduleArgs_t* next = nullptr;
};

// Struct used in modules callback functions (handler in module_t)
struct callbackInfo_t
{
    const moduleArgs_t* module_args;
    parse_args_t&       parse_args;
};

/* Main struct for declaring a customfetch module.
 *
 * Submodules are referenced with '.' in their path.  
 * Example: $<parent.child> -> parent = `name`, child = `submodules[x].name`.
 *
 * WARN: Do not pass submodules to cfRegisterModule.  
 * It registers recursively and will include them automatically.
 *
 * Real example: $<github.profile.following>  
 *   - github = root module  
 *   - profile = submodule of github  
 *   - following = submodule of profile
 *
 * The handler is executed when the module is invoked in the layout.
 * If it's NULL, it returns "(unknown/invalid module)"
 *
 * Code example:
 * module_t submodule_foo = {"idk", "description", {}, submodule_foo_callback};
 * module_t foo = {"foo", "description", {std::move(submodule_foo)}, foo_callback};
 * cfRegisterModule(foo); // you can call $<foo> and $<foo.idk> from the layout.
 */
struct module_t
{
    std::string           name;
    std::string           description;
    std::vector<module_t> submodules; /* Use std::move() for efficiency when adding. */
    std::function<std::string(const callbackInfo_t*)> handler;
};

// C ABI is needed to prevent symbol mangling, but we don't actually need C compatibility,
// so we ignore this warning about return types that are potentially incompatible with C.
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
#endif

/* Register a module, and its submodules, to customfetch. */
APICALL EXPORT void cfRegisterModule(const module_t& module);

/* Get a list of all modules registered. */
APICALL EXPORT const std::vector<module_t>& cfGetModules();
