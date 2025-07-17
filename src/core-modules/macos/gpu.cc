#include "platform.hpp"
#if CF_MACOS

#include "core-modules.hh"
#include "libcufetch/common.hh"

MODFUNC(gpu_name)
{ return MAGIC_LINE; }

MODFUNC(gpu_vendor)
{ return "Intel"; }

#endif
