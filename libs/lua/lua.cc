#if _MSC_VER
#pragma push_macro("_CRT_SECURE_NO_WARNINGS")
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#ifdef PLATFORM_WINDOWS
# define LUA_USE_WINDOWS
#elif defined __APPLE__
# define LUA_USE_MACOSX
#else
# define LUA_USE_LINUX
#endif

extern "C"
{

#define lobject_c
#define lvm_c
#define LUA_CORE
#define LUA_LIB
#include "lua-5.3.1/luaconf.h"
#undef lobject_c
#undef lvm_c
#undef LUA_CORE
#undef LUA_LIB

// disable support for extenal libs
#undef LUA_DL_DLL
#undef LUA_USE_DLOPEN

#if _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4244) /* Possible loss of data */
#pragma warning (disable: 4702) /* Unreachable code */
#endif

#include "lua-5.3.1/ltable.c"

#include "lua-5.3.1/lauxlib.c"
#include "lua-5.3.1/lbaselib.c"

#include "lua-5.3.1/lbitlib.c"
#include "lua-5.3.1/lcorolib.c"
#include "lua-5.3.1/ldblib.c"
#include "lua-5.3.1/linit.c"
#include "lua-5.3.1/liolib.c"
#include "lua-5.3.1/lmathlib.c"
#include "lua-5.3.1/loslib.c"
#include "lua-5.3.1/lstrlib.c"
#include "lua-5.3.1/ltablib.c"

#include "lua-5.3.1/lapi.c"
#include "lua-5.3.1/lcode.c"
#include "lua-5.3.1/lctype.c"
#include "lua-5.3.1/ldebug.c"
#include "lua-5.3.1/ldo.c"
#include "lua-5.3.1/ldump.c"
#include "lua-5.3.1/lfunc.c"
#include "lua-5.3.1/lgc.c"
#include "lua-5.3.1/llex.c"
#include "lua-5.3.1/lmem.c"
#include "lua-5.3.1/lobject.c"
#include "lua-5.3.1/lopcodes.c"
#include "lua-5.3.1/lparser.c"
#include "lua-5.3.1/lstate.c"
#include "lua-5.3.1/lstring.c"
#include "lua-5.3.1/ltm.c"
#include "lua-5.3.1/lundump.c"
#include "lua-5.3.1/lutf8lib.c"
#include "lua-5.3.1/lvm.c"
#include "lua-5.3.1/lzio.c"

#include "lua-5.3.1/loadlib.c"

#if _MSC_VER
#pragma warning (pop)
#endif

}

#if _MSC_VER
#pragma pop_macro("_CRT_SECURE_NO_WARNINGS")
#endif
