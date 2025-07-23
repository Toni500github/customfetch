#include "platform.hpp"
#if CF_MACOS

#include "core-modules.hh"
#include "libcufetch/common.hh"

// clang-format off
MODFUNC(battery_modelname)
{ return MAGIC_LINE; }

MODFUNC(battery_vendor)
{ return MAGIC_LINE; }

MODFUNC(battery_capacity_level)
{ return MAGIC_LINE; }

MODFUNC(battery_technology)
{ return MAGIC_LINE; }

MODFUNC(battery_status)
{ return MAGIC_LINE; }

MODFUNC(battery_perc)
{ return MAGIC_LINE; }

double battery_temp()
{ return 0; }

#endif
