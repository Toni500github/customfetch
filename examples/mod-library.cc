/* This is an example for a plugin you could install in ~/.config/customfetch/plugins/

Plugins are essentially custom(fetch) libraries where you can create modules that you can implement yourself and register using libcufetch!
They have to be compiled as shared libraries **with no name mangling!!**, with one start() and finish() functions. Scroll down for more details on both functions.

To compile this, just run `g++ -I../include -shared -fPIC mod-library.cc -o mod-library.so`.
*/

#include <stdio.h>
#include <cstdlib>
#include <string>

#include <libcufetch/common.hh>
#include <libcufetch/config.hh>
#include <libcufetch/cufetch.hh>

/* The handler that we'll use for our module, Handlers return `std::string`
 * and take as input the `const callbackInfo_t*` struct
 */
std::string test_func(const callbackInfo_t* _) {
    return "Hello!";
}

const char *useless_malloc;

/* Start function.

The start function takes in a handle, This handle is going to be the libcufetch library.
And also take the config class ConfigBase, instance loaded from customfetch.

It gets called once we have loaded the plugin shared library.

*/
APICALL EXPORT PLUGIN_INIT(void *handle, const ConfigBase& config) {

    /* Our goal in this example is to create a `modification.test` module, This will just return "Hello!" and nothing else. */
    /* The way we'll do this is we will create the test module, with no submodules and a handler (that just returns "Hello!"). */
    /* But then we will also create the modification module, which won't do anything other than hold the test module as a submodule. */
    /* We will then register the modification module. We won't register the test module because it is already a submodule and will be implicitly added. */

    /* Here we create the 'test' submodule. in customfetch there's no practical difference between a parent module and a submodule. So we just define it like any other module. */
    /* We will not register this, it will be implicitly added through its parent module (so we can't directly invoke `test`, we can only invoke `modification.test`) */
    module_t test_module = {"test", "a generic submodule description", {}, test_func};

    /* And here we create the 'modification' module. This is what we're actually going to register and it will include the test module as a submodule. */
    /* This module doesn't have a handler, so it can't be used in the config (`modification` won't work). We'll instead use `modification.test` in the config (which does have a handler). */
    module_t modification_module = { "modification", "root module description", { std::move(test_module) }, NULL };

    /* Register the module. */
    /* This will take the modification module, recursively add it and its submodules to the list, and continue until its finished everything. */
    cfRegisterModule(modification_module);

    /* Lookup the finish function on why this */
    useless_malloc = reinterpret_cast<const char*>(malloc(16));

    /* And done, after this customfetch will call our handler whenever our test module is invoked in the layout. */
}

/* Finish function.
 *
 * The finish function gets called when customfetch exists.
 * You should put free here any thing you have manually allocated to avoid memory leaks.
 */
APICALL EXPORT PLUGIN_FINISH() {
    std::free((void*)useless_malloc);
}
