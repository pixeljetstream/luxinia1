
// includes, system
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// includes, GL
#include <GL/glew.h>


// includes cuda
#include <cuda_runtime.h>
#include <cutil_inline.h>
#include <cuda_gl_interop.h>
#include <vector_types.h>

// include LUA/Luxinia
#define LIBAPI	__declspec(dllexport)

#if defined(__cplusplus)
extern "C"
{
#endif

	#include <lua/lua.h>
	#include <lua/lauxlib.h>
	#include <lua/lualib.h>

	LIBAPI int luaopen_cudatest(lua_State *L);

#if defined(__cplusplus)
}
#endif


////////////////////////////////////////////////////////////////////////////////
// kernels
//#include <simpleGL_kernel.cu>

extern "C" 
void launch_kernel(struct Vertex16Color_s* pos, unsigned int mesh_width, unsigned int mesh_height, float time);

//////////////////////////////////////////////////////////////////////////
// Setup

void cuda_setup()
{
	cudaGLSetGLDevice( cutGetMaxGflopsDeviceId() );
}


//////////////////////////////////////////////////////////////////////////
// The Functions we want to make accessible with lua

static int cuda_register_buffer(lua_State *L)
{
	// get vbo
	luaL_checktype(L,1,LUA_TLIGHTUSERDATA);
	GLuint vbo = reinterpret_cast<GLuint>(lua_touserdata(L,1));
	
	cutilSafeCall(cudaGLRegisterBufferObject(vbo));

	return 0;
}

static int cuda_unregister_buffer(lua_State *L)
{
	// get vbo
	luaL_checktype(L,1,LUA_TLIGHTUSERDATA);
	GLuint vbo = reinterpret_cast<GLuint>(lua_touserdata(L,1));

	cutilSafeCall(cudaGLUnregisterBufferObject(vbo));


	return 0;
}

static int cuda_run_testkernel(lua_State *L)
{
	// the argumens we want
	GLuint vbo = 0;
	int	mesh_width = 0;
	int	mesh_height = 0;
	float anim = 0.0f;

	int stacksize = lua_gettop(L);
	if (stacksize < 4){
		lua_pushstring(L,"1 luserdata (GLuint buffer) 1 int 1 int 1 float required");
		lua_error(L);
		return 1;
	}
	// get vbo
	luaL_checktype(L,1,LUA_TLIGHTUSERDATA);
	vbo = reinterpret_cast<GLuint>(lua_touserdata(L,1));

	// get the numbers, by default lua has one numbertype (double)
	mesh_width = luaL_checkint(L,2);
	mesh_height = luaL_checkint(L,3);
	anim = (float)luaL_checknumber(L,4);

	// some more error checking
	if (mesh_width < 8 || mesh_height < 8){
		lua_pushstring(L,"grid dimension minium 8x8");
		lua_error(L);
		return 1;
	}

	// map OpenGL buffer object for writing from CUDA
	struct Vertex16Color_s *dptr;
	cutilSafeCall(cudaGLMapBufferObject((void**)&dptr, vbo));

	// execute the kernel
	//    dim3 block(8, 8, 1);
	//    dim3 grid(mesh_width / block.x, mesh_height / block.y, 1);
	//    kernel<<< grid, block>>>(dptr, mesh_width, mesh_height, anim);

	launch_kernel(dptr, mesh_width, mesh_height, anim);

	// unmap buffer object
	cutilSafeCall(cudaGLUnmapBufferObject(vbo));

	return 0;
}


//////////////////////////////////////////////////////////////////////////
// Lua Lib Setup

static void set_info (lua_State *L) {
	// assumes table on top of stack
	//: tab

	lua_pushliteral (L, "_COPYRIGHT");
	// : tab,key

	lua_pushliteral (L, "Copyright (C) 2009 - Christoph Kubisch");
	// : tab,key,value

	// set value in table at -3,
	// ie tab[key] = value
	// key,value pair is always on top of stack
	// and then popped
	lua_settable (L, -3);
	// : tab


	lua_pushliteral (L, "_DESCRIPTION");
	lua_pushliteral (L, "Cuda/OpenGL Test");
	lua_settable (L, -3);
	lua_pushliteral (L, "_VERSION");
	lua_pushliteral (L, "1.0");
	lua_settable (L, -3);
}


static const struct luaL_reg cudatestlib[] = {
	{"registerbuffer", cuda_register_buffer},
	{"unregisterbuffer", cuda_unregister_buffer},
	{"runtestkernel",cuda_run_testkernel},
	{NULL, NULL},
};


LIBAPI int luaopen_cudatest(lua_State *L) {
	// openlib pushes a table on top of the lua stack
	// into which the functions were registered
	luaL_openlib (L, "cudatest", cudatestlib, 0);
	// we also store some info into that table
	set_info (L);

	cuda_setup();

	// and tell lua that we returned a table on the stack
	return 1;
}



