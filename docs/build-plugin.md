# Developing a Customfetch Plugin

> [!Note]
> This guide is a work in progress and may change as customfetch continues to develop.

Since version `v2.0.0-beta1`, **customfetch** has evolved beyond being just another neofetch-like program that displays system information.  
It now supports **user-made plugins**, allowing developers to fetch and display any type of information they want directly within customfetch.

This means you no longer need separate tools like [onefetch](https://github.com/o2sh/onefetch) (for Git information) or [githubfetch](https://github.com/isa-programmer/githubfetch) (for GitHub profile information).  
With customfetch, you can display as much and different fetched infos you'd like in one single config.

Developing a customfetch plugin requires **basic to intermediate C++ knowledge**, as also Git.

---

## What Are Customfetch Plugins?

Customfetch plugins are dynamically loaded modules that provide external info fetching, used in the info tag `$<>`.  
When the configuration is parsed, customfetch automatically calls the corresponding plugin function to fetch and display the data.

Currently, plugins can only extend **data fetching** capabilities — they cannot modify the overall appearance or behavior of customfetch itself.


## 1. Initializing Your Plugin Project

If you want your plugin to be installable through customfetch’s plugin manager, `cufetchpm`, start by setting up your repository and manifest.

1. **Create a new Git repository**

2. **Generate a template manifest file:**

```bash
$ cufetchpm gen-manifest
```
This command creates a `cufetchpm.toml` file with explanatory comments for each configuration option.
After doing that, modify the manifest template with your repository name and url and other changes if needed.

## 2. Developing the Plugin
Currently, plugins are only supported via the C++ API. Bindings for other languages like C, Rust, or Go are not yet available.

For this example, we will create a simple repository containing one plugin.  
If you want to see how to include multiple plugins in a single repository, refer to the [customfetch-plugin-github](https://github.com/Toni500github/customfetch-plugin-github/) example.

### Example Plugin Source

Create a new file called `test-plugin.cc` and include the required headers:

```c++
#include <libcufetch/common.hh>
#include <libcufetch/config.hh>
#include <libcufetch/cufetch.hh>
```

<details>
<summary><b>About the headers</b></summary>

* `<libcufetch/common.hh>` — includes logging utilities (`die`, `error`, `warn`, `info`, `debug`) and helpful macros such as `APICALL`, `EXPORT`, `PLUGIN_INIT`, and `PLUGIN_FINISH`.
* `<libcufetch/config.hh>` — provides access to the `ConfigBase` class for reading configuration values.
* `<libcufetch/cufetch.hh>` — defines important structs like `moduleArgs_t`, `callbackInfo_t`, `module_t`, and the `cfRegisterModule()` function used for module registration.
</details>

---

### Defining a Handler Function

A handler function must return a `std::string` and take a `const callbackInfo_t*` parameter.  
Let’s create a simple submodule handler:

```c++
std::string test_submod_func(const callbackInfo_t* cb) {
    return "Sub module";
}
```
<details>
<summary><b>About callbackInfo_t</b></summary>
  
  The struct `callbackInfo_t` contains:
  * `const moduleArgs_t* moduleArgs`: A linked list including module arguments. An argument may be specified for any part of the module path (e.g. `disk(/).used(GiB)`, `test.hi(a)`)
  * `parse_args_t& parse_args`: a context struct that can be used in the `parse()` function, for parsing strings with tags.
      ```c++
      /* Context struct used when parsing tags in strings.
       * @param modules_info The modules fetched infos
       * @param config The config instance
       * @param pure_output The output of the string but without tags
       * @param layout The layout of customfetch
       * @param tmp_layout The temponary layout to be used for multiple-line modules
       * @param no_more_reset uhh let me see
       * @param parsing_layout Are we parsing the layout or the ASCII art logo?
       */
      struct EXPORT parse_args_t
      {
          const moduleMap_t&        modules_info;
          const ConfigBase&         config;
          std::string&              pure_output;
          std::vector<std::string>& layout;
          std::vector<std::string>& tmp_layout;
          bool                      parsing_layout;
          bool                      no_more_reset = false;
          bool                      firstrun_clr  = true; // don't use it. Internal "flag"
      };
      ```
</details>

---

### Plugin Initialization Function

Next, define the plugin’s main initialization entry point.  
This function is automatically called when the plugin is successfully loaded.

```c++
APICALL EXPORT PLUGIN_INIT(void *handle, const ConfigBase& config) {
 
}
```
* `handle` — The plugin (shared library) handle, as returned by `dlopen()`.
* `config` — An instance of the parsed customfetch config file.

Inside this function, you will declare and register the modules your plugin provides.

---

### Defining and Registering Modules

customfetch allows defining **nested modules** (submodules).  
For this reason, it’s best to define modules from the bottom up.

```c++
// Define a submodule. There’s no practical difference between a module and a submodule in customfetch.
module_t submod_module = { "submod", "A generic submodule description", {}, test_submod_func };

// Define a root module that contains the submodule.
module_t root_module = { "root", "Root module description", { std::move(submod_module) }, NULL };

// Register the root module (and all its submodules recursively).
cfRegisterModule(root_module);
```

In this example:
- `root.submod` can be used in the customfetch configuration file.
- The parent module `root` does not have a handler, so it will return `(unknown/invalid module)`

---

## 3. Building the Plugin
> [!Note]
> The build command shown is for Linux/Unix systems. Windows and macOS may require different compiler flags and file extensions (`.dll` for Windows, `.dylib` for macOS).

You can build a plugin using any build system, but a simple compiler command works fine:

```bash
c++ test-plugin.cc -std=c++17 -shared -fPIC -o test-plugin.so -Wl,--export-dynamic -Wl,-undefined,dynamic_lookup
```

If you defined this in your manifest’s `build-steps` plugin section, `cufetchpm` can build it automatically when installing.

---

## 4. Trying Out the Plugin
Now save and commit all your changes and `git push` to the remote repository.
To install your plugin, you need to run:
```bash
$ cufetchpm install <your-git-repo-url>
```

If `cufetchpm` doesn't report any errors, congratulations!
To try out your plugin, run:
```bash
$ customfetch -nm "\$<root.submod>"
Sub module
```

## Tips and Troubleshooting

- Ensure your handler functions strictly match the required signature.
- Always call `cfRegisterModule()` for your top-level modules.
- When debugging, use `--debug=1` to view additional logs from your plugin.
- If your plugin doesn't load, verify symbol exports with:
```bash
nm -D test-plugin.so | grep "start"
```
