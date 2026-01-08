#ifndef AMBIENT_ALPHA_VEC_H
#define AMBIENT_ALPHA_VEC_H
/*
 * $Id: gfx_alpha_vec.h,v 1.3 2006/02/22 14:48:19 fab Exp $
 */

#if USE_ALTIVEC
void gfx_alpha_set_vec(APTR bm, ULONG x, ULONG y, ULONG width, ULONG height, UBYTE val);
void gfx_alpha_compose_vec(APTR bm, ULONG x, ULONG y, ULONG width, ULONG height, UBYTE val, void (*fp)(UBYTE *p, ULONG val));
#endif

#endif /* AMBIENT_ALPHA_VEC_H */
