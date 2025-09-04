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
    const moduleArgs_t* moduleArgs;
    parse_args_t&       parse_args;
};

struct module_t
{
    std::string           name;
    std::string           description;
    std::vector<module_t> submodules; /* For best performance, use std::move() when adding modules in here. */
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
