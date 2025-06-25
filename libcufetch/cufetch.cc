#include "cufetch.hh"

static std::vector<module_t> modules;

static char separator = '.';

static void addModule(const module_t& module, const std::string& prefix = "")
{
    modules.emplace_back(module).name = prefix + module.name;

    for (const module_t& submodule : module.submodules)
        addModule(submodule, prefix + module.name + separator);
}

/* Register a module, and its submodules, to customfetch. */
void cfRegisterModule(const module_t& module) { addModule(module); }

/* Get a list of all modules registered. */
const std::vector<module_t>& cfGetModules() { return modules; }
