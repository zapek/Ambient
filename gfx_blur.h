#ifndef AMBIENT_GFX_BLUR_H
#define AMBIENT_GFX_BLUR_H
/*
 * $Id: gfx_blur.h,v 1.3 2006/02/22 14:48:19 fab Exp $
 */

void gfx_blur_alpha(APTR bm, ULONG w, ULONG h);
void gfx_blur_alpha_transfer(APTR bm, ULONG w, ULONG h, LONG component, ULONG fill);


#endif /* AMBIENT_GFX_BLUR_H */
