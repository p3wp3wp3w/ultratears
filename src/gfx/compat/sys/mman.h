// Stand-in for the macOS header <sys/mman.h> (MacNativeGames code on Windows): definitions live in gfx_compat.h.
#pragma once
#ifdef __linux__
#include_next <sys/mman.h>
#else
#include "gfx_compat.h"
#endif
