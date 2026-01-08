#ifndef AMBIENT_TEXTBOX_H
#define AMBIENT_TEXTBOX_H
/*
 * $Id: textbox.h,v 1.5 2007/02/11 22:35:30 fab Exp $
 */

#include <utility/tagitem.h>

#include "fonts.h"

APTR textbox_create(struct atextfont *afont, ...);
APTR v_textbox_create(struct atextfont *afont, struct TagItem *tags);
void textbox_delete(APTR ctx);
void textbox_render(APTR ctx, struct RastPort *rp, ULONG x, ULONG y);
void textbox_setattrs(APTR ctx, ...);
void v_textbox_setattrs(APTR ctx, struct TagItem *tags);
ULONG textbox_getwidth(APTR ctx);
ULONG textbox_getheight(APTR ctx);


/*
 * Tags
 */
enum {
	TEXTBOXTAG_RenderMode = TAG_USER + 1, /* [I..] font rendering mode, see TBRENDER_ below */
	TEXTBOXTAG_Text,                      /* [I..] STRPTR to actual text */
	TEXTBOXTAG_Color1,                    /* [I..] color1 (usually foreground) */
	TEXTBOXTAG_Color2,                    /* [I..] color2 (background or outline color) */
	TEXTBOXTAG_Width,                     /* [I..] suggested width limit in pixels, text will be multilined to fit */
	TEXTBOXTAG_Alpha,                     /* [IS.] alpha to render with (default: 0xffffffff) */
	TEXTBOXTAG_Italic,                    /* [IS.] renders text as italic */
};

enum {
	TBRENDER_Normal,      /* normal */
	TBRENDER_Shadow,      /* simple shadow */
	TBRENDER_AlphaShadow, /* WinXP-like alpha shadow */
	TBRENDER_Outline,     /* outline mode à la DOpus */
	TBRENDER_Background,  /* JAM2 */
	TBRENDER_MultiColor,  /* multicolor mode using Google colors for each font or something like that :) */
	TBRENDER_Gradient,    /* Does a gradient from color 1 to color 2 */

};



#endif /* AMBIENT_TEXTBOX_H */
