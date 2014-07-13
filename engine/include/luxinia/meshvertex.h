// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __LUXINIA_MESH_VERTEX_H__
#define __LUXINIA_MESH_VERTEX_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum lxVertexType_e{
	VERTEX_32_TEX2 = 0,		// 2 texcoords, color
	VERTEX_32_NRM,			// 1 tecoord, normal, color
	VERTEX_32_FN,			// float normals, no color
	VERTEX_32_TERR,	
	VERTEX_64_TEX4,			// 4 texcoords, tangent, normal, color
	VERTEX_64_SKIN,			// 2 texcoords, tangent, normal, color, blending
	VERTEX_16_HOM,			// homogenous coord, no color
	VERTEX_16_CLR,			// color
	VERTEX_16_TERR,
	VERTEX_LASTTYPE
}lxVertexType_t;


typedef struct lxVertex16Color_s{
	float			pos[3];
	unsigned char	color[4];
}lxVertex16Color_t;

typedef struct lxVertex16Hom_s{
	float			pos[4];
}lxVertex16Hom_t;

typedef struct lxVertex16Terrain_s{
	unsigned char	tangent[4];
	unsigned char	normal[4];

	short			pos[4];
}lxVertex16Terrain_t;

typedef struct lxVertex32Terrain_s{
	short			tangent[4];
	short			normal[4];

	float			pos[3];
	unsigned char	color[4];
}lxVertex32Terrain_t;

typedef struct lxVertex32Nrm_s{
	float			tex[2];	
	short			normal[4];

	float			pos[3];
	unsigned char	color[4];
}lxVertex32Nrm_t;

typedef struct lxVertex32Tex2_s{
	float			tex[2];	
	float			tex1[2];

	float			pos[3];
	unsigned char	color[4];
}lxVertex32Tex2_t;

typedef struct lxVertex32FN_s{
	float			pos[3];
	float			normal[3];
	float			tex[2];
}lxVertex32FN_t;

typedef struct lxVertex64Tex4_s{	
	float			tex[2];
	float			tex1[2];
	float			tex2[2];
	float			tex3[2];
	short			normal[4];
	short			tangent[4];
	float			pos[3];
	unsigned char	color[4];
}lxVertex64Tex4_t;

typedef struct lxVertex64Skin_s{	
	float			tex[2];
	float			tex1[2];
	unsigned char	seccolor[4];
	unsigned char	blendi[4];
	unsigned short	blendw[4];
	short			normal[4];
	short			tangent[4];
	float			pos[3];
	unsigned char	color[4];
}lxVertex64Skin_t;

//////////////////////////////////////////////////////////////////////////
// Internal Types (unions, colorcompact)

typedef struct lxVertex16_s{
	float		pos[3];
	union{
		unsigned char	color[4];
		unsigned int	colorc;
		float			posw;
	};
}lxVertex16_t;

typedef struct lxVertex32_s{
	float		tex[2];		// base
	union{
		float	tex1[2];	// lightmap
		short	normal[4];	// normal
	};

	float		pos[3];
	union{
		unsigned char	color[4];
		unsigned int	colorc;
	};
}lxVertex32_t;


typedef struct lxVertex64_s{	
	float		tex[2];
	float		tex1[2];
	union{
		struct{
			float	tex2[2];
			float	tex3[2];
		};
		struct{
			unsigned char	seccolor[4];
			unsigned char	blendi[4];
			unsigned short	blendw[4];
		};
	};

	short		normal[4];
	short		tangent[4];
	float		pos[3];
	union{
		unsigned char	color[4];
		unsigned int	colorc;
	};
}lxVertex64_t;

#ifdef __cplusplus
}
#endif

#endif
