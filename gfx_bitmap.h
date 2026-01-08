#ifndef AMBIENT_GFX_BITMAP_H
#define AMBIENT_GFX_BITMAP_H
/*
 * $Id: gfx_bitmap.h,v 1.7 2015/08/15 18:41:41 itix Exp $
 */

#include <graphics/gfx.h>

/* this structure should be considered private, use the accessor macros */
struct bitmap_ctx {
	struct BitMap *bm;
	ULONG width;
	ULONG height;
	ULONG depth;
	ULONG modulo;
	ULONG bpr;
	ULONG isnative;

	LONG usecount;
	struct bitmap_ctx *ref;
};

struct Screen;

APTR v_gfx_bitmap_create(ULONG width, ULONG height, ULONG depth, struct TagItem *tags);
APTR gfx_bitmap_create(ULONG width, ULONG height, ULONG depth, ...);
void gfx_bitmap_delete(APTR ctx);
APTR gfx_bitmap_create_from_native(struct BitMap *bm, ULONG width, ULONG height);
BOOL gfx_bitmap_check_vram(APTR bm);
APTR gfx_bitmap_clone(APTR bm, LONG vmem, struct Screen *screen_friend);
#ifdef DEBUG
void gfx_bitmap_check_vmem(APTR bm);
#endif

/*
 * Tags
 */
enum {
	BITMAPTAG_Friend = TAG_USER + 1, /* pointer to friend (another bitmap context) */
	BITMAPTAG_ExtFriend,             /* pointer to friend (amiga-style bitmaps) */
	BITMAPTAG_ScreenFriend,          /* pointer to friend (struct Screen *) */
	BITMAPTAG_VMem,                  /* bitmap should be allocated in video memory (but it is not locked there and can move anytime, default: FALSE) */
	BITMAPTAG_Format,                /* bitmap format, see BITMAPVAL_Format_#?. if a friend is passed the format is not guaranteed */
	BITMAPTAG_Clear,                 /* clears the bitmap (default: FALSE) */
};

/*
 * BITMAPTAG_Format
 */
#define BITMAPVAL_Format_LUT8    (0UL)
#define BITMAPVAL_Format_RGB15   (1UL)
#define BITMAPVAL_Format_BGR15   (2UL)
#define BITMAPVAL_Format_RGB15PC (3UL)
#define BITMAPVAL_Format_BGR15PC (4UL)
#define BITMAPVAL_Format_RGB16   (5UL)
#define BITMAPVAL_Format_BGR16   (6UL)
#define BITMAPVAL_Format_RGB16PC (7UL)
#define BITMAPVAL_Format_BGR16PC (8UL)
#define BITMAPVAL_Format_RGB24   (9UL)
#define BITMAPVAL_Format_BGR24   (10UL)
#define BITMAPVAL_Format_ARGB32  (11UL)
#define BITMAPVAL_Format_BGRA32  (12UL)
#define BITMAPVAL_Format_RGBA32  (13UL)

/*
 * Special values for the 'width', 'height' and 'depth arguments.
 * If given, the width/height/depth is taken from the friend bitmap.
 * If there's no friend the function fails.
 */
#define BITMAPWIDTH_Clone 0
#define BITMAPHEIGHT_Clone 0
#define BITMAPDEPTH_Clone 0

/*
 * Accessor macros
 */
#define gfx_bitmap_width(_ctx) (((struct bitmap_ctx *)_ctx)->width)
#define gfx_bitmap_height(_ctx) (((struct bitmap_ctx *)_ctx)->height)
#define gfx_bitmap_depth(_ctx) (((struct bitmap_ctx *)_ctx)->depth)
#define gfx_bitmap_modulo(_ctx) (((struct bitmap_ctx *)_ctx)->modulo) /* aka bytes per row */
#define gfx_bitmap_bpr(_ctx) (((struct bitmap_ctx *)_ctx)->bpr) /* aka bytes per row */
#define gfx_bitmap_array(_ctx) (((struct bitmap_ctx *)_ctx)->bm->Planes[0]) /* array of data */
#define gfx_bitmap_bm(_ctx) (((struct bitmap_ctx *)_ctx)->bm) /* if we ever need a native bitmap ptr */

#endif /* AMBIENT_GFX_BITMAP_H */
