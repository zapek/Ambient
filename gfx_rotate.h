#ifndef AMBIENT_GFX_ROTATE_H
#define AMBIENT_GFX_ROTATE_H
/*
 * $Id: gfx_rotate.h,v 1.2 2006/02/22 14:48:20 fab Exp $
 */

APTR gfx_rotate(APTR bm, ULONG mode);

/* for gfx_rotate() */
enum {
	ROTATE_90,
	ROTATE_180,
	ROTATE_270,
};

#endif /* AMBIENT_GFX_ROTATE_H */
