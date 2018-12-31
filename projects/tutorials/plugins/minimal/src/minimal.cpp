
// includes, system
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// include LUA
#define LIBAPI	__declspec(dllexport)

#if defined(__cplusplus)
extern "C"
{
#endif

	#include <lua/lua.h>
	#include <lua/lauxlib.h>
	#include <lua/lualib.h>

	LIBAPI int luaopen_minimal(lua_State *L);

#if defined(__cplusplus)
}
#endif

static int sum(lua_State *L)
{

	int stacksize = lua_gettop(L);
	lua_Number summed = 0;

	if (stacksize < 1){
		lua_pushstring(L,"at least 1 number required");
		lua_error(L);
		return 1;
	}

	for (int i = 1; i <= stacksize; i++){
		lua_Number add = luaL_checknumber(L,i);
		summed += add;
	}

	lua_pushnumber(L,summed);

	return 1;
}

static const struct luaL_reg minimallib[] = {
	{"sum", sum},
	{NULL, NULL},
};


LIBAPI int luaopen_minimal(lua_State *L) {
	// openlib pushes a table on top of the lua stack
	// into which the functions were registered
	luaL_openlib (L, "minimal", minimallib, 0);

	// and tell lua that we returned a table on the stack
	return 1;
}
