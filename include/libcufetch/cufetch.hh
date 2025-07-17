#pragma once

#include <functional>
#include <string>
#include <vector>

#include "libcufetch/parse.hh"

// Map from a modules name to its pointer.
using moduleMap_t = std::unordered_map<std::string, const module_t&>;

/* A linked list including module arguments. An argument may be specified for any part of the module path (e.g.
 * `disk(/).used(GiB)`, `test(a).hi`) */
struct moduleArgs_t
{
    struct moduleArgs_t* prev = nullptr;

    std::string name;
    std::string value;

    struct moduleArgs_t* next = nullptr;
};

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

/* Register a module, and its submodules, to customfetch. */
APICALL EXPORT void cfRegisterModule(const module_t& module);

/* Get a list of all modules registered. */
APICALL EXPORT const std::vector<module_t>& cfGetModules();
