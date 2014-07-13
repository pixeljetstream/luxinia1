// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "fnpublish.h"
#include "../common/common.h"
#include "../scene/actorlist.h"
#include "../common/3dmath.h"
#include "../common/interfaceobjects.h"
#include "../common/reflib.h"
#include "../resource/bone.h"
#include "pubclass_types.h"
#include "../render/gl_list3d.h"
#include "../common/console.h"

#include <stdio.h>



///////////////////////////////////////////////////////////////////////////////
// Matrix44 interface functions
typedef struct BoneMI_s
{
  // - 4 DW aligned
  lxMatrix44 offsetmatrix;
    Reference l3dRef;
    float *matrixptr;
    int useoffset;
}
BoneMI_t;

static float* BoneMI_getElements  (void *data, float *f)
{
    BoneMI_t *obj = (BoneMI_t*) data;
    List3DNode_t *l3d;

    if (!Reference_get(obj->l3dRef,l3d))
        return NULL;

    lxMatrix44Multiply(f,l3d->finalMatrix,obj->matrixptr);
    if (obj->useoffset)
        lxMatrix44Multiply1(f,obj->offsetmatrix);

    return f;
}
static void BoneMI_setElements  (void *data, float *f)
{}
static void BoneMI_fnFree   (void *data)
{
    genfreeSIMD(data,sizeof(BoneMI_t));
}

static Matrix44Interface_t* BoneMI_new (Reference l3dRef, float *matrix,
                                      float *offsetmatrix)
{
    static Matrix44InterfaceDef_t def =
        {
            BoneMI_getElements, BoneMI_setElements,
            BoneMI_fnFree
        };

  BoneMI_t *self = genzallocSIMD(sizeof(BoneMI_t));

  LUX_SIMDASSERT((size_t)((BoneMI_t*)self)->offsetmatrix  % 16 == 0);

    self->l3dRef = l3dRef;
    self->matrixptr = matrix;
    self->useoffset = offsetmatrix!=NULL;
    if (offsetmatrix)
        lxMatrix44Copy(self->offsetmatrix,offsetmatrix);

    return Matrix44Interface_new(&def,(void*)self);
}

////////////////////////////////////////////////////////////////////////////////

static ActorNode_t *PubActorNode_getFromArg(PState state,int n)
{
    Reference ref;
    ActorNode_t *act;

    if (FunctionPublish_getNArg(state,n,LUXI_CLASS_ACTORNODE,(void*)&ref) != 1 ||
      !Reference_get(ref,act))
        return NULL;
    return act;
}

static int PubActorNode_new (PState state, PubFunction_t *f, int n)
{
    char *name;
    ActorNode_t *act;
    booln drawable = LUX_TRUE;
  lxVector3 pos = {0,0,0};

    if (FunctionPublish_getNArg(state,0,LUXI_CLASS_STRING,(void*)&name)!=1)
        return FunctionPublish_returnError(state,"1 string required");
    if ((n==2 || n==5) && !FunctionPublish_getNArg(state,1,LUXI_CLASS_BOOLEAN,(void*)&drawable))
        return FunctionPublish_returnError(state,"1 boolean required");
    if ((n==4 || n==5) && 3!=FunctionPublish_getArgOffset(state,1+(n==5),3,FNPUB_TOVECTOR3(pos)))
        return FunctionPublish_returnError(state,"3 floats required");

    act = ActorNode_new(name,drawable,pos);
  Reference_makeVolatile(act->link.reference);
    return FunctionPublish_returnType(state,LUXI_CLASS_ACTORNODE,REF2VOID(act->link.reference));
}

static int PubActorNode_link(PState state, PubFunction_t *f, int n)
{
    ActorNode_t *self;
    MatrixLinkObject_t *body;
    Reference ref;

    self = PubActorNode_getFromArg(state,0);
    if (self == NULL)
        return FunctionPublish_returnError(state,"Arg 1 must be valid actornode");

    if (n==1 || (!FunctionPublish_getNArg(state,1,LUXI_CLASS_PGEOM,(void*)&ref) && !FunctionPublish_getNArg(state,1,LUXI_CLASS_PBODY,(void*)&ref)))
    {
        ActorNode_setMatrixIF(self,NULL);
        return 0;
    }
    if (!Reference_get(ref,body))
        return FunctionPublish_returnError(state,"Given dgeom/dbody is not valid");

    ActorNode_setMatrixIF(self,body->matrixIF);

    return 0;
}

static int PubActorNode_linkBone(PState state, PubFunction_t *f, int n)
{
    ActorNode_t *self;
    Reference ref;
    List3DNode_t *l3d;
    BoneSysUpdate_t *boneupd;
    const char *name;
    Matrix44Interface_t *mif;
    int index;
    float *m;
    Reference refmatrix;

    if (n<3) return FunctionPublish_returnError(state,"actornode, l3dmodel and name of bone required");
    self = PubActorNode_getFromArg(state,0);
    if (self == NULL) return FunctionPublish_returnError(state,"Arg 1 must be valid actornode");
    if (!FunctionPublish_getNArg(state,1,LUXI_CLASS_L3D_MODEL,(void*)&ref) || !Reference_get(ref,l3d) ||
            !FunctionPublish_getNArg(state,2,LUXI_CLASS_STRING,(void*)&name))
        return FunctionPublish_returnError(state,"l3dmodel and string required");

    boneupd = l3d->minst->boneupd;
    if (!boneupd) return FunctionPublish_returnError(state,"l3dmodel doesnt allow boneanimation");

    index = BoneSys_searchBone(&(ResData_getModel(l3d->minst->modelRID)->bonesys),name);
    if (index==-1)
        return FunctionPublish_returnError(state,"no such bone");

    m=NULL;
    if (n == 4 && (!FunctionPublish_getNArg(state,3,LUXI_CLASS_MATRIX44,(void*)&refmatrix)
                   || !Reference_get(refmatrix,m)))
        return FunctionPublish_returnError(state,"matrix4x4 required");


    mif = BoneMI_new(ref,&boneupd->bonesAbs[index*16],m);
    mif->refCount = 0;
    ActorNode_setMatrixIF(self,mif);

    return 0;
}

static int PubActorNode_name(PState state, PubFunction_t *f, int n)
{
    ActorNode_t *self;

    self = PubActorNode_getFromArg(state,0);
    if (self == NULL)
        return FunctionPublish_returnError(state,"Arg 1 must be valid actornode");

    return FunctionPublish_returnString(state,self->name);
}

static int PubActorNode_getCount (PState state, PubFunction_t *f, int n)
{
  return FunctionPublish_returnInt(state,ActorNode_getCount ());
}
static int PubActorNode_getNext (PState state, PubFunction_t *f, int n)
{
  ActorNode_t *self = PubActorNode_getFromArg(state,0);
    if (self == NULL)
        return FunctionPublish_returnError(state,"Arg 1 must be valid actornode");

  self = ActorNode_getNext (self);

  return FunctionPublish_returnType(state,LUXI_CLASS_ACTORNODE,*(void**)&self->link.reference);
}
static int PubActorNode_getPrev (PState state, PubFunction_t *f, int n)
{
  ActorNode_t *self = PubActorNode_getFromArg(state,0);
    if (self == NULL)
        return FunctionPublish_returnError(state,"Arg 1 must be valid actornode");

  self = ActorNode_getPrev (self);

  return FunctionPublish_returnType(state,LUXI_CLASS_ACTORNODE,*(void**)&self->link.reference);
}
static int PubActorNode_getFromIndex (PState state, PubFunction_t *f, int n)
{
  ActorNode_t *self;
    int idx;

  FNPUB_CHECKOUT(state,n,0,LUXI_CLASS_INT,idx)

  self = ActorNode_getFromIndex (idx);
  if (self==NULL) return 0;

  return FunctionPublish_returnType(state,LUXI_CLASS_ACTORNODE,*(void**)&self->link.reference);
}

static int PubActorNode_posrot (PState state, PubFunction_t *f, int n)
{ // rotation and position via upvalues
    static lxMatrix44SIMD matrix;

  float *m;
    ActorNode_t *self = PubActorNode_getFromArg(state,0);
    float vec[4];
    float *x,*y,*z;

    if (self == NULL)
        return FunctionPublish_returnError(state,"Arg 1 must be valid actornode");

    m = Matrix44Interface_getElements(ActorNode_getMatrixIF(self),matrix);
    switch (n)
    {
    case 1: // get position / rotation :)
        switch ((int)f->upvalue)
        {
        default:
            return FunctionPublish_setRet(state,3,FNPUB_FROMMATRIXPOS(m));
        case ROT_RAD:
            lxMatrix44ToEulerZYX(m,vec);
            return FunctionPublish_setRet(state,3,FNPUB_FROMVECTOR3(vec));
        case ROT_DEG:
            lxMatrix44ToEulerZYX(m,vec);
            lxVector3Rad2Deg(vec,vec);
            return FunctionPublish_setRet(state,3,FNPUB_FROMVECTOR3(vec));
    case ROT_QUAT:
      lxQuatFromMatrix(vec,m);
      return FunctionPublish_setRet(state,4,FNPUB_FROMVECTOR4(vec));
        case ROT_AXIS:
            x = y = z = m;
            y+=4;
            z+=8;
            return FunctionPublish_setRet(state,9, FNPUB_FROMVECTOR3(x)
                                          ,FNPUB_FROMVECTOR3(y)
                                          ,FNPUB_FROMVECTOR3(z));
        }
    case 4: // set position / rotation
        if (FunctionPublish_getArgOffset(state,1,3,FNPUB_TOVECTOR3(vec))<3)
            return FunctionPublish_returnError(state,"1 actornode 3 floats required");
        switch ((int)f->upvalue)
        {
        default:
            m[12] = vec[0];
            m[13] = vec[1];
            m[14] = vec[2];
            break;
        case ROT_RAD:
            lxMatrix44FromEulerZYXFast(m,vec);
            break;
        case ROT_DEG:
            lxMatrix44FromEulerZYXdeg(m,vec);
            break;
        }
    Matrix44Interface_setElements(ActorNode_getMatrixIF(self),m);
    return 0;
  case 5:
    if ((int)f->upvalue != ROT_QUAT || FunctionPublish_getArgOffset(state,1,4,FNPUB_TOVECTOR4(vec))<4)
      return FunctionPublish_returnError(state,"1 actornode 4 floats required");
    lxQuatToMatrix(vec,m);
    Matrix44Interface_setElements(ActorNode_getMatrixIF(self),m);
    return 0;
    case 10:  // set rot axis
        x = y = z = m;
        y+=4;
        z+=8;
        if (FunctionPublish_getArgOffset(state,1,9,FNPUB_TOVECTOR3(x),
                                         FNPUB_TOVECTOR3(y),
                                         FNPUB_TOVECTOR3(z)) != 9)
            return FunctionPublish_returnError(state,"1 actornode 9 floats required");
        Matrix44Interface_setElements(ActorNode_getMatrixIF(self),m);
        return 0;
    }

    return FunctionPublish_returnError(state,"requires 1,4 or 10 arguments");
}




static void orient(lxMatrix44 absmatrix, lxVector3 toTarget, lxVector3 toEffector,
                   int axis,int limit,float minAngle[3],float maxAngle[3])
{
    lxVector3 side, up;
    int i;
    lxVector3 absaxis[3];
    lxVector3 anglesNew;
    lxVector3 anglesOld;
    float fNumer;
    float fDenom;
    float fDelta;

    if (lxVector3Dot(toEffector,toTarget) > 0.99999)
        return;

    for (i = 0; i < 3; i++)
        lxVector3Copy(absaxis[i],&absmatrix[i*4]);  // column

    for (i = 0; i < 3; i++)
    {
        if (axis & (1<<i))
        {
            lxVector3Cross(side,absaxis[i],toEffector);
            lxVector3Cross(up,absaxis[i],side);
            fNumer = lxVector3Dot(toTarget,side);
            fDenom = lxVector3Dot(toTarget,up);

            if ( fNumer*fNumer + fDenom*fDenom <= LUX_FLOAT_EPSILON )
                continue;

            lxMatrix44ToEulerZYX(absmatrix,anglesOld);
            // desired angle to rotate about axis(i)

            fDelta = atan2(fNumer,fDenom);

            anglesNew[i] = anglesOld[i] - fDelta;

            if (limit & (1<<i))
                anglesNew[i] = LUX_MAX(minAngle[i],LUX_MAX(maxAngle[i],anglesNew[i]));

            anglesOld[i] = anglesNew[i];
            lxMatrix44FromEulerZYXFast(absmatrix,anglesOld);
        }
    }
}


static int PubActorNode_setLookat (PState state, PubFunction_t *f, int n)
{ // let the matrix lookat at a point :)
  static lxMatrix44SIMD matrix;

    ActorNode_t *self = PubActorNode_getFromArg(state,0);
    float *mat;
    lxVector3 to,up;
    int axis;

    if (self == NULL)
        return FunctionPublish_returnError(state,"1 actornode 6 floats required");



    axis = 1;
    if (n < 7 || FunctionPublish_getArgOffset(state,1,7,FNPUB_TOVECTOR3(to),FNPUB_TOVECTOR3(up),LUXI_CLASS_INT,(void*)&axis) < 6
            || axis > 2 || axis < 0)
        return FunctionPublish_returnError(state," 1 actornode 6 floats [1 int 0-2] required");

    mat = Matrix44Interface_getElements(ActorNode_getMatrixIF(self),matrix);

    lxVector3Sub(to,to,&mat[12]);
    lxVector3Normalized(to);
    lxMatrix44Orient(mat,to,up,axis);

    Matrix44Interface_setElements(ActorNode_getMatrixIF(self),mat);
    return 0;
}


static int PubActorNode_matrix(PState state, PubFunction_t *f, int n)
{
    float *m;
    Reference ref,matrix;
    ActorNode_t *self;

    switch (n)
    {
    default:
        return FunctionPublish_returnError(state,"use: (actornode self,[matrix4x4 m])");
  case 1:
    if (1>FunctionPublish_getArg(state,1,LUXI_CLASS_ACTORNODE,(void*)&ref) || !Reference_get(ref,self))
      return FunctionPublish_returnError(state,"1 actornode required");
    m = Matrix44Interface_getElements(self->link.matrixIF,self->link.matrix);
    return PubMatrix4x4_return (state,m);
  case 2:
    if (2>FunctionPublish_getArg(state,2,LUXI_CLASS_ACTORNODE,(void*)&ref,LUXI_CLASS_MATRIX44,(void*)&matrix) ||
      !Reference_get(ref,self) || !Reference_get(matrix,m))
      return FunctionPublish_returnError(state,"1 actornode 1 matrix4x4 required");
    Matrix44Interface_setElements(self->link.matrixIF,m);
    return 0;
    }

    return 0;
}

static int PubActorNode_delete(PState state, PubFunction_t *f, int n)
{
    ActorNode_t *self;
    Reference ref;

    if (n < 1 || FunctionPublish_getNArg(state,0,LUXI_CLASS_ACTORNODE,(void*)&ref) != 1 || !Reference_get(ref,self))
        return FunctionPublish_returnError(state,"1 actornode required");
    Reference_free(ref);//RActorNode_free(ref);
    return 0;
}

static int PubActorNode_vistest(PState state, PubFunction_t *f, int n)
{
    ActorNode_t *self;
    Reference ref;
    lxVector3 min;
  lxVector3 max;

    if (n < 1 || FunctionPublish_getNArg(state,0,LUXI_CLASS_ACTORNODE,(void*)&ref) != 1 || !Reference_get(ref,self)
            || !self->link.visobject)
        return FunctionPublish_returnError(state,"1 drawable actornode required");

    if (n==1)
    {
        return FunctionPublish_setRet(state,6,FNPUB_FROMVECTOR3(self->link.visobject->bbox.min),FNPUB_FROMVECTOR3(self->link.visobject->bbox.max));
    }
    else if (6!=FunctionPublish_getArgOffset(state,1,6,FNPUB_TOVECTOR3(min),FNPUB_TOVECTOR3(max)))
        return FunctionPublish_returnError(state,"1 actornode 6 floats required");
    VisObject_setBBoxForce(self->link.visobject,min,max);
    return 0;
}


void PubClass_ActorNode_init()
{
  FunctionPublish_initClass(LUXI_CLASS_ACTORNODE,"actornode",
    "The actorlist is a double linked ring list containing actornodes. "
    "The purpose of actors is to provide positional and movement "
    "information as dynamic spatialnodes. \n\n"
    "Actors are normaly used for objects that are moved around "
    "and can be directly linked with dbody objects, which are "
    "used in physical simulations.\n\n"
    "!!Example\n"
    "[img actornode.info.png align='left']"
    "A simple visible box, attached to an actor. The visual appearance can be "
    "manipulated with the functions listed in the l3dnode or l3dprimitive classes.\n\n"
    " -- connect a l3dprimitive with an actor and use another actor as camera\n"
    " UtilFunctions.simplerenderqueue()"
    " actor = actornode.new('actor')\n"
    " actor.l3d = l3dprimitive.newbox('box',1,1,1)\n"
    " actor.l3d:linkinterface(actor) -- connect the box with the actor\n"
    " cam = actornode.new('camera',5,4,3)\n"
    " l3dcamera.default():linkinterface(cam) -- use the cam actornode as camera\n"
    " cam:lookat(0,0,0, 0,0,1) -- look at 0,0,0 and up is 0,0,1\n",
 NULL,LUX_TRUE);
  FunctionPublish_setParent(LUXI_CLASS_ACTORNODE,LUXI_CLASS_SPATIALNODE);

    FunctionPublish_addFunction(LUXI_CLASS_ACTORNODE,PubActorNode_new,NULL,"new",
                                "(actornode):(string name,[boolean drawable],[float x,y,z]) - creates a new actornode (default is drawable at 0,0,0) with the given name");
    //FunctionPublish_addFunction(LUXI_CLASS_ACTORNODE,PubActorNode_deleteall,NULL,"deleteall",
    //  "():() - deletes all actornodes in the system");
    FunctionPublish_addFunction(LUXI_CLASS_ACTORNODE,PubActorNode_delete,NULL,"delete",
                                "():(actornode) - deletes the actornode");
    FunctionPublish_addFunction(LUXI_CLASS_ACTORNODE,PubActorNode_link,NULL,"link",
                                "():(actornode, [dgeom/dbody/other]) - links actornode to given body or \
                                unlinks it if other type is given (i.e. boolean) or if nothing is given. An actor can't tell you \
                                the body that it is linked to.");
    FunctionPublish_addFunction(LUXI_CLASS_ACTORNODE,PubActorNode_linkBone,NULL,"linkbone",
                                "():(actornode self, l3dmodel model, string bone,[matrix4x4 offset]) - Links the actor to a certain bone. \
                                The position of the actor can not be changed anymore when it is linked. The matrix of a bone \
                                is only correct if the l3dnode is visible! Otherwise the behaviour is undefined.");
    FunctionPublish_addFunction(LUXI_CLASS_ACTORNODE,PubActorNode_name,NULL,"name",
                                "[string name] name (ActorNode self) - gets the name");
    FunctionPublish_addFunction(LUXI_CLASS_ACTORNODE,PubActorNode_posrot,(void*)POS,"pos",
                                "([float x,y,z]):(actornode self, [float x,y,z]) - sets/gets position");
    FunctionPublish_addFunction(LUXI_CLASS_ACTORNODE,PubActorNode_posrot,(void*)ROT_RAD,"rotrad",
                                "([float x,y,z]):(actornode self, [float x,y,z]) - sets/gets rotation in radians");
    FunctionPublish_addFunction(LUXI_CLASS_ACTORNODE,PubActorNode_posrot,(void*)ROT_DEG,"rotdeg",
                                "([float x,y,z]):(actornode self, [float x,y,z]) - sets/gets rotation in degrees");
    FunctionPublish_addFunction(LUXI_CLASS_ACTORNODE,PubActorNode_posrot,(void*)ROT_AXIS,"rotaxis",
                                "([float Xx,Xy,Xz, Yx,Yy,Yz, Zx,Zy,Zz]):(ActorNode self,[float Xx,Xy,Xz, Yx,Yy,Yz, Zx,Zy,Zz]) - returns or sets rotation axis, make sure they make a orthogonal basis.");
  FunctionPublish_addFunction(LUXI_CLASS_ACTORNODE,PubActorNode_posrot,(void*)ROT_QUAT,"rotquat",
                "([float x,y,z,w]):(actornode self, [float x,y,z,w]) - sets/gets rotation as quaternion");
    FunctionPublish_addFunction(LUXI_CLASS_ACTORNODE,PubActorNode_matrix,NULL,"matrix",
                                "([matrix4x4 m]):(actornode self, [matrix4x4 m]) - sets/gets matrix ");
    FunctionPublish_addFunction(LUXI_CLASS_ACTORNODE,PubActorNode_vistest,(void*)0,"vistestbbox",
                                "([float minX,minY,minZ,maxX,maxY,maxZ]):(actornode self, [float minX,minY,minZ,maxX,maxY,maxZ]) - sets/gets visibility testing boundingbox when actornode is drawable ");
    FunctionPublish_addFunction(LUXI_CLASS_ACTORNODE,PubActorNode_setLookat,NULL,"lookat",
                                "():(actornode self, float tox,toy,toz,upx,upy,upz,[int axis 0-2]) - rotates actornode to look at a point along defined axis. axis can be 0:x, 1:y (defualt) 2:z.");
    FunctionPublish_addFunction(LUXI_CLASS_ACTORNODE,PubActorNode_getFromIndex,NULL,"fromindex",
                  "([actornode]):(int index) - returns the actornode at the given index. Returns nothing if the index is out of bounds. Index may be negative and will count back from the end of the list then.");
    FunctionPublish_addFunction(LUXI_CLASS_ACTORNODE,PubActorNode_getNext,NULL,"next",
                "(actornode):(actornode) - returns the next actornode in the list (which can be the same if only one actornode is present).");
  FunctionPublish_addFunction(LUXI_CLASS_ACTORNODE,PubActorNode_getPrev,NULL,"prev",
                "(actornode):(actornode) - returns the prev actornode in the list (which can be the same if only one actornode is present).");
  FunctionPublish_addFunction(LUXI_CLASS_ACTORNODE,PubActorNode_getCount,NULL,"getcount",
                "(int count):() - returns the current number of actornodes in the system.");
}
