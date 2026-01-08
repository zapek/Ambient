#ifndef AMBIENT_GFX_MASK_H
#define AMBIENT_GFX_MASK_H
/*
 * $Id: gfx_mask.h,v 1.3 2006/02/22 14:48:20 fab Exp $
 */

APTR gfx_mask_create_planar(APTR bm, ULONG width, ULONG height, ULONG depth);
APTR gfx_mask_create_chunky8(UBYTE *chunky, ULONG width, ULONG height, ULONG bgvalue);
APTR gfx_mask_create_inverted(APTR bm, ULONG width, ULONG height);
#if USE_SOLIDDRAG
APTR gfx_mask_create_cgx_fastram(APTR bm, ULONG width, ULONG height, ULONG bgvalue);
#endif

#endif /* AMBIENT_GFX_MASK_H */
