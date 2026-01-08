#ifndef AMBIENT_GFX_ALPHA_H
#define AMBIENT_GFX_ALPHA_H
/*
 * $Id: gfx_alpha.h,v 1.5 2009/09/27 19:18:05 kiero Exp $
 */

void gfx_alpha_set(APTR bm, ULONG x, ULONG y, ULONG width, ULONG height, UBYTE val);
#if 0
void gfx_alpha_blend(APTR srcbm, APTR dstbm, ULONG x, ULONG y, ULONG width, ULONG height, UBYTE val);
#endif
void gfx_alpha_set_mask(APTR bm, ULONG x, ULONG y, ULONG width, ULONG height, APTR mask, UBYTE val, ULONG mode);
void gfx_alpha_set_array_add(APTR bm, ULONG x, ULONG y, ULONG width, ULONG height, UBYTE *a, UBYTE val);
void gfx_alpha_compose(APTR bm, ULONG x, ULONG y, ULONG width, ULONG height, UBYTE val);
void gfx_alpha_set_radial(APTR bm, ULONG x, ULONG y, ULONG width, ULONG height, UBYTE val, ULONG radius);
void gfx_alpha_transfer(APTR bm, ULONG w, ULONG h, LONG component, ULONG fill);
ULONG gfx_alpha_ispresent(APTR bm);

/* for gfx_set_alpha_mask() */
enum {
	ACM_SET,               /* just set the value */
	ACM_SET_AND_CLEAR,     /* set the value and clears the rest */
	ACM_COMPOSE,           /* adds 'val' to the value already there */
	ACM_COMPOSE_AND_CLEAR, /* composes and clears the rest */
};

#endif /* AMBIENT_GFX_ALPHA_H */
