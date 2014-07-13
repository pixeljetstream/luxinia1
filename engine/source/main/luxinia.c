// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "main.h"
#include "luacore.h"
#include "../render/gl_window.h"
#include "../controls/controls.h"

LuxiniaWinMgr_t       g_LuxiniaWinMgr;
static LuxiniaCallbacks_t l_LuxiniaCallbacks = {
  Main_frame,       //void (LUXICALL * LUXIframefun)(void);
  LuxWindow_reshape,    //void (LUXICALL * LUXIwindowsizefun)(int,int);
  clearall,       //void (LUXICALL * LUXIatexitfun)(void);
  Controls_mouseClick,  //void (LUXICALL * LUXImousebuttonfun)(int,int);
  Controls_mouseInput,  //void (LUXICALL * LUXImouseposfun)(int,int);
  Controls_keyInput,    //void (LUXICALL * LUXIkeyfun)(int,int,int);
  //NULL,         //void (LUXICALL * LUXImousewheelfun)(int);
};

LUXIAPI void  Luxina_getCallbacks(LuxiniaCallbacks_t *destination)
{
  memcpy(destination,&l_LuxiniaCallbacks,sizeof(LuxiniaCallbacks_t));
}

LUXIAPI int   Luxina_setWinMgr(const LuxiniaWinMgr_t *source)
{
  const void **ptr = (void**)source;
  const void **lastptr = ptr+(sizeof(LuxiniaWinMgr_t)/sizeof(void*));
  while (ptr < lastptr){
    if (*ptr == NULL)
      return LUXI_FALSE;
    ptr++;
  }

  memcpy(&g_LuxiniaWinMgr,source,sizeof(LuxiniaWinMgr_t));
  return LUXI_TRUE;
}

LUXIAPI int   Luxina_start(struct lua_State *lstate,const int argc, const char **argv)
{
  return Main_start(lstate,argc,argv);
}
LUXIAPI struct lua_State*  Luxina_getLuaState(){
  return LuaCore_getMainState();
}
