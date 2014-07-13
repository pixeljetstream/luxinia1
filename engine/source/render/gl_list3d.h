// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __GL_LIST3D_H__
#define __GL_LIST3D_H__

/*
List3D
------

The List3D is used to collect everything that is supposed to be rendered in the perspective view.
At the beginning of each frame the list is rendered. It also contains a bit self optimizing via
sorting for shaders and texture useage. Sortkey is priorized: number of lights > shader > material

List3D allows a little hierarchy to e.g. bind models to other model's bones. Though only the base L3DNode
has a matrix/state interface.

The List3D is split into different List3DSets, which work as standalone worlds
Each set has infos regarding cameras, lights, projectors and background settings.
When no info is provided the first set's settings are used

To have a bit of influence on when things are drawn, the set is separated into
Layers. Each layer contains the nodes to be rendered.
Layer LAYER_FTB is drawn first then subsequent. There is no transparency sorting so far (only within ParticleSystem)
hence use the layers wisely, however certain layers are sorted for depth based on node's position.

Use the "new" functions to get a node which you can work with. When you are sure you dont need the node anymore please
use free to release the node again. It becomes active when its State and Matrix44 Interface were set.


List3DNode  (user,GenMem)
List3DView  (user,GenMem)
List3DSet (nested,FixedMem)
List3D    (nested,FixedMem)

Author: Christoph Kubisch

*/


#include "../common/interfaceobjects.h"
#include "../common/reflib.h"
#include "../resource/resmanager.h"
#include "../scene/linkobject.h"
#include "gl_camlight.h"
#include "gl_print.h"
#include "gl_render.h"
#include "gl_draw3d.h"
#include "gl_list3dbatch.h"
#include "gl_list3dgeometry.h"
#include "gl_shader.h"
#include "gl_trail.h"


#define LIST3D_SET_PROJECTORS 32
#define LIST3D_VIEW_DEFAULT_COMMANDS  255

#define LIST3D_MAX_TOTAL_PROJECTORS   g_List3D.maxTotalProjectors
#define LIST3D_LAYER_MAX_NODES      g_List3D.maxDrawPerLayer
#define LIST3D_LAYER_MAX_NODES_MASK   g_List3D.maxDrawPerLayerMask
#define LIST3D_LAYERS         g_List3D.maxLayers
#define LIST3D_SETS           g_List3D.maxSets

// ORDERS MUST NOT CHANGE !!
// here we define what matrix operations we perform in the position update
typedef enum List3DUpdateType_e{
  LIST3D_UPD_UNLINKED,
  LIST3D_UPD,
  LIST3D_UPD_INH,
  LIST3D_UPD_LOCAL,
  LIST3D_UPD_INH_LOCAL,
  LIST3D_UPD_PARENT,
  LIST3D_UPD_PARENT_INH,
  LIST3D_UPD_PARENT_LOCAL,
  LIST3D_UPD_PARENT_INH_LOCAL,
  LIST3D_UPD_PARENT_BONE,
  LIST3D_UPD_PARENT_BONE_INH,
  LIST3D_UPD_PARENT_BONE_LOCAL,
  LIST3D_UPD_PARENT_BONE_INH_LOCAL,
  LIST3D_UPD_MANUAL_WORLD,
  LIST3D_UPD_PARENT_MANUAL_WORLD,
  LIST3D_UPDS,
}List3DUpdateType_t;
enum List3DUpdateShifts_e{
  LIST3D_UPD_SHIFT_INH = 1,
  LIST3D_UPD_SHIFT_LOCAL  = 2,
  LIST3D_UPD_SHIFT_BONE = 4,
  LIST3D_UPD_SHIFT_PARENT = 4,
};

// if special stuff is done in the updates, if there is animation..
// we do it in the matrix update, cause children could rely on the bones data...
typedef enum List3DSpecialType_e{
  LIST3D_SPC_NONE,
  LIST3D_SPC_MODEL_ANIM,    // bones and/or morph
  LIST3D_SPC_TBTERRAIN,
  LIST3D_SPCS,
}List3DSpecialType_t;

// ORDERS MUST NOT CHANGE !!
// do special operations in the processing of the node
// eg add to layers, or spawn particles, or do nothing
// the stateifs do a second check, which atm is redundant
// as we could do a precise check in the main list iteration
// children still need stateifs for lights/projectors
typedef enum List3DProcessType_e{
  LIST3D_PROC_NONE,     // lights,projectors
  LIST3D_PROC_PARTICLE_GRP,
  LIST3D_PROC_PARTICLE_EMIT,
  LIST3D_PROC_LEVELMODEL,
  LIST3D_PROC_ALWAYS,     // trail,drawfunc
  LIST3D_PROC_SIMPLE,     // default or parent did all the work
  LIST3D_PROC_SIMPLE_LIGHTS,
  LIST3D_PROC_SIMPLE_PROJ,
  LIST3D_PROC_SIMPLE_PROJ_LIGHTS,
}List3DProcessType_t;
enum List3DProcessShifts_e{
  LIST3D_PROC_START_SIMPLE = 4,
  LIST3D_PROC_SHIFT_LIGHTS = 1,
  LIST3D_PROC_SHIFT_PROJ  = 2,
};
enum List3DCompileFlag_e{
  LIST3D_COMPILE_NOT    = 1,
  LIST3D_COMPILE_BATCHED  = 2,
};



//////////////////////////////////////////////////////////////////////////
// List3DNode

typedef struct List3DVis_s{
  uchar       ignore;
  uchar       inside; // if set, we draw when camera is inside
  uchar       box;
  uchar       ranged;

  lxVector3       omin; // outer
  lxVector3       omax;
  lxVector3       imin; // inner
  lxVector3       imax;
}List3DVis_t;


typedef struct List3DNode_s{
  // 2 DW
  lxClassType     nodeType;   // type of this node
  char        *name;

  // 3 DW
  List3DProcessType_t processType;
  List3DUpdateType_t  updateType;
  List3DSpecialType_t specUpdType;

  // 5 DW
  Reference     reference;
  Reference     linkReference;  // actor/scenenode
  LinkType_t      linkType;   // 0 = s3d, 1 = actor
  union{
    struct LinkObject_s *linkObject;
    struct SceneNode_s  *linkScenenode;
    struct ActorNode_s  *linkActor;
  };
  struct VisObject_s    *linkVisobject;

  // 2 DW
  enum32        viscameras;   // all of wanted cameras it is visible to
  enum32        visflag;    // cameras it wants to be visible to

  // 4 DW
  struct List3DNode_s   *next,*prev;    // used for hierarchy
  struct List3DNode_s   *parent;
  struct List3DNode_s   *childrenListHead;

  //  - 4 DW aligned
  lxMatrix44      finalMatrix;
  lxMatrix44      localMatrix;

  float       *boneMatrix;

  lxVector3       finalPos;   // used for lookat
  lxVector3       renderscale;
  int         update;     // if not set our matrix is already correct
  ulong       drawnFrame;
  ulong       updFrame;

  struct List3DNode_s   *visnext,*visprev;  // used for visobjects
  struct List3DNode_s   *linknext,*linkprev;  // used for visobjects

  uchar       unlinked;
  uchar       novistest;
  ushort        inheritLocks;

  ushort        setID;      // cant be changed
  ushort        compileflag;
  ushort        lookat;
  ushort        lookatlocal;

  DrawEnv_t     enviro;   // node environment
  struct VisLightResult_s *vislightres;
  List3DVis_t     *l3dvis;

  int         numDrawNodes;
  DrawNode_t      *drawNodes; // there can be multiple for meshes

  union{
    struct PText_s    *ptext;
    Modelnstance_t    *minst;
    ParticleEmitter_t *pemitter;
    ParticleGroup_t   *pgroup;
    List3DPrimitive_t *primitive;
    Light_t       *light;
    Trail_t       *trail;
    Projector_t     *proj;
    ShadowModel_t   *smdl;
    LevelModel_t    *lmdl;
    Camera_t      *cam;
    void        *data;
  };

  int           checkRID; // resmanager dependent nodes might need to
  int           checkType;  // update their pointers if resources were reloaded
  void          *checkptr;
} List3DNode_t;



//////////////////////////////////////////////////////////////////////////
// List3DView & Set & Main

typedef struct List3DView_s{
  struct List3DView_s **list;
  struct List3DView_s *next,*prev;

  Reference     reference;

  VIDViewPort_t   viewport;
  RenderBackground_t  background;

  Camera_t      *camera;
  enum32        camvisflag;
  enum32        rcmdflag;

  int         usepolyoffset;
  lxVector2       polyoffset;

  int         layerfill;
  int         depthonly;
  int         isdefault;
  int         drawcamaxis;

  int         maxCommands;
  int         numCommands;
  RenderCmd_t     *commands[LIST3D_VIEW_DEFAULT_COMMANDS+1];
    // must be NULL terminated
    // must be at the end
}List3DView_t;

typedef struct List3DSet_s{
  List3DNode_t    *novistestListHead;   // list of novistest nodes
  List3DNode_t    **visCurrent;
  List3DNode_t    **visibleBufferArray; // volatile array filled by vistest, end is NULL

  List3DNode_t    *nodeListHead;
  int         numNodes;

  Int2Node_t      *psysListHead;
  Int2Node_t      *pcloudListHead;

  Light_t       *sun;
  Light_t       *fxlightListHead;
  Projector_t     *projectorListHead;

  Projector_t     *projLookUp[LIST3D_SET_PROJECTORS];

  List3DView_t    defaultView;
  List3DView_t    *viewListHead;

  List3DBatchBuffer_t *batchListHead;

  int         disabled;
}List3DSet_t;

typedef struct List3D_s{
  void    *visibleBufferStart;      // set by vistest

  Projector_t     **projCurrent;
  Projector_t     **projBufferArray;    // volatile filled at rendertime, set by vistest
  Projector_t     **projBufferArrayEnd;

  int         setdefault;
  int         layerdefault;

  int         maxTotalProjectors;
  int         maxDrawPerLayer;
  int         maxDrawPerLayerMask;
  int         maxLayers;
  int         maxSets;

  List3DSet_t     *drawSets;
}List3D_t;

// Cache/Draw stuff

typedef struct List3DLayerData_s{
  DrawSortNode_t  *sortNodes;
  int       numSortNodes;
  ParticleCloud_t *pcloudListHead;
  ParticleSys_t *psysListHead;
}List3DLayerData_t;

typedef struct List3DDrawData_s{
  List3DNode_t    **drawBufferArray;
  List3DNode_t    **drawCurrent;

  List3DLayerData_t *layerdata;
}List3DDrawData_t;

typedef struct List3DDrawState_s{
  List3DView_t *view;
  List3DSet_t *lset;
  List3DDrawData_t* l3ddraw;

  int     useshaders;
  int     orthoprojection;

  flags32 kickflag;
  flags32 ignoreflag;
  flags32 forceflag;
  flags32 ignoreflags;
  flags32 forceflags;
  booln ignoreprojs;
  booln ignorelights;

  FBObject_t  *fboTargets[FBO_TARGETS];


  booln isdefaultcam;
  int     drawdebug;
  int     firstclear;
  enum32    defaultcambitid;
  Camera_t  *defaultcam;
  Light_t   *defaultsun;
}List3DDrawState_t;



typedef int   (LUX_FASTCALL List3DNode_intfunc)(List3DNode_t *node);

typedef struct List3DNodeIntFunc_s{
    List3DNode_intfunc  *func;
}List3DNodeIntFunc_t;

typedef void  (LUX_FASTCALL List3DNode_voidfunc)(List3DNode_t *node);

typedef struct List3DNodeVoidFunc_s{
  List3DNode_voidfunc *func;
}List3DNodeVoidFunc_t;




// GLOBALS
extern List3D_t g_List3D;
#define List3D_ZsortLayer(i) (i == LIST3D_LAYER_FTB || i == LIST3D_LAYER_BTF)

//////////////////////////////////////////////////////////////////////////
// List3DNode

#define List3DNode_markUpdate(node) node->update = LUX_TRUE

//void List3DNode_free(List3DNode_t *node);
void RList3DNode_free (lxRl3dnode ref);

  // should only be called if you want to manually set matrices
  // then is directly followed by draw call
  // else let List3D and the InterfaceObjects do the job
void List3DNode_update(List3DNode_t *node, const enum32 viscameras);
void List3DLayerNode_draw(DrawNode_t *node);

  // bubbles up the hierarchy
int List3DNode_updateUp_matrix_recursive(List3DNode_t*node);

// uploads
  // returns node that represents query, automatically takes care of children (e.g. meshes within a model)
List3DNode_t* List3DNode_newPrimitive(const char *name, int layer,
              List3DPrimitiveType_t type,const float *size);
  // you must have created Modelnstance yourself and hand it over
  // useful for animation sharing
List3DNode_t* List3DNode_newModelnstance(const char *name, int layer,
              Modelnstance_t* mobj);

  //  auto creates a new Modelnstance
List3DNode_t* List3DNode_newModelRes(const char *name, int layer,
              int resRID, booln boneupd, booln modelupd);

List3DNode_t* List3DNode_newParticleEmitterRes(const char *name, int layer,
              int resRID,const int sortid);

List3DNode_t* List3DNode_newParticleGroupRes(const char *name, int layer,
              int resRID,const int sortid);

List3DNode_t* List3DNode_newLight(const char *name, int layer);

List3DNode_t* List3DNode_newProjector(const char *name, int layer);

List3DNode_t* List3DNode_newTrail(const char *name, int layer,
              ushort length);

List3DNode_t* List3DNode_newText(const char *name, int layer,
              const char *text);

List3DNode_t* List3DNode_newShadowModel(const char *name, int layer,
              const List3DNode_t *target,const List3DNode_t *light, const char *meshnameflag);

List3DNode_t* List3DNode_newLevelModel(const char *name, int layer,
              int resRID, int dimension[3], int numpolys, float margin);

List3DNode_t* List3DNode_newCamera(const char *name, int layer,
              int bitid);

// hierarchy

  // link to another l3dnode, will get automatically drawn if parent is drawn
  // finalM = finalParent * localM
  // if newparent exists, node will unref its interfaces
  // returns on error (setID different)
int  List3DNode_link(List3DNode_t *node, List3DNode_t *newparent);

  // not linked to anything,not drawn (be careful!!)
void List3DNode_unlink(List3DNode_t *node);

  // uses parent's bone before applying offset
  // finalM = finalParent * Bone * localM
int  List3DNode_linkParentBone(List3DNode_t *node, char *bonename);

  // same as if normally linked
void List3DNode_unlinkParentBone(List3DNode_t *node);


// sets

  // clears or sets interfaces
  // also makes unlinked nodes active, if both interfaces are valid
  // as well as handles proper parent handling
  // linktype 0 = s3d, 1 = actor, -1 = unlink -n.. = unlink but no change to updatetype (for l3dchildren)
  // childstate = 0 no change 1 becamechild -1 becameorphan
  // returns TRUE on fail if host already has different setID
int  List3DNode_setIF(List3DNode_t *base, Reference linkReference, LinkType_t linkType, int childstate);

void List3DNode_setVisflag(List3DNode_t *base,enum32 flag);


  // pass -1 to get state
int  List3DNode_useLocal(List3DNode_t *self, int state);
  // pass -1 to get state
int  List3DNode_useManualWorld(List3DNode_t *self, int state);
  // pass -1 to get state
int  List3DNode_useLookAt(List3DNode_t *self,int state);

  // only works for non childrenm -1 to get state
int List3DNode_useNovistest(List3DNode_t *base, int state);

// overwrites previous layer info, also overwrites shader layer infos
// return TRUE on error (changing l3dsets when linked)
int  List3DNode_setLayer(List3DNode_t *base, int layer, int dnode);

void List3DNode_setInheritLocks(List3DNode_t *base, int inh);
  // dnode < 0 sets all dnodes
void List3DNode_setRF(List3DNode_t *base, enum32 renderflag, int state, int dnode);

void  List3DNode_setProjectorFlag(List3DNode_t *base, enum32 projectorflag);

// specific for Model
  // get/set a Model's mesh material, get with -2 passed
int  List3DNode_Model_meshMaterial(List3DNode_t *base, const int mesh, const int resRID);

  // sets if all bones run in world coordinates
void  List3DNode_Model_setWorldBones(List3DNode_t *base, const int state);

// specifc for tbtinst
void  List3DNode_TBT_updatedMaterial(List3DNode_t *base);

//////////////////////////////////////////////////////////////////////////
// List3DView

List3DView_t *List3DView_new(int rcmdbias);
//void  List3DView_free(List3DView_t *view);
void  RList3DView_free(lxRl3dview ref);

void  List3DView_rcmdEmpty(List3DView_t *view);
booln List3DView_rcmdRemove(List3DView_t *view,RenderCmd_t *rcmd, int refpos);
booln List3DView_rcmdAdd(List3DView_t *view, RenderCmd_t *rcmd, booln before, RenderCmd_t *refcmd, int refpos);

  // added as first to the list (if ref NULL), returns TRUE on success
int   List3DView_activate(List3DView_t *view, const int setID, List3DView_t *ref, int after);
void  List3DView_deactivate(List3DView_t *view);

//////////////////////////////////////////////////////////////////////////
// List3DSet

void  List3DSet_updateAll(List3DSet_t *lset);

//////////////////////////////////////////////////////////////////////////
// List3D

// returns NULL when okay, else error string
char* List3DLayerOpCodes_checkString(char *commands);


  // updates cameras and lights
  // sets g_CamLight.cameraListHead
void List3D_updateProps();

void List3D_updateAll();

void List3D_resetVisible();

int  List3D_getDefaultLayer();

  // bitID - currently unused
  // only lights with highes priority and closest valid range will be picked
  // if duration is set the light will be auto removed from active list after given time
void List3D_activateLight(int l3dsetID,Light_t *light, const uchar priority, const float range, const uint duration);
void List3D_deactivateLight(Light_t *light);
void List3D_activateProjector(int l3dsetID,Projector_t *proj,const int id);
void List3D_deactivateProjector(Projector_t *proj);

void List3D_removeCamera(Camera_t *cam);
void List3D_removeLight(Light_t *light);
void List3D_removeSkyBox(SkyBox_t *skybox);
void List3D_removeProjector(Projector_t *proj);

  // RPOOLUSE
void List3D_draw();

  // no RPOOL as no layers are filled
void List3DView_drawDirect(List3DView_t *view, Light_t *sun);

  // 0 noerror, 1 error, -1 warning
int  List3D_fbotest(char *outerror, int outerrorlen, List3DView_t *singleview);

  // initializes the list
void List3D_init(int maxSets, int maxLayers, int maxDrawPerLayer, int maxTotalProjectors);
void List3D_deinit();
void List3D_updateMemPoolRelated();

#endif
