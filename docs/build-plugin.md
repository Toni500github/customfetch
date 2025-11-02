# How to develop a customfetch plugin (still WIP)

Thanks to the new update since `v2.0.0-beta1`, customfetch is no longer just another neofetch-like program that fetches only system information. But it was made possible to develop and load user-made plugins for fetching any other information the user would like. 

Instead of installing different neofetch-like tools for different info fetching, e.g [onefetch](https://github.com/o2sh/onefetch) for viewing git tree infos or [githubfetch](https://github.com/isa-programmer/githubfetch) for viewing a GitHub user infos, you can use one neofetch-like tool (customfetch) and be able to view as many different infos as you would like to.

It's required having a knowledge about C++ programming (basic/mid) for creating a customfetch plugin.

## What are customfetch plugins used for?

Customfetch plugins are used for loading info tag `$<>` external modules, where at parsing it calls the module function automatically.  For now, it doesn't let the user modify more the aspect of customfetch generally, but just fetching external infos other than system ones.

## Developing and building the plugin

### Initializing

To make your plugins installable through customfetch's own plugin manager `cufetchpm`, you'll need to first create a git repository and a manifest file called `cufetchpm.toml`.

1. Create a git repository (`git init`)

2. run `cufetchpm gen-manifest` for generating a manifest template with comments explaning each config

After doing that, modify the manifest template with your repository name and url and other changes if needed.

### Developing

Now it's time to actually start developing the plugin. At the moment it's only compatible with C++ API, thus external languages such as C or Rust or Go, cannot create a binding of the libcufetch API.

In this example, we gonna create repository that contains only one plugin, but `cufetchpm` supports installing multiple plugins in the same repository, you can checkout https://github.com/Toni500github/customfetch-plugin-github/ for creating a repository with multiple plguins.

Let's create a simple `test-plugin.cc` source file. We're going to include the required libcufetch headers

```c++
#include <libcufetch/common.hh>
#include <libcufetch/config.hh>
#include <libcufetch/cufetch.hh>
```

<details>
  <summary><b>Click to expand for details about each header</b></summary>

  * `<libcufetch/common.hh>` will include logging calls `die`, `error`, `warn`, `info` and `debug` (only if customfetch is run with `--debug=1`)
    and also macros such as `APICALL`, `EXPORT`, `PLUGIN_INIT` and `PLUGIN_FINISH`, which can be useful for facilitating the plugin development.
  * `<libcufetch/config.hh>` will include the `ConfigBase` class which is used for accessing the configs of the config file customfetch uses.
  * `<libcufetch/cufetch.hh>` will include the important structs `moduleArgs_t`, `callbackInfo_t`, `module_t` and `cfRegisterModule()` for creating/registering modules that will be called from the config file.
</details>

And let's create our handler callback function:

```c++
std::string test_submod_func(const callbackInfo_t* cb) {
    return "Sub module";
}
```

An handler function must return a `std::string` and take in parameters a `const callbackInfo_t*`.



Now the important part starts here. We going to create our plugin main start entry function, which will be called once the plugin got loaded successfully:

```c++
APICALL EXPORT PLUGIN_INIT(void *handle, const ConfigBase& config) {
    
}
```

* `handle` is the plugin (library) main handler from `dlopen()`.

* `config` is the instance of the parsed config file from customfetch, where you can get parsed values of the whole file. 

Inside the main start entry (canonically `void start()`) we're going to declare and register all the modules the user can use.

Since it's possible to create recursive modules, we're going to start defining modules from bottom to top

```c++
/* Here we create the 'submod' submodule. in customfetch there's no practical difference between a parent module and a submodule. So we just define it like any other module. */
/* We will not register this, it will be implicitly added through its parent module (so we can't directly invoke `submod`, we can only invoke `root.submod`) */
module_t submod_module = {"submod", "a generic submodule description", {}, test_submod_func};

/* And here we create the 'root' module. This is what we're actually going to register and it will include the 'submod' module as a submodule. */
/* This module doesn't have a handler, so it can't be used in the config (`root` won't work). We'll instead use `root.submod` in the config (which does have a handler). */
module_t root_module = { "root", "root module description", { std::move(submod_module) }, NULL };

/* Register the module. */
/* This will take the root module, recursively add it and its submodules to the list, and continue until its finished everything. */
cfRegisterModule(root_module);
```

### Building
Building a customfetch plugin doesn't require any specific build-system such as cmake or meson, heck you can straight just put `c++ test-plugin.cc -std=c++17 -shared -fPIC -o test-plugin.so -Wl,--export-dynamic -Wl,-undefined,dynamic_lookup` in your plugin table `build-steps` section if it's a super simple Linux/android plugin.
