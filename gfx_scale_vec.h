#ifndef AMBIENT_GFX_SCALE_VEC_H
#define AMBIENT_GFX_SCALE_VEC_H
/*
 * $Id: gfx_scale_vec.h,v 1.4 2006/08/08 13:31:34 fab Exp $
 */

#if USE_ALTIVEC
ULONG gfx_scale_vec_average(struct gsi_info *gsii, CONST ULONG *s, ULONG x, ULONG y);
#endif

#endif /* AMBIENT_GFX_SCALE_VEC_H */
