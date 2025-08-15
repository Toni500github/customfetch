#include "libcufetch/libcufetch_c.h"

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

#include "libcufetch/config.hh"
#include "libcufetch/cufetch.hh"
#include "libcufetch/parse.hh"

static std::atomic<bool> g_plugins_active{ true };

// --- Utilities ---------------------------------------------------------------
static char* dup_cstr(const char* s)
{
    if (!s)
        return nullptr;
    size_t n   = std::strlen(s) + 1;
    char*  out = static_cast<char*>(std::malloc(n));
    if (!out)
        return nullptr;
    std::memcpy(out, s, n);
    return out;
}

static char* dup_cpp_string(const std::string& s)
{
    size_t n   = s.size() + 1;
    char*  out = static_cast<char*>(std::malloc(n));
    if (!out)
        return nullptr;
    std::memcpy(out, s.c_str(), n);
    return out;
}

// Convert C++ moduleArgs_t linked list to C structure
cf_module_args_t* convert_module_args(const moduleArgs_t* head_cpp)
{
    if (!head_cpp)
        return nullptr;

    // Count nodes
    size_t count = 0;
    for (auto p = head_cpp; p; p = p->next)
        ++count;

    // Allocate array (+1 sentinel)
    cf_module_args_t* head = static_cast<cf_module_args_t*>(std::calloc(count + 1, sizeof(cf_module_args_t)));
    if (!head)
        return nullptr;

    // Fill
    size_t i = 0;
    for (auto p = head_cpp; p; p = p->next, ++i)
    {
        head[i].name  = dup_cpp_string(p->name);
        head[i].value = dup_cpp_string(p->value);
        // If dup fails, best effort: leave nulls; caller should handle.
    }

    // Last element is sentinel {nullptr, nullptr}
    return head;
}

extern "C" {

// Configuration access (during plugin init)
bool cf_config_get_bool(cf_config_t* config, const char* path, bool fallback)
{
    if (!config || !path) return fallback;
    const ConfigBase* cpp = reinterpret_cast<const ConfigBase*>(config);
    return cpp->getValue<bool>(path, fallback);
}

int cf_config_get_int(cf_config_t* config, const char* path, int fallback)
{
    if (!config || !path) return fallback;
    const ConfigBase* cpp = reinterpret_cast<const ConfigBase*>(config);
    return cpp->getValue<int>(path, fallback);
}

char* cf_config_get_string(cf_config_t* config, const char* path, const char* fallback)
{
    if (!config || !path)
        return fallback ? dup_cstr(fallback) : nullptr;
    const ConfigBase* cpp = reinterpret_cast<const ConfigBase*>(config);
    std::string       v   = cpp->getValue<std::string>(path, fallback ? std::string(fallback) : std::string{});
    return dup_cpp_string(v);
}

char** cf_config_get_string_array(cf_config_t* config, const char* path, int* count)
{
    if (!config || !path || !count)
    {
        if (count)
            *count = 0;
        return nullptr;
    }

    const ConfigBase*        cpp_config = reinterpret_cast<const ConfigBase*>(config);
    std::vector<std::string> fallback;
    std::vector<std::string> result = cpp_config->getValueArrayStr(path, fallback);

    *count = result.size();
    if (*count == 0)
        return nullptr;

    char** c_array = static_cast<char**>(malloc(*count * sizeof(char*)));
    if (!c_array)
    {
        *count = 0;
        return nullptr;
    }

    for (int i = 0; i < *count; ++i)
    {
        c_array[i] = static_cast<char*>(malloc(result[i].length() + 1));
        if (!c_array[i])
        {
            for (int j = 0; j < i; ++j)
                free(c_array[j]);
            free(c_array);
            *count = 0;
            return nullptr;
        }
        strcpy(c_array[i], result[i].c_str());
    }

    return c_array;
}

// Parse context access (from callback_info)
char* cf_get_config_from_callback(const cf_callback_info_t* cb, const char* path, const char* fallback)
{
    if (!cb || !path)
    {
        return fallback ? dup_cstr(fallback) : nullptr;
    }
    const auto* cpp = reinterpret_cast<const callbackInfo_t*>(cb);
    std::string val =
        cpp->parse_args.config.getValue<std::string>(path, fallback ? std::string(fallback) : std::string{});
    return dup_cpp_string(val);
}

char* cf_get_colored_percentage(const cf_callback_info_t* info, float n1, float n2, bool invert)
{
    if (!info || !info->parse_args_context)
        return nullptr;

    parse_args_t* parse_args = reinterpret_cast<parse_args_t*>(info->parse_args_context);
    std::string   result     = get_and_color_percentage(n1, n2, *parse_args, invert);

    char* c_result = static_cast<char*>(malloc(result.length() + 1));
    if (c_result)
        strcpy(c_result, result.c_str());
    return c_result;
}

// Module arguments traversal
const cf_module_args_t* cf_get_first_arg(const cf_callback_info_t* info)
{
    if (!info || !info->module_args)
        return nullptr;

    const cf_module_args_t* current = info->module_args;
    while (current->prev)
        current = current->prev;
    return current;
}

const cf_module_args_t* cf_get_last_arg(const cf_callback_info_t* info)
{
    if (!info || !info->module_args)
        return nullptr;

    const cf_module_args_t* current = info->module_args;
    while (current->next)
        current = current->next;
    return current;
}

const cf_module_args_t* cf_find_arg_by_name(const cf_callback_info_t* info, const char* name)
{
    if (!info || !info->module_args || !name)
        return nullptr;

    const cf_module_args_t* current = cf_get_first_arg(info);
    while (current)
    {
        if (strcmp(current->name, name) == 0)
            return current;
        current = current->next;
    }
    return nullptr;
}

struct cf_module_t
{
    std::string                               name;
    std::string                               description;
    cf_module_handler_t                       handler;
    std::vector<std::unique_ptr<cf_module_t>> submodules;

    // Convert to C++ module_t
    module_t to_cpp_module() const
    {
        module_t cpp_module;
        cpp_module.name        = name;
        cpp_module.description = description;

        // Convert submodules recursively
        for (const auto& sub : submodules)
        {
            cpp_module.submodules.push_back(sub->to_cpp_module());
        }

        // Set handler with safe atomic check
        if (handler)
        {
            cpp_module.handler = [handler_copy = handler](const callbackInfo_t* info) -> std::string {
                // Return empty string if plugins are being unloaded
                if (!g_plugins_active.load())
                    return std::string();

                if (!info)
                    return std::string();

                // Convert C++ callbackInfo_t to C structure
                cf_callback_info_t c_info;
                c_info.module_args        = convert_module_args(info->moduleArgs);
                c_info.parse_args_context = const_cast<parse_args_t*>(&info->parse_args);

                // Call C handler
                const char* result = handler_copy(&c_info);

                return result ? std::string(result) : std::string();
            };
        }

        return cpp_module;
    }
};

cf_module_t* cf_create_module(const char* name, const char* description, cf_module_handler_t handler)
{
    if (!name || !description)
        return nullptr;

    cf_module_t* module = new (std::nothrow) cf_module_t();
    if (!module)
        return nullptr;

    module->name        = name;
    module->description = description;
    module->handler     = handler;

    return module;
}

bool cf_add_submodule(cf_module_t* parent, cf_module_t* child)
{
    if (!parent || !child)
        return false;

    parent->submodules.push_back(std::unique_ptr<cf_module_t>(child));
    return true;
}

bool cf_register_module_tree(cf_module_t* root)
{
    if (!root)
        return false;

    // Convert to C++ module and register
    module_t cpp_module = root->to_cpp_module();
    cfRegisterModule(std::move(cpp_module));

    // Clean up the C wrapper structure after registration
    delete root;
    return true;
}

// Memory management
void cf_free_string(char* str)
{
    if (str)
        free(str);
}

void cf_free_string_array(char** array, int count)
{
    if (!array)
        return;

    for (int i = 0; i < count; ++i)
        if (array[i])
            free(array[i]);
    free(array);
}

void cf_finish_all() { g_plugins_active.store(false, std::memory_order_release); }

}  // extern "C"
