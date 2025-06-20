#include <memory>
#include <vector>

#include "common.hpp"

static std::vector<module_t> modules;

/* TODO: can we customize the separator perhaps? */
static char separator = '.';

extern "C" {
    static void addModule(const module_t &module, const std::string &prefix = "") {
        modules.emplace_back(module).name = prefix + module.name;
        
        for (const module_t &submodule : module.submodules) {
            addModule(submodule, module.name + separator);
        }
    }

    /* Register a module, and its submodules, to customfetch. */
    [[gnu::unused]] void cfRegisterModule(const module_t &module) {
        addModule(module);
    }

    /* Get a list of all modules registered. */
    [[gnu::unused]] const std::vector<module_t> &cfGetModules() {
        return modules;
    }
}
