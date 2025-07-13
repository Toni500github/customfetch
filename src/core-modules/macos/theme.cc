#include "platform.hpp"
#if CF_MACOS

#include "core-modules.hh"
#include "cufetch/common.hh"

MODFUNC(theme_gtk_name) { return MAGIC_LINE; }
MODFUNC(theme_gtk_icon) { return MAGIC_LINE; }
MODFUNC(theme_gtk_font) { return MAGIC_LINE; }
MODFUNC(theme_cursor_name) { return MAGIC_LINE; }
MODFUNC(theme_cursor_size) { return MAGIC_LINE; }
MODFUNC(theme_gtk_all_name) { return MAGIC_LINE; }
MODFUNC(theme_gtk_all_icon) { return MAGIC_LINE; }
MODFUNC(theme_gtk_all_font) { return MAGIC_LINE; }
MODFUNC(theme_gsettings_name) { return MAGIC_LINE; }
MODFUNC(theme_gsettings_icon) { return MAGIC_LINE; }
MODFUNC(theme_gsettings_font) { return MAGIC_LINE; }
MODFUNC(theme_gsettings_cursor_name) { return MAGIC_LINE; }
MODFUNC(theme_gsettings_cursor_size) { return MAGIC_LINE; }

#endif  // CF_MACOS
