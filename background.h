#ifndef AMBIENT_BACKGROUND_H
#define AMBIENT_BACKGROUND_H
/*
 * $Id: background.h,v 1.9 2018/08/12 20:53:15 itix Exp $
 */

#include <exec/types.h>
#include <graphics/rastport.h>

enum {
	BGRENDER_Color,     /* just a plain color (root and window) */
	BGRENDER_Tiled,     /* normal tiling (root and window) */
	BGRENDER_Centered,  /* center image + color */
	BGRENDER_Scaled,    /* scale image */
	BGRENDER_Stretched, /* stretch image to screen dimensions */
	BGRENDER_Zoomed,    /* scale image in a way there are no borders visible */
};

void background_blit(ULONG, APTR, ULONG, int, int, struct RastPort *, WORD, WORD, WORD, WORD);
ULONG tr_background_load(APTR obj, ULONG type, ULONG mode, CONST_STRPTR filename);

#endif /* AMBIENT_BACKGROUND_H */
