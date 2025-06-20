/* This is an example for a mod you could install in .config/mods

Mods are essentially custom(fetch) modules that you can implement yourself and register using libcufetch!
They are to be compiled as shared libraries **with no name mangling!!**, with one start function. Scroll down for more details on the start function.

To compile this, just run `g++ -I../include -shared -fPIC mod-library.cc -o mod-library.so`. To use it, you'll need to put it in your customfetch/mods config directory.
*/

#include <dlfcn.h>
#include <functional>
#include <iostream>
#include <stdio.h>
#include <string>

#include "common.hpp"

/* ret_type = type of what the function returns
 * func     = the function name
 * ...      = the arguments in a function if any
 */
#define LOAD_LIB_SYMBOL(ret_type, func, ...)   \
    typedef ret_type (*func##_t)(__VA_ARGS__); \
    func##_t func = reinterpret_cast<func##_t>(dlsym(handle, #func));

#define UNLOAD_LIBRARY() dlclose(handle);

/* The handler that we'll use for our module, Handlers return const std::string (WILL be changed to const char pointers). */
const std::string test() {
    return "Hello!";
}

/* Start function.

When writing in C++, you MUST put the extern "C" part in the beginning to avoid name mangling.
customfetch will look for a "start" function, but if your compiler mangles the names it will not be able to find it.

The start function takes in a handle, This handle is going to be the libcufetch library.
You'll need to use dlsym on this handle to get the functions necessary for whatever you're doing. (e.g. cfRegisterModule)

*/
extern "C" void start(void *handle) {
    if (!handle) {
        std::cout << "Exiting because !handle" << std::endl;
        return;
    }

    /* Load cfRegisterModule, you don't need this macro but it's nice! */
    LOAD_LIB_SYMBOL(void, cfRegisterModule, const module_t &module);

    /* Our goal in this example is to create a `modification.test` module, This will just return "Hello!" and nothing else. */
    /* The way we'll do this is we will create the test module, with no submodules and a handler (that just returns "Hello!"). */
    /* But then we will also create the modification module, which won't do anything other than hold the test module as a submodule. */
    /* We will then register the modification module. We won't register the test module because it is already a submodule and will be implicitly added. */

    /* Here we create the 'test' submodule. in customfetch there's no practical difference between a parent module and a submodule. So we just define it like any other module. */
    /* We will not register this, it will be implicitly added through its parent module (so we can't directly invoke `test`, we can only invoke `modification.test`) */
    module_t test_module = {"test", {}, test};

    /* And here we create the 'modification' module. This is what we're actually going to register and it will include the test module as a submodule. */
    /* This module doesn't have a handler, so it can't be used in the config (`modification` won't work). We'll instead use `modification.test` in the config (which does have a handler). */
    module_t modification_module = { "modification", { std::move(test_module) }, NULL };

    /* Register the module. */
    /* This will take the modification module, recursively add it and its submodules to the list, and continue until its finished everything. */
    cfRegisterModule(modification_module);

    /* And done, after this customfetch will call our handler whenever our test module is invoked. */
}
