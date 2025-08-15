#include "libcufetch/libcufetch_c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Static storage for plugin data
static char* username = NULL;

const char* github_user_name_handler(const cf_callback_info_t* cb)
{
    // 1) Get arg (host allocates a tiny array + string copies) → must free with cf_free_module_args
    const cf_module_args_t* found = cf_find_arg_by_name(cb, "github-user");

    // Choose the username (copy if you want to keep it)
    const char* user = (found && found[0].value) ? found[0].value : "unknown";

    // Optional: keep a copy in plugin-owned memory
    free(username); // free previous if you had one
    username = NULL;
    if (user && user != (const char*)"unknown") {
        username = strdup(user); // your memory, free with free() in finish()
    }

    static char result[256];

    snprintf(result, sizeof(result), "%s", user);

    return result; // valid until next call on this thread
}

const char* github_user_followers_handler(const cf_callback_info_t* callback_info) {
    // Example of using colored percentage
    char* percentage = cf_get_colored_percentage(callback_info, 150, 1000, false);
    
    static char result[256];
    snprintf(result, sizeof(result), "150 followers %s", percentage ? percentage : "");
    
    cf_free_string(percentage);
    return result;
}

const char* github_user_bio_handler(const cf_callback_info_t* callback_info) {
    return "C plugin developer";
}

#ifdef __cplusplus
extern "C" {
#endif

// Plugin lifecycle - MUST match exact signature: void start(void*, const ConfigBase&)
void start(void* handle, const ConfigBase& config) {
    // Get username from config
    username = cf_config_get_string((cf_config_t*)&config, "plugin.github-user-fetch.username", "unknown");
    
    if (!username || strcmp(username, "unknown") == 0) {
        printf("Warning: Username not set in config\n");
        return;
    }
    
    // Create submodules (equivalent to your C++ example)
    cf_module_t* name_module = cf_create_module("name", "profile username", github_user_name_handler);
    cf_module_t* followers_module = cf_create_module("followers", "profile followers", github_user_followers_handler);
    cf_module_t* bio_module = cf_create_module("bio", "profile bio", github_user_bio_handler);
    
    // Create parent module
    cf_module_t* github_user_module = cf_create_module("github-user", "Github user modules", NULL);
    
    // Build hierarchy (equivalent to std::move in C++)
    if (!cf_add_submodule(github_user_module, name_module) ||
        !cf_add_submodule(github_user_module, followers_module) ||
        !cf_add_submodule(github_user_module, bio_module)) {
        printf("Error: Failed to add submodules\n");
        return;
    }
    
    // Register the complete module tree
    if (!cf_register_module_tree(github_user_module)) {
        printf("Error: Failed to register module tree\n");
        return;
    }
    
    printf("GitHub user plugin loaded successfully\n");
}

// Plugin cleanup - MUST match exact signature: void finish(void*)
void finish(void* handle) {
    cf_free_string(username);
    username = NULL;
    cf_finish_all();
    printf("GitHub user plugin unloaded\n");
}

#ifdef __cplusplus
}
#endif
