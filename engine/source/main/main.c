// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/vid.h"
#include "main.h"

// Common

#include "../common/memorymanager.h"
#include "../common/timer.h"
#include "../common/console.h"
#include "../common/3dmath.h"
#include "../common/perfgraph.h"
// Controls
#include "../controls/controls.h"
// Resource
#include "../resource/texture.h"
#include "../resource/model.h"
#include "../resource/animation.h"
#include "../resource/sound.h"
#include "../resource/shader.h"
#include "../resource/particle.h"
#include "../resource/resmanager.h"
// Render
#include "../render/gl_render.h"
#include "../render/gl_window.h"
#include "../render/gl_list3d.h"
#include "../render/gl_list2d.h"
#include "../render/gl_print.h"
#include "../render/gl_particle.h"
#include "../render/gl_shader.h"
#include "../render/gl_window.h"

// File I/O
#include "../fileio/filesystem.h"
#include "../fileio/parser.h"

// Main
#include "../sound/soundlist.h"

#include "../scene/scenetree.h"
#include "../scene/vistest.h"
#include "../scene/actorlist.h"

#include "../fnpublish/fnpublish.h"
#include "luacore.h"


// GLOBALS
char*       g_projectpath = NULL;


// LOCALS
//  important for timers to work
static struct MAINData_s{
  double    difapp;     // time app takes
  double    lastframe;    // all timers
  int     regularexit;  // set if exit command is used
  double    frametimes[LUX_AVGTIME];
  double    sleeptime;
  int     curtimeslot;
  LuxTask_t curtask;
  LuxTask_t taskstack[LUX_TASKSTACK];
  int     taskstackdepth;
  int     initall;
  int     pause;
}l_MAINData;

//////////////////////////////////////////////////////////////////////////

#define bcstr(type) \
  case type:  return #type

static const char * LuxTask_str(LuxTask_t task)
{
  switch(task) {
    bcstr( LUX_TASK_MAIN );
    bcstr( LUX_TASK_RENDER );
    bcstr( LUX_TASK_SOUND );
    bcstr( LUX_TASK_LUA );
    bcstr( LUX_TASK_ACTOR );
    bcstr( LUX_TASK_SCENETREE );
    bcstr( LUX_TASK_EXIT);
    bcstr( LUX_TASK_INPUT);
    bcstr( LUX_TASK_TIMER);
    bcstr( LUX_TASK_THINK);
    bcstr( LUX_TASK_RESOURCE);
  default:
    return NULL;
  }
}
#undef bcstr

void Main_startTask(LuxTask_t task)
{
  LUX_DEBUG_FRPRINT ("%s start\n",LuxTask_str(task)); LUX_DEBUG_FRFLUSH;
  l_MAINData.curtask = task;
  if (l_MAINData.taskstackdepth == LUX_TASKSTACK-1){
    bprintf("ERROR: taskstack maxium\n");
    l_MAINData.taskstackdepth--;
  }
  l_MAINData.taskstack[++l_MAINData.taskstackdepth] = task;
}
void Main_endTask()
{
  LUX_DEBUG_FRPRINT ("%s end\n",LuxTask_str(l_MAINData.curtask)); LUX_DEBUG_FRFLUSH;
  l_MAINData.curtask = l_MAINData.taskstack[--l_MAINData.taskstackdepth];
}


void Main_deinitAll(int nocleanup)
{
  bprintf("Closing application:\n");
  bprintf("- Lua Core\n");
  LuaCore_deinit();

  if (nocleanup)
    return;

  bprintf("- Console\n");
  Console_deinit();

  bprintf("- SoundList\n");
  SoundList_deinit();

  bprintf("- Lua Close\n");
  lua_close(LuaCore_getMainState ());

  //bprintf("- FunctionPublishing Lua\n");
  //FunctionPublish_deinitLua(LuaCore_getMainState ());

  bprintf("- FunctionPublishing\n");
  FunctionPublish_deinit();

  bprintf("- SceneTree\n");
  SceneTree_deinit();

  bprintf("- Render Clear\n");
  Render_deinit();

  bprintf("- Resources\n");
  ResData_deinit();

  bprintf("- Reference\n");
  Reference_deinit();


  // final freeing of stuff that was still needed for gc
  bprintf("- VisTest\n");
  VisTest_free();

  bprintf("- VID\n");
  VID_deinitGL();
  VID_deinit();
  bprintf("- Cg\n");
  if (g_VID.cg.context)
    cgDestroyContext(g_VID.cg.context);

  // MEMORY
  bprintf("- MemoryStack\n");
  genfreestr(g_projectpath);
  lxMemoryStack_delete(g_BufferMemStack);

  bprintf("- DynMemory\n");
  DynMemory_deinit();

  bprintf("- MemoryGeneric\n");
  GenMemory_deinit();

  bprintf("deinitialization successful\n");
  LogDeinit();

  g_LuxiniaWinMgr.luxiTerminate();
}

int Main_exit(int nocleanup)
{
  l_MAINData.regularexit = LUX_TRUE;
  Main_startTask(LUX_TASK_EXIT);
  if (l_MAINData.initall)
    Main_deinitAll(nocleanup);
  exit(0); // <- important for threads: Deinit first, THEN call exit!
  return 0;
}

void clearall(void){

  if (l_MAINData.regularexit)
    return;
}

void Main_freezeTime(){
  l_MAINData.lastframe = getMicros();
}


/*
typedef void (ReferenceFunc)(void *ref,void* state,...);

void testfunc (void * ref, void *state, int a, float b)
{
  LUX_PRINTF("-> %i %f",a,b);
}

void testit ()
{
  ReferenceFunc *func = (ReferenceFunc*)testfunc;
  union {
    int ls[0xff];
    float fs[0xff];
  } t;
  t.ls[0] = 1;
  t.fs[1] = 2.1f;
  func(NULL,NULL, t);

}

int main(int argc, char **argv)
{
  testit();
  return 0;
}
*/

static int l_fps = 0;

static booln fp_defaultcheck(const char *name){
  booln negated = (name && name[0] == '!' && (name=name+1));
  booln found = (VIDUserDefine_get(name) || ResData_isDefinedCgCstring(name));

  return negated^found;
}

static Char2PtrNode_t* fp_defaultalloc(const char *str, const char* buffer, size_t buffersize){
  Char2PtrNode_t* annotation;

  annotation = reszalloc(sizeof(Char2PtrNode_t));
  resnewstrcpy(annotation->name,str);
  annotation->str = reszalloc(buffersize);
  memcpy(annotation->str,buffer,buffersize);
  annotation->str[buffersize-1] = 0;

  return annotation;
}



int Main_start(struct lua_State *Lstate, const int argc, const char **argv)
{

  int i,n;
  char *blah = NULL;
  int l3dsets = 4,
    l3dlayers = 16,
    l3dmaxdraws = 16384,
    l3dmaxprojs = 8192;
  int membuffer = 16,
    memresdefchunk = 16;

  memset(&l_MAINData,0,sizeof(struct MAINData_s));
  l_MAINData.curtask = LUX_TASK_MAIN;

  ProfileData_clearAll();

  LuaCore_setMainState(Lstate);

  l_MAINData.taskstack[0] = LUX_TASK_MAIN;


  LUX_PRINTF("%s\nLoading application:\n\n",LUX_VERSION);
  LUX_PRINTF("- Log\n");
  // main setup
  LogInit();
  bprintf("- MemoryGeneric\n");
  GenMemory_init();
  bprintf("- FileSystem\n");
  // filesystem
  FS_init();
  FS_checkcrc();
  FileParse_setDefineCheck(fp_defaultcheck);
  FileParse_setAnnotationAlloc(fp_defaultalloc);
  {
    float scale[3] = {1.0f,1.0f,1.0f};
    ModelLoader_setPrescale(scale);
  }


  bprintf("- Window Internal init\n");
  LuxWindow_init(&g_Window);
  bprintf("- Commandline\n");
  for(i=0;i<argc;i++)
  {
    if((strcmp(argv[i],"--projectpath") == 0 || strcmp(argv[i],"-p") == 0) && argc-1 > i)
    {
      n = strlen(argv[i+1]);
      g_projectpath = lxMemGenZalloc(sizeof(char)*(n+2));
      strcpy(g_projectpath,argv[i+1]);
      g_projectpath[n]='/';
    }
    else if(strcmp(argv[i], "--fullscreen") == 0 || strcmp(argv[i], "-f") == 0){
      setFullscreen(LUX_TRUE);
    }
    else if (strcmp(argv[i],"--l3dconfig") == 0 || strcmp(argv[i], "-l") == 0 && argc-1 > i){
      if(sscanf(argv[i+1],"%d;%d;%d;%d",&l3dsets,&l3dlayers,&l3dmaxdraws,&l3dmaxprojs))
        bprintf("- USING l3dconfig: %d;%d;%d;%d\n",l3dsets,l3dlayers,l3dmaxdraws,l3dmaxprojs);
    }
    else if (strcmp(argv[i],"--memconfig") == 0 || strcmp(argv[i], "-m") == 0 && argc-1 > i){
      if(sscanf(argv[i+1],"%d;%d",&membuffer,&memresdefchunk))
        bprintf("- USING memconfig: %d,%d (MBytes)\n",membuffer,memresdefchunk);
    }
  }

  if (!g_projectpath)
    gennewstrcpy(g_projectpath,LUX_BASEPATH);


  bprintf("- MemoryStacks\n");
  // reosources
  g_BufferMemStack = lxMemoryStack_new(GLOBAL_ALLOCATOR,"buffer",LUX_MEMORY_MEGS(membuffer));
  bprintf("- Resources\n");
  ResData_init(LUX_MEMORY_KBS(memresdefchunk));
  bprintf("- Reference\n");
  Reference_init();
  bprintf("- RenderHelpers\n");
  RenderHelpers_init();
  VID_clear();
  bprintf("- FunctionPublish\n");
  FunctionPublish_init();
  bprintf("- Lua Core\n");
  Main_startTask(LUX_TASK_LUA);
  LuaCore_init(argc,argv);
  Main_endTask();

  bprintf("- Console\n");
  Console_init();
  bprintf("- Math\n");
  Math_init();

  g_LuxTimer.time = 0;
  g_LuxTimer.timef = 0.0f;


  // window
  bprintf("- Window GL\n");
  LuxWindow_setGL(&g_Window);

  bprintf("- Sound\n");
  SoundList_init();
  bprintf("- VID\n");
  if (!VID_init()){
    bprintf("... caused exit\n");
    return 1;
  }

  LuxWindow_updatedWindow();
  bprintf("- Render\n");
  Render_init(l3dsets,l3dlayers,l3dmaxdraws,l3dmaxprojs);
  bprintf("- PGraph\n");
  PGraphs_init();
  bprintf("- VisTest\n");
  VisTest_init();
  bprintf("- SceneTree\n");
  SceneTree_init();
  bprintf("- ActorList\n");
  ActorList_init();
  bprintf("- Input Callbacks\n");
  Main_init();
  l_MAINData.initall = LUX_TRUE;
  Main_endTask();

  return 0;
}



void Main_frame(void)
{
  Main_startTask(LUX_TASK_TIMER);
  Main_think(-1.0f,LUX_TRUE,LUX_FALSE);

  if (g_VID.maxFps && g_LuxTimer.fps > g_VID.maxFps+10 && getFPSAvg() >= g_VID.maxFps+10){
    LUX_DEBUG_FRPRINT("timer sleep\n");
    g_LuxiniaWinMgr.luxiSleep(0.0001);
  }

  Main_endTask();

}

// MAIN INIT
// --------------



void Main_init(){
  g_LuxiniaWinMgr.luxiSetCallbacks();

  g_LuxiniaWinMgr.luxiEnable(LUXI_KEY_REPEAT);
  Controls_setMousePosition(320,240);
  g_LuxiniaWinMgr.luxiDisable(LUXI_MOUSE_CURSOR);
  g_LuxiniaWinMgr.luxiSetMousePos(g_Window.width >> 1,g_Window.height >> 1);

  // we need to reset think time in case we come from
  // a resolution change so that the time for a reinit
  // is not lost but whole app is frozen for the time
  // the reinit takes.

  l_MAINData.lastframe = getMicros();
}


void Main_sleep(int timems){
  double time = (timems/1000.0f);

  l_MAINData.sleeptime += time;

  g_LuxiniaWinMgr.luxiSleep(time);
}

// THINK
// -----



void Main_think(float timediff, int autoprocess, int drawfirst)
{
  GLenum errCode;
  double curtime;
  int i;
  int canrender;
  Main_startTask(LUX_TASK_THINK);

  canrender = g_LuxiniaWinMgr.luxiGetWindowParam(LUXI_OPENED) && !LuxWindow_poppedGL();

  LUX_DEBUG_FRPRINT("profile clear\n");
  ProfileData_clearPerFrame();

  LUX_PROFILING_OP (g_Profiling.global.memory.luausemem = g_LuxiniaWinMgr.luxiLuaGetMemUsed());
  LUX_PROFILING_OP (g_Profiling.global.memory.luaallocmem = g_LuxiniaWinMgr.luxiLuaGetMemAllocated());

  LUX_DEBUG_FRPRINT("think proc time\n");

  curtime  = getMicros();

  l_MAINData.difapp= curtime-l_MAINData.lastframe;
  l_MAINData.difapp = LUX_MAX(100,(l_MAINData.difapp - (l_MAINData.sleeptime*1000)));

  g_LuxTimer.timediff = 0.001*(float)l_MAINData.difapp;
  g_LuxTimer.fps = (int)(1000000.0f/(float)l_MAINData.difapp);

  l_MAINData.sleeptime = 0;

  // we want to cap FPS a bit
  if (autoprocess && g_LuxTimer.fps > g_VID.maxFps && g_VID.maxFps != 0){
    Main_endTask();
    return;
  }

  l_MAINData.frametimes[l_MAINData.curtimeslot] -= curtime;
  l_MAINData.curtimeslot = (l_MAINData.curtimeslot+1)%LUX_AVGTIME;
  g_LuxTimer.avgtimediff = 0.0;
  for (i=0; i < LUX_AVGTIME;i++)
    g_LuxTimer.avgtimediff += -l_MAINData.frametimes[i];
  g_LuxTimer.avgtimediff /= (double)LUX_AVGTIME;
  g_LuxTimer.avgtimediff *= 0.001;

  l_MAINData.frametimes[l_MAINData.curtimeslot] = curtime;

  if (l_MAINData.pause){
    g_LuxTimer.timediff = 0.0f;
  }
  else if (timediff > 0)
    g_LuxTimer.timediff = timediff;

  g_LuxTimer.timef += g_LuxTimer.timediff;
  g_LuxTimer.time = (int)g_LuxTimer.timef;

  if (g_Draws.drawPerfGraph){
    LUX_PROFILING_OP (g_Profiling.perframe.timers.frame = -curtime);
  }

  // maybe should not depend on resizemode ?
  LUX_DEBUG_FRPRINT("think window update\n");
  if (g_Window.resizetime && curtime>g_Window.resizetime){
    g_Window.resizetime = 0;
    LuxWindow_updatedWindow();
  }


  // start draw if in automatic mode
  if (autoprocess || drawfirst){
    Main_startTask(LUX_TASK_RENDER);
    if (canrender)
      Render();
    Main_endTask();
  }

  l_MAINData.lastframe = curtime;

  if (g_Draws.drawPerfGraph){
    LUX_PROFILING_OP (g_Profiling.perframe.timers.think = -getMicros());
    LUX_PROFILING_OP (g_Profiling.perframe.timers.ode = 0);
  }


  // do main stuff here


  if (autoprocess){
    Main_startTask(LUX_TASK_LUA);
    LuaCore_think ();
    Main_endTask();
  }

  Main_startTask(LUX_TASK_SOUND);
  SoundList_update();
  Main_endTask();

  Main_startTask(LUX_TASK_ACTOR);
  ActorList_think();
  Main_endTask();

  Main_startTask(LUX_TASK_SCENETREE);
  SceneTree_run();
  Main_endTask();

  LUX_DEBUG_FRPRINT("think gl error\n");
  while (errCode = glGetError() != GL_NO_ERROR) {
    dprintf ("ERROR OpenGL: %s %d\n", gluErrorString(errCode), errCode);
  }

  if (g_Draws.drawPerfGraph){
    LUX_PROFILING_OP (g_Profiling.perframe.timers.think += getMicros() - g_Profiling.perframe.timers.ode);
  }

  if (autoprocess){
    if(g_VID.autodump){
      vidScreenShot(g_VID.autodumptga);
      if (g_VID.frameCnt % 10 > 3){
        PText_t ptext;
        PText_setDefaults(&ptext);
        PText_setSize(&ptext,8.0f);

        Draw_RawText_start();
        vidColor4f(1,1,1,1);
        Draw_PText(0.0f,0.0f,0.0f,NULL,&ptext,"\vb\v900 R E C");
        Draw_RawText_end();
      }

    }

    if (canrender){
      //double swaptime = -getMicros();
      LUX_DEBUG_FRPRINT("think swap buffers\n");
      g_LuxiniaWinMgr.luxiSwapBuffers();
      if (g_Draws.forceFinish)
        glFinish();

      //glFinish();
      //swaptime += getMicros();
      //LUX_PRINTF("Swaptime: %f\n",swaptime);
    }
  }
  else if (!drawfirst){
    // in manual mode we start rendering at the end
    Main_startTask(LUX_TASK_RENDER);
    if (canrender)
      Render();
    Main_endTask();
  }

  if (g_Draws.drawPerfGraph){
    LUX_PROFILING_OP (g_Profiling.perframe.timers.frame += getMicros());
    PGraph_addSample(PGRAPH_FRAME,(float)g_Profiling.perframe.timers.frame*0.001f);
    PGraph_addSample(PGRAPH_THINK,(float)g_Profiling.perframe.timers.think*0.001f);
    PGraph_addSample(PGRAPH_ODESTEP,(float)g_Profiling.perframe.timers.ode*0.001f);
  }

  Main_endTask();
}

void Main_pause(int state)
{
  l_MAINData.pause = state;
}



