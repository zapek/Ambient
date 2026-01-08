#ifndef AMBIENT_GFX_BLIT_H
#define AMBIENT_GFX_BLIT_H
/*
 * $Id: gfx_blit.h,v 1.5 2015/08/11 21:51:17 itix Exp $
 */

struct BitMap;
struct Rectangle;

void gfx_blit_tiled(APTR src ,WORD offx, WORD offy, struct BitMap *Dst,struct Rectangle *DstBounds);
void gfx_blit_tiled_alpha(APTR src, WORD offx, WORD offy, struct BitMap *Dst, struct Rectangle *DstBounds, size_t mixlevel);

void v_gfx_blit(APTR src, APTR dst, struct TagItem *tags);
void gfx_blit(APTR src, APTR dst, ...);

/*
 * gfx_blit() tags.
 */
enum {
	BLITTAG_SrcX = TAG_USER + 1, /* source X offset (default: 0) */
	BLITTAG_SrcY,                /* source Y offset (default: 0) */
	BLITTAG_DstX,                /* destination X offset (default: 0) */
	BLITTAG_DstY,                /* destination Y offset (default: 0) */
	BLITTAG_DstWidth,            /* destination width (default: the smallest width between src and dst) */
	BLITTAG_DstHeight,           /* destination height (default: the smallest height between src and dst) */
	BLITTAG_Minterm,             /* minterm to use (default: 0xc0, plain copy) */
	BLITTAG_SrcType,             /* type of the source (default: BLITVAL_SrcType_Context) */
	BLITTAG_DstType,             /* type of the destination (default: BLITVAL_DstType_Context) */
	BLITTAG_SrcFormat,           /* format of the src (only needed for BLITVAL_SrcType_Array) */
	BLITTAG_Alpha,               /* value of the alpha (0x0 (transparent) - 0xffffffff (opaque), default: none, which means no alpha) */
	BLITTAG_Mask,                /* mask to use when blitting (default: 0xff, all planes) */
	BLITTAG_MaskPlane,           /* pointer to a mask plane, has to be the same size as the source */
	BLITTAG_Modulo,              /* modulo (optional for BLITVAL_SrcType_Array, constructed from the width otherwise) */
	BLITTAG_CMAP,                /* pointer to CLUT map, needs also the format */
	BLITTAG_CMAPFormat,          /* BLITVAL_CMAP_#? */
};


/*
 * BLITTAG_*Type values.
 */
#define BLITVAL_SrcType_Context 0 /* what gfx_bitmap_create() returned */
#define BLITVAL_DstType_Context BLITVAL_SrcType_Context
#define BLITVAL_SrcType_RastPort 1 /* struct RastPort * */
#define BLITVAL_DstType_RastPort BLITVAL_SrcType_RastPort
#define BLITVAL_SrcType_Array 2 /* an array of pixels, needs a format value then */

/*
 * BLITTAG_SrcFormat values.
 */
#define BLITVAL_SrcFormat_RGB   (0UL)
#define BLITVAL_SrcFormat_RGBA  (1UL)
#define BLITVAL_SrcFormat_ARGB  (2UL)
#define BLITVAL_SrcFormat_LUT8  (3UL)
#define BLITVAL_SrcFormat_GREY8 (4UL)
#define BLITVAL_SrcFormat_RAW   (5UL)

/*
 * BLITTAG_CMAPFormat values.
 */
#define BLITVAL_CMAPFormat_XRGB8 (0UL)
#define BLITVAL_CMAPFormat_RGB8  (2UL)


#endif /* AMBIENT_GFX_BLIT_H */
