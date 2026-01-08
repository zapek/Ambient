#ifndef AMBIENT_GFX_H
#define AMBIENT_GFX_H
/*
 * $Id: gfx.h,v 1.3 2006/02/22 14:48:19 fab Exp $
 */

#ifdef DEBUG

#include <cybergraphx/cybergraphics.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>


#define CHECKCYBERMAP(bm) ({ \
	ASSERT(bm); \
	if (!GetCyberMapAttr(bm, CYBRMATTR_ISCYBERGFX)) \
	{ \
		PDB(("bitmap 0x%lx is not a cgx bitmap\n", (ULONG)bm)); \
	}})

#define CHECKARGB32(bm) ({ \
	ASSERT(bm); \
	if (!GetCyberMapAttr(bm, CYBRMATTR_ISCYBERGFX)) \
	{ \
		PDB(("bitmap 0x%lx is not a cgx bitmap\n", (ULONG)bm)); \
	} \
	if (GetCyberMapAttr(bm, CYBRMATTR_PIXFMT) != PIXFMT_ARGB32) \
	{ \
		PDB(("bitmap 0x%lx is not an ARGB32 bitmap\n", (ULONG)bm)); \
	}})

#define CHECKMASK(bm) ({ \
	ASSERT(bm); \
	if (GetBitMapAttr(bm, BMA_DEPTH) != 1) \
	{ \
		PDB(("bitmap 0x%lx is not a mask\n", (ULONG)bm)); \
	}})

#define CHECKPLANAR(bm) ({ \
	ASSERT(bm); \
	if (GetBitMapAttr(bm, BMA_DEPTH) > 8) \
	{ \
		PDB(("bitmap 0x%lx is not planar\n", (ULONG)bm)); \
	}})

#else

#define CHECKCYBERMAP(bm)
#define CHECKARGB32(bm)
#define CHECKMASK(bm)
#define CHECKPLANAR(bm)

#endif

#endif /* AMBIENT_GFX_H */
