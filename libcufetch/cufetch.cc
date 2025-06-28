#include "cufetch.hh"

static std::vector<module_t> modules;

static char separator = '.';

static void addModule(module_t module, const std::string& prefix = "")
{
    module.name = prefix + module.name;
    modules.push_back(module); // No std::move since we modify name first

    for (module_t submodule : module.submodules) // Copy submodule to avoid reference issues
        addModule(std::move(submodule), prefix + module.name + separator);
}

/* Register a module, and its submodules, to customfetch. */
void cfRegisterModule(const module_t& module) { addModule(module); }

/* Get a list of all modules registered. */
const std::vector<module_t>& cfGetModules() { return modules; }
