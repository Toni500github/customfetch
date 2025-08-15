#ifndef _LIBCUFETCH_C_H_
#define _LIBCUFETCH_C_H_

#include <stdbool.h>

#ifdef __cplusplus
class ConfigBase;
typedef ConfigBase cf_config_t;
extern "C" {
#else
typedef struct cf_config_t cf_config_t;
#endif

typedef struct cf_module_args_t   cf_module_args_t;
typedef struct cf_callback_info_t cf_callback_info_t;
typedef struct cf_module_t        cf_module_t;

struct cf_module_args_t
{
    struct cf_module_args_t* prev;
    char*                    name;
    char*                    value;
    struct cf_module_args_t* next;
};

struct cf_callback_info_t
{
    const struct cf_module_args_t* module_args;
    void*                          parse_args_context;  // Opaque pointer to parse_args_t
};

// Module handler function
typedef const char* (*cf_module_handler_t)(const cf_callback_info_t* callback_info);

// Configuration access (during plugin init only)
char*  cf_config_get_string(cf_config_t* config, const char* path, const char* fallback);
bool   cf_config_get_bool(cf_config_t* config, const char* path, bool fallback);
int    cf_config_get_int(cf_config_t* config, const char* path, int fallback);
char** cf_config_get_string_array(cf_config_t* config, const char* path, int* count);

// Parse context access (from callback_info)
char* cf_get_config_from_callback(const cf_callback_info_t* info, const char* path, const char* fallback);
char* cf_get_colored_percentage(const cf_callback_info_t* info, float n1, float n2, bool invert);

// Module arguments traversal
const cf_module_args_t* cf_get_first_arg(const cf_callback_info_t* info);
const cf_module_args_t* cf_get_last_arg(const cf_callback_info_t* info);
const cf_module_args_t* cf_find_arg_by_name(const cf_callback_info_t* info, const char* name);

// Simple module registration - just register directly, no complex management
bool cf_register_module(const char* name, const char* description, cf_module_handler_t handler);
bool cf_register_submodule(const char* parent_name, const char* name, const char* description,
                           cf_module_handler_t handler);

cf_module_t* cf_create_module(const char* name, const char* description, cf_module_handler_t handler);
bool         cf_add_submodule(cf_module_t* parent, cf_module_t* child);
bool         cf_register_module_tree(cf_module_t* root);

// Memory management
void cf_free_string(char* str);
void cf_free_string_array(char** array, int count);
void cf_finish_all(void);

#ifdef __cplusplus
}
#endif

#endif
