#ifndef __gtkardour_luabindings_h__
#define __gtkardour_luabindings_h__

#include "lua/lua.h"

namespace ArdourLuaScripting {
	void register_classes (lua_State* L);
} // namespace

#endif /* __gtkardour_luabindings_h__ */
