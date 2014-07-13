// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __LUXINIA_REFCLASSES_H__
#define __LUXINIA_REFCLASSES_H__

#ifdef __cplusplus
extern "C" {
#endif

	#include "reference.h"

	// Math
	typedef lxRef	lxRmatrix4x4;
	typedef lxRef	lxRintarray;
	typedef lxRef	lxRfloatarray;
	typedef lxRef	lxRscalararray;

	// Generic
	typedef lxRef	lxRfontset;

	// Sound
	typedef lxRef	lxRsoundnode;

	// Spatial
	typedef lxRef	lxRactornode;
	typedef lxRef	lxRscenenode;
	
	// List2D
	typedef lxRef	lxRl2dnode;
	typedef lxRl2dnode	lxRl2dimage;
	typedef lxRl2dnode	lxRl2dtext;
	typedef lxRl2dnode	lxRl2dnode3d;
	typedef lxRl2dnode	lxRl2dflag;
	typedef lxRl2dnode	lxRl2dview3d;

	// List3D
	typedef lxRef	lxRl3dnode;
	typedef lxRl3dnode	lxRl3dmodel;
	typedef lxRl3dnode	lxRl3dprimitive;
	typedef lxRl3dnode	lxRl3dshadowmodel;
	typedef lxRl3dnode	lxRl3dlevelmodel;
	typedef lxRl3dnode	lxRl3dtrail;
	typedef lxRl3dnode	lxRl3dtext;
	typedef lxRl3dnode	lxRl3dprojector;
	typedef lxRl3dnode	lxRl3dlight;
	typedef lxRl3dnode	lxRl3dcamera;
	typedef lxRl3dnode	lxRl3dpgroup;
	typedef lxRl3dnode	lxRl3dpemitter;
	
	// List2D/3D Rendering
	typedef lxRef	lxRl3dview;
	typedef lxRef	lxRl3dbatchbuffer;

	typedef lxRef	lxRskybox;
	typedef lxRef	lxRfrustumobject;
	typedef lxRef	lxRmorphcontrol;
	typedef lxRef	lxRbonecontrol;
	typedef lxRef	lxRmatautocontrol;

	typedef lxRef	lxRrcmd;
	typedef lxRrcmd		lxRrcmdclear;
	typedef lxRrcmd		lxRrcmdstencil;
	typedef lxRrcmd		lxRrcmddepth;
	typedef lxRrcmd		lxRrcmdlogicop;
	typedef lxRrcmd		lxRrcmdforceflag;
	typedef lxRrcmd		lxRrcmdignoreflag;
	typedef lxRrcmd		lxRrcmdshaders;
	typedef lxRrcmd		lxRrcmdshadersoff;
	typedef lxRrcmd		lxRrcmdignore;
	typedef lxRrcmd		lxRrcmdcopytex;
	typedef lxRrcmd		lxRrcmddrawmesh;
	typedef lxRrcmd		lxRrcmddrawbg;
	typedef lxRrcmd		lxRrcmddrawdbg;
	typedef lxRrcmd		lxRrcmddrawlayer;
	typedef lxRrcmd		lxRrcmddrawprt;
	typedef lxRrcmd		lxRrcmddrawl2d;
	typedef lxRrcmd		lxRrcmddrawl3d;
	typedef lxRrcmd		lxRrcmdmipmaps;
	typedef lxRrcmd		lxRrcmdreadpixels;
	typedef	lxRrcmd		lxRrcmdr2vb;
	typedef lxRrcmd		lxRrcmdupdatetex;
	typedef lxRrcmd		lxRrcmdfncall;

	typedef lxRrcmd		lxRrcmdfbobind;
	typedef lxRrcmd		lxRrcmdfbooff;
	typedef lxRrcmd		lxRrcmdfbotex;
	typedef lxRrcmd		lxRrcmdfborb;
	typedef lxRrcmd		lxRrcmdfbodrawto;
	typedef lxRrcmd		lxRrcmdfboreadfrom;
	typedef lxRrcmd		lxRrcmdfboblit;
	
	typedef lxRef	lxRrenderbuffer;
	typedef lxRef	lxRrenderfbo;
	typedef lxRef	lxRvidbuffer;
	typedef lxRef	lxRrendermesh;

	//////////////////////////////////////////////////////////////////////////
	// ResourceIDs

	typedef lxResID	lxRIDtexture;
	typedef lxResID	lxRIDmodel;
	typedef lxResID	lxRIDanimation;
	typedef lxResID	lxRIDshader;
	typedef lxResID	lxRIDmaterial;
	typedef lxResID	lxRIDgpuprog;
	typedef lxResID	lxRIDsound;
	typedef lxResID	lxRIDprtsys;
	typedef lxResID	lxRIDprtcloud;

	typedef lxResSubID	lxRIDmeshSUB;
	typedef lxResSubID	lxRIDskinSUB;
	typedef lxResSubID	lxRIDboneSUB;

	typedef lxResSubID	lxRIDmattexctrlSUB;
	typedef lxResSubID	lxRIDmatshdctrlSUB;
	typedef lxResSubID	lxRIDmatcontrolSUB;

	typedef lxResSubID	lxRIDshaderparamSUB;

	typedef lxResSubID	lxRIDprtforceSUB;
	
	typedef lxResSubID	lxRIDtrackSUB;
	

	typedef void (lxRcmdfncall_func_fn)(void* upvalue, const float projmatrix[16], const float viewmatrix[16], const float nodematrix[16]);

#ifdef __cplusplus
}
#endif

#endif