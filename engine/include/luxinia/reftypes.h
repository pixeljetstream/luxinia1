// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __LUXINIA_REFTYPES_H__
#define __LUXINIA_REFTYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum lxClassType_e {
	LUXI_CLASS_ANY = -1,				//
	LUXI_CLASS_ERROR = 0,				//
	LUXI_CLASS_BOOLEAN,					//	
	LUXI_CLASS_INT,						//	
	LUXI_CLASS_FLOAT,					//	
	LUXI_CLASS_DOUBLE,					//	
	LUXI_CLASS_STRING,					//	null terminated ANSI 8-bit char string
	LUXI_CLASS_BINSTRING,				//  binary strings
	LUXI_CLASS_CHAR,					//	single 8-bit char
	LUXI_CLASS_POINTER,					//  
	LUXI_CLASS_FUNCTION,				//	

	// WRAPPER with PubArray_t (order sensitive!)
	LUXI_CLASS_ARRAY_POINTER,			//	void**
	LUXI_CLASS_ARRAY_STRING,			//	char**
	LUXI_CLASS_ARRAY_INT,				//	int*
	LUXI_CLASS_ARRAY_FLOAT,				//	float*
	LUXI_CLASS_ARRAY_BOOLEAN,			//	bool8*

	// INTERNAL
	LUXI_CLASS_CLASS,					//	static, class definitions
	
	LUXI_CLASS_REFLIB,					//  static, reference library functions
	LUXI_CLASS_FILELOADER,				//	static

	// MATH
	LUXI_CLASS_MATH,					//	group
	LUXI_CLASS_MATRIX44,				//	user lxRmatrix4x4
										//
	LUXI_CLASS_STATICARRAY,				//	group
	LUXI_CLASS_INTARRAY,				//	user lxRintarray
	LUXI_CLASS_FLOATARRAY,				//	user lxRfloatarray
										//
	LUXI_CLASS_SCALARARRAY,				//	user lxRscalararray (supercedes staticarray and subclasses)
	LUXI_CLASS_SCALARTYPE,				//	user enum
	LUXI_CLASS_SCALAROP,				//	user enum
	
	// RENDERINTERFACE
	LUXI_CLASS_RENDERINTERFACE,			//	group
										//
	LUXI_CLASS_TECHNIQUE,				//	user enum
	LUXI_CLASS_VERTEXTYPE,				//	user enum
	LUXI_CLASS_MESHTYPE,				//	user enum
	LUXI_CLASS_TEXTYPE,					//	user enum
	LUXI_CLASS_TEXDATATYPE,				//	user enum
	LUXI_CLASS_BLENDMODE,				//	user enum
	LUXI_CLASS_OPERATIONMODE,			//	user enum
	LUXI_CLASS_COMPAREMODE,				//	user enum
	LUXI_CLASS_LOGICMODE,				//	user enum
	LUXI_CLASS_PRIMITIVETYPE,			//	user enum
	LUXI_CLASS_BUFFERTYPE,				//	user enum
	LUXI_CLASS_BUFFERHINT,				//	user enum
	LUXI_CLASS_BUFFERMAPPING,			//	user enum

										//
	LUXI_CLASS_TEXCOMBINER,				//	group
	LUXI_CLASS_TEXCOMBINER_CFUNC,		//	user enum
	LUXI_CLASS_TEXCOMBINER_AFUNC,		//	user enum
	LUXI_CLASS_TEXCOMBINER_SRC,			//	user enum
	LUXI_CLASS_TEXCOMBINER_OP,			//	user enum
										//
	LUXI_CLASS_RENDERFLAG,				//	interface
	LUXI_CLASS_RENDERSURF,				//	interface
	LUXI_CLASS_RENDERSTENCIL,			//	interface
	LUXI_CLASS_MATSURF,					//	group/interface
										//
										// order IMPORTANT !! (gl_render sizeof array)
	LUXI_CLASS_RCMD,					//	user lxRrcmd
	LUXI_CLASS_RCMD_CLEAR,				//	user lxRrcmdclear
	LUXI_CLASS_RCMD_STENCIL,			//	user lxRrcmdstencil
	LUXI_CLASS_RCMD_DEPTH,				//	user lxRrcmddepth
	LUXI_CLASS_RCMD_LOGICOP,			//	user lxRrcmdlogicop
	LUXI_CLASS_RCMD_FORCEFLAG,			//	user lxRrcmdforceflag
	LUXI_CLASS_RCMD_IGNOREFLAG,			//	user lxRrcmdignoreflag
	LUXI_CLASS_RCMD_BASESHADERS,		//	user lxRrcmdshaders
	LUXI_CLASS_RCMD_BASESHADERS_OFF,	//	user lxRrcmdshadersoff
	LUXI_CLASS_RCMD_IGNOREEXTENDED,		//	user lxRrcmdignore
	LUXI_CLASS_RCMD_COPYTEX,			//	user lxRrcmdcopytex
	LUXI_CLASS_RCMD_DRAWMESH,			//	user lxRrcmddrawmesh
	LUXI_CLASS_RCMD_DRAWBACKGROUND,		//	user lxRrcmddrawbg
	LUXI_CLASS_RCMD_DRAWDEBUG,			//	user lxRrcmddrawdbg
	LUXI_CLASS_RCMD_DRAWLAYER,			//	user lxRrcmddrawlayer
	LUXI_CLASS_RCMD_DRAWPARTICLES,		//	user lxRrcmddrawprt
	LUXI_CLASS_RCMD_DRAWL2D,			//	user lxRrcmddrawl2d
	LUXI_CLASS_RCMD_DRAWL3D,			//	user lxRrcmddrawl3d
	LUXI_CLASS_RCMD_GENMIPMAPS,			//	user lxRrcmdmipmaps
	LUXI_CLASS_RCMD_READPIXELS,			//	user lxRrcmdreadpixels
	LUXI_CLASS_RCMD_R2VB,				//	user lxRrcmdr2vb
	LUXI_CLASS_RCMD_UPDATETEX,			//	user lxRrcmdupdatetex
	LUXI_CLASS_RCMD_FNCALL,				//	user lxRrcmdcall

										// all RCMDs >= RCMD_FBO will be used in FBO_CHECK
	LUXI_CLASS_RCMD_FBO_BIND,			//	user lxRrcmdfbobind		
	LUXI_CLASS_RCMD_FBO_OFF,			//	user lxRrcmdfbooff
	LUXI_CLASS_RCMD_FBO_TEXASSIGN,		//	user lxRrcmdfbotex
	LUXI_CLASS_RCMD_FBO_RBASSIGN,		//	user lxRrcmdfborb
	LUXI_CLASS_RCMD_FBO_DRAWBUFFERS,	//	user lxRrcmdfbodrawto
	LUXI_CLASS_RCMD_FBO_READBUFFER,		//	user lxRrcmdfboreadfrom
	LUXI_CLASS_RCMD_BUFFER_BLIT,		//	user lxRrcmdfboblit
										//
	LUXI_CLASS_INDEXARRAY,				//	interface
	LUXI_CLASS_VERTEXARRAY,				//	interface
	LUXI_CLASS_MATOBJECT,				//	interface
										//
	LUXI_CLASS_MATAUTOCONTROL,			//	user lxRmatautocontrol
	LUXI_CLASS_RENDERBUFFER,			//	user lxRrenderbuffer
	LUXI_CLASS_RENDERFBO,				//	user lxRrenderfbo
	LUXI_CLASS_VIDBUFFER,				//	user lxRvidbuffer
	LUXI_CLASS_RENDERMESH,				//	user lxRrendermesh
	LUXI_CLASS_CGPARAM,					//	user ptr (CGparameter)



	// OTHER/SYSTEM
	LUXI_CLASS_CONSOLE,					//	static
	LUXI_CLASS_RESDATA,					//	static
	LUXI_CLASS_RESCHUNK,				//	user ptr (ResChunk_t*)
	LUXI_CLASS_WINDOW,					//	static
	LUXI_CLASS_SYSTEM,					//	static
	LUXI_CLASS_INPUT,					//	static
	LUXI_CLASS_CAPABILITY,				//	static
	LUXI_CLASS_PGRAPH,					//	static / user int
	LUXI_CLASS_RENDERDEBUG,				//	static
	LUXI_CLASS_RENDERSYS,				//	static
	LUXI_CLASS_VISTEST,					//	static

	// SOUND
	LUXI_CLASS_SOUNDNODE,				//	user lxRsoundnode
	LUXI_CLASS_SOUNDLISTENER,			//  static
	LUXI_CLASS_SOUNDCAPTURE,			//  static
	LUXI_CLASS_MUSIC,					//  static

	// SPATIAL
	LUXI_CLASS_SPATIALNODE,				//	group
	LUXI_CLASS_SCENENODE,				//	user lxRactornode
	LUXI_CLASS_ACTORNODE,				//	user lxRscenenode

	// RESOURCES
	LUXI_CLASS_RESOURCE,				//	group
	LUXI_CLASS_SOUND,					//	user lxRIDsound
	LUXI_CLASS_GPUPROG,					//	user lxRIDgpuprog
	LUXI_CLASS_ANIMATION,				//	user lxRIDanimation
	LUXI_CLASS_TEXTURE,					//	user lxRIDtexture
	LUXI_CLASS_MODEL,					//	user lxRIDmodel
	LUXI_CLASS_PARTICLECLOUD,			//	user lxRIDprtcloud
	LUXI_CLASS_MATERIAL,				//	user lxRIDmaterial
	LUXI_CLASS_SHADER,					//	user lxRIDshader
	LUXI_CLASS_PARTICLESYS,				//	user lxRIDprtsys
										//
	LUXI_CLASS_RESSUBID,				//	group (16 bit hostid, 16 bit subid)
	LUXI_CLASS_SHADERPARAMID,			//	user int
	LUXI_CLASS_MATCONTROLID,			//	user int
	LUXI_CLASS_MATTEXCONTROLID,			//	user int
	LUXI_CLASS_MATSHDCONTROLID,			//	user int
	LUXI_CLASS_PARTICLEFORCEID,			//	user int
	LUXI_CLASS_MESHID,					//	user int
	LUXI_CLASS_SKINID,					//	user int
	LUXI_CLASS_BONEID,					//	user int
	LUXI_CLASS_TRACKID,					//	user int

	LUXI_CLASS_PARTICLE,				//	user ptr



	// list3d
	LUXI_CLASS_L3D_LIST,				//	group
	LUXI_CLASS_L3D_SET,					//	user int
	LUXI_CLASS_L3D_LAYERID,				//	user int
	LUXI_CLASS_L3D_BATCHBUFFER,			//	user lxRl3dbatchbuffer
										//
	LUXI_CLASS_L3D_NODE,				//	user lxRl3dnode
	LUXI_CLASS_L3D_TEXT,				//	user lxRl3dtext
	LUXI_CLASS_L3D_PROJECTOR,			//	user lxRl3dprojector
	LUXI_CLASS_L3D_LIGHT,				//	user lxRl3dlight
	LUXI_CLASS_L3D_PGROUP,				//	user lxRl3dpgroup
	LUXI_CLASS_L3D_PEMITTER,			//	user lxRl3dpemitter
	LUXI_CLASS_L3D_MODEL,				//	user lxRl3dmodel
	LUXI_CLASS_L3D_SHADOWMODEL,			//	user lxRl3dshadowmodel
	LUXI_CLASS_L3D_LEVELMODEL,			//	user lxRl3dlevelmodel
	LUXI_CLASS_L3D_PRIMITIVE,			//	user lxRl3dprimitive
	LUXI_CLASS_L3D_TRAIL,				//	user lxRl3dtrail
	LUXI_CLASS_L3D_CAMERA,				//	user lxRl3dcamera
	
	LUXI_CLASS_L3D_VIEW,				//	user lxRl3dview
	LUXI_CLASS_FRUSTUMOBJECT,			//  user lxRfrustumobject
	LUXI_CLASS_SKYBOX,					//	user lxRskybox
	LUXI_CLASS_TRAILPOINT,				//	user ptr
	LUXI_CLASS_BONECONTROL,				//	user lxRbonecontrol
	LUXI_CLASS_MORPHCONTROL,			//	user lxRmorphcontrol
	

	// list2d
	LUXI_CLASS_L2D_LIST,				//	group
	LUXI_CLASS_L2D_NODE,				//	user lxRl2dnode
	LUXI_CLASS_L2D_FLAG,				//	user lxRl2dflag
	LUXI_CLASS_L2D_L3DLINK,				//	user lxRl2dnode3d
	LUXI_CLASS_L2D_IMAGE,				//	user lxRl2dimage
	LUXI_CLASS_L2D_TEXT,				//	user lxRl2dtext
	LUXI_CLASS_L2D_L3DVIEW,				//	user lxRl2dview3d

	// STANDALONE
	LUXI_CLASS_FONTSET,					//	user lxRfontset


	// LUA related
	LUXI_CLASS_LUACORE,
	LUXI_CLASS_LUAFUNCTION,

	LUXI_CLASS_TEST,	// testing purpose class, probably useful for unit tests
						// only for debug compile

	// ODE
	LUXI_CLASS_ODE,						//	group
	LUXI_CLASS_PCOLLRESULT,				//	collision result
	LUXI_CLASS_PWORLD,					//	group
	LUXI_CLASS_PCOLLIDER,				//  user ref - all following
	LUXI_CLASS_PJOINT,					//
	LUXI_CLASS_PJOINTGROUP,				//
	LUXI_CLASS_PSPACE,					//
	LUXI_CLASS_PSPACESIMPLE,			//
	LUXI_CLASS_PSPACEQUAD,				//
	LUXI_CLASS_PSPACEHASH,				//
	LUXI_CLASS_PGEOM,					//
	LUXI_CLASS_PBODY,					//
	LUXI_CLASS_PGEOMSPHERE,				//
	LUXI_CLASS_PGEOMBOX,				//
	LUXI_CLASS_PGEOMCUBE,				//
	LUXI_CLASS_PGEOMCYLINDER,			//
	LUXI_CLASS_PGEOMCONE,				//
	LUXI_CLASS_PGEOMTERRAIN,			//
	LUXI_CLASS_PGEOMTERRAINDATA,		//
	LUXI_CLASS_PGEOMCCYLINDER,			//
	LUXI_CLASS_PGEOMRAY,				//
	LUXI_CLASS_PGEOMPLANE,				//
	LUXI_CLASS_PGEOMTRANSFORM,			//
	LUXI_CLASS_PGEOMTRIMESH,			//
	LUXI_CLASS_PGEOMTRIMESHDATA,		//
	LUXI_CLASS_PJOINTBALL,				//
	LUXI_CLASS_PJOINTHINGE,				//
	LUXI_CLASS_PJOINTSLIDER,			//
	LUXI_CLASS_PJOINTCONTACT,			//
	LUXI_CLASS_PJOINTUNIVERSAL,			//
	LUXI_CLASS_PJOINTHINGE2,			//
	LUXI_CLASS_PJOINTFIXED,				//
	LUXI_CLASS_PJOINTAMOTOR,			//

	LUXI_CLASS_COUNT,


	LUXI_CLASS_MAX = 256,
	LUXI_CLASS_FORCE_DWORD = 0x7FFFFFFF,
} lxClassType;

#ifdef __cplusplus
}
#endif

#endif

