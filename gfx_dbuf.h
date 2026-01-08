#ifndef AMBIENT_GFX_DBUF_H
#define AMBIENT_GFX_DBUF_H
/*
 * $Id: gfx_dbuf.h,v 1.3 2006/02/22 14:48:20 fab Exp $
 */

#include <graphics/gfx.h>

struct doublebuf {
	APTR bm;
	struct RastPort *rp;
	/* for reuse call */
	ULONG pixfmt;
	ULONG flags;
};

#define DBUF_DISPLAYABLE (1L << 0) /* is allocated in VRAM, if possible */
#define DBUF_CLIPPED     (1L << 1) /* bitmap is clipped */

struct doublebuf * gfx_dbuf_alloc(ULONG w, ULONG h, ULONG flags, struct BitMap *fri);
void gfx_dbuf_free(struct doublebuf *dbuf);
struct doublebuf * gfx_dbuf_alloc_reuse(struct doublebuf *dbuf, ULONG w, ULONG h, ULONG flags, struct BitMap *fri);

#endif /* AMBIENT_GFX_DBUF_H */
