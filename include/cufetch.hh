#include "common.hpp"

#include <vector>

/* Register a module, and its submodules, to customfetch. */
void cfRegisterModule(const module_t& module);

/* Get a list of all modules registered. */
const std::vector<module_t>& cfGetModules();
