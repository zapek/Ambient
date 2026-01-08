/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 * $Id: gfx_dbuf.c,v 1.1 2015/03/02 13:50:54 geit Exp $
 */

#include "../config.h"
#include "../macros.h"
#include "../library.h"

#include <graphics/gfx.h>
#include <graphics/layers.h>
#include <cybergraphx/cybergraphics.h>
#include <workbench/ambientsupport.h>

#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/layers.h>
#define USE_INLINE_STDARG
#include <proto/ambientsupport.h>
#undef USE_INLINE_STDARG


/*
 * Clones a bitmap.
 */

struct doublebuf *LIB_gfx_DoubleBufferAlloc(ULONG w, ULONG h, ULONG flags, struct BitMap *fri, struct AmbientSupportBase *AmbientSupportBase )
{
	struct doublebuf *dbuf;

	if ((dbuf = malloc(sizeof(*dbuf))))
	{
		APTR bm;

		if ((bm = gfx_bitmap_create(w, h, BITMAPDEPTH_Clone, BITMAPTAG_VMem, (flags & DBUF_DISPLAYABLE) ? TRUE : FALSE, BITMAPTAG_ExtFriend, (IPTR) fri, TAG_DONE)))
		{
			dbuf->rp = NULL;

			if (flags & DBUF_CLIPPED)
			{
				struct Layer *l;
				struct Layer_Info *li;

				if ((li = NewLayerInfo()))
				{
					if ((l = CreateUpfrontLayer(li, gfx_bitmap_bm(bm), 0, 0, w - 1, h - 1, LAYERSIMPLE, NULL)))
					{
						dbuf->rp = l->rp;
					}
					else
					{
						DisposeLayerInfo(li);
					}
				}
			}
			else
			{
				if ((dbuf->rp = malloc(sizeof(*dbuf->rp))))
				{
					InitRastPort(dbuf->rp);
					dbuf->rp->BitMap = gfx_bitmap_bm(bm);
				}
			}

			if (dbuf->rp)
			{
				dbuf->bm = bm;

				/* reuse */
				dbuf->pixfmt = GetCyberMapAttr(fri, CYBRMATTR_PIXFMT);
				dbuf->flags = flags;

				/*
				 * Would be useful to have some defaults
				 * there (font, drawmode, background)
				 * but.. bah
				 */
				return (dbuf);
			}
			gfx_bitmap_delete(bm);
		}
		free(dbuf);
	}
	return (NULL);
}


void LIB_gfx_DoubleBufferFree(struct doublebuf *dbuf, struct AmbientSupportBase *AmbientSupportBase )
{
	ASSERT(dbuf);
	ASSERT(dbuf->bm);
	ASSERT(dbuf->rp);

	if (dbuf->flags & DBUF_CLIPPED)
	{
		struct Layer_Info *li = dbuf->rp->Layer->LayerInfo;

		DeleteLayer(NULL, dbuf->rp->Layer);
		DisposeLayerInfo(li);
	}
	else
	{
		free(dbuf->rp);
	}
	gfx_bitmap_delete(dbuf->bm);
	free(dbuf);
}


struct doublebuf *LIB_gfx_DoubleBufferAllocReuse(struct doublebuf *dbuf, ULONG w, ULONG h, ULONG flags, struct BitMap *fri, struct AmbientSupportBase *AmbientSupportBase )
{
	if (dbuf && fri && dbuf->pixfmt == GetCyberMapAttr(fri, CYBRMATTR_PIXFMT) && gfx_bitmap_width(dbuf->bm) == w && gfx_bitmap_height(dbuf->bm) == h && dbuf->flags == flags)
	{
		return (dbuf);
	}
	else
	{
		if (dbuf)
		{
			gfx_dbuf_free(dbuf);
		}
		return (gfx_dbuf_alloc(w, h, flags, fri));
	}
}
