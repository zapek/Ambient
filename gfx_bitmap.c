/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2015 Ambient Open Source Team
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
 * $Id: gfx_bitmap.c,v 1.12 2017/08/11 23:32:05 cyfm Exp $
 */

#include "ambient.h"

/* public */
#include <cybergraphx/cybergraphics.h>
#include <hardware/atomic.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>

/* private */
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "mui_func.h" /* FORTAG.. */
#include "screen.h"


APTR v_gfx_bitmap_create(ULONG width, ULONG height, ULONG depth, struct TagItem *tags)
{
	struct bitmap_ctx *ct;

	if ((ct = malloc(sizeof(*ct))))
	{
		struct BitMap *afr = NULL;
		ULONG flags        = BMF_MINPLANES;
		ULONG has_fmt      = FALSE;
		ULONG fmt          = 0;

		ct->isnative = FALSE;
		ct->usecount = 1;
		ct->ref = NULL;

		FORTAG(tags)
		{
			case BITMAPTAG_Friend:
				ASSERT(tag->ti_Data);
				afr = gfx_bitmap_bm(tag->ti_Data);
				break;

			case BITMAPTAG_ExtFriend:
				ASSERT(tag->ti_Data);
				afr = (struct BitMap *)tag->ti_Data;
				break;

			case BITMAPTAG_ScreenFriend:
				if (tag->ti_Data)
					afr = ((struct Screen *)tag->ti_Data)->RastPort.BitMap;
				break;

			case BITMAPTAG_VMem:
				if (tag->ti_Data)
				{
					flags |= BMF_DISPLAYABLE;
				}
				break;

			case BITMAPTAG_Format:
				has_fmt = TRUE;
				fmt = tag->ti_Data;
				break;

			case BITMAPTAG_Clear:
				if (tag->ti_Data)
				{
					flags |= BMF_CLEAR;
				}
				break;

			#ifdef DEBUG
			default:
				PDB(("unknown tag 0x%lx\n", tag->ti_Tag));
				break;
			#endif
		}
		NEXTTAG

		if (afr)
		{
			if (width == BITMAPWIDTH_Clone)
				width = GetBitMapAttr(afr, BMA_WIDTH);
			if (height == BITMAPHEIGHT_Clone)
				height = GetBitMapAttr(afr, BMA_HEIGHT);
			if (depth == BITMAPDEPTH_Clone)
				depth = GetBitMapAttr(afr, BMA_DEPTH);
		}

		if (!(width == BITMAPWIDTH_Clone || height == BITMAPHEIGHT_Clone || depth == BITMAPDEPTH_Clone))
		{
			if (depth <= 8)
			{
				#warning check this...
				flags &= ~BMF_MINPLANES;	/* XXX: sure about that one ? think so.. */
			}

			if (has_fmt && afr)
			{
				if (afr)
				{
					PDB(("warning, format and friend specified both at the same time. result is undefined\n"));
				}
				flags |= BMF_SPECIALFMT | SHIFT_PIXFMT(fmt);
			}

			ct->width  = width;
			ct->height = height;
			ct->depth  = depth;

			if ((ct->bm = AllocBitMap(ct->width, ct->height, ct->depth, flags, afr)))
			{
				ASSERT(ct->height == GetBitMapAttr(ct->bm, BMA_HEIGHT));
				ASSERT(ct->depth == GetBitMapAttr(ct->bm, BMA_DEPTH));

				#if 0
				if (ct->depth != GetBitMapAttr(ct->bm, BMA_DEPTH))
				{
					DB(("Requester:%d,Depth:%d, flags=0x%x, afr=0x%x\n", ct->depth, GetBitMapAttr(ct->bm, BMA_DEPTH), flags, afr));
				}
				#endif

				if (depth <= 8)
				{
					if (GetCyberMapAttr(ct->bm, CYBRMATTR_ISCYBERGFX) && (GetCyberMapAttr(ct->bm, CYBRMATTR_PIXFMT) == PIXFMT_LUT8))	/* XXX: I think */
					{
						ct->modulo = ct->bpr = GetCyberMapAttr(ct->bm, CYBRMATTR_XMOD);
					}
					else
					{
						ct->modulo = ct->bpr = ct->bm->BytesPerRow;
					}
				}
				else
				{
					ct->bpr = GetCyberMapAttr(ct->bm, CYBRMATTR_XMOD);
					ct->modulo = ct->bpr / GetCyberMapAttr(ct->bm, CYBRMATTR_BPPIX);
				}
				return (ct);
			}
			else
			{
				PDB(("failed to allocate a bitmap for width: %ld, height: %ld, depth: %ld, flags: %ld, friend: %p\n", ct->width, ct->height, ct->depth, flags, afr));
			}
		}
		else
		{
			PDB(("no friend to find out proper attributes, width: %ld, height: %ld, depth: %ld\n", width, height, depth));
		}
		free(ct);
	}
	return (NULL);
}


APTR gfx_bitmap_create_from_native(struct BitMap * bm, ULONG width, ULONG height)
{
	struct bitmap_ctx *ct;

	if ((ct = malloc(sizeof(*ct))))
	{
		ct->bm       = bm;
		ct->width    = width;
		ct->height   = height;
		ct->depth    = GetBitMapAttr(bm, BMA_DEPTH);
		ct->isnative = TRUE;
		ct->usecount = 1;
		ct->ref      = NULL;

		ct->bpr = GetCyberMapAttr(ct->bm, CYBRMATTR_XMOD);

		if (ct->depth <= 8)
		{
			if (GetCyberMapAttr(ct->bm, CYBRMATTR_ISCYBERGFX) && (GetCyberMapAttr(ct->bm, CYBRMATTR_PIXFMT) == PIXFMT_LUT8))	/* XXX: I think */
			{
				ct->modulo = ct->bpr = GetCyberMapAttr(ct->bm, CYBRMATTR_XMOD);
			}
			else
			{
				ct->modulo = ct->bpr = ct->bm->BytesPerRow;
			}
		}
		else
		{
			ct->bpr = GetCyberMapAttr(ct->bm, CYBRMATTR_XMOD);
			ct->modulo = ct->bpr / GetCyberMapAttr(ct->bm, CYBRMATTR_BPPIX);
		}
	}
	return (ct);
}


void gfx_bitmap_delete(APTR ctx)
{
	if (ctx)
	{
		struct bitmap_ctx *ct = ctx;
		ASSERT(ct->bm);

		if (ct->ref)
		{
			APTR tmp = ct->ref;

			free(ct);
			ct = tmp;
		}

		if (ATOMIC_SUB(&ct->usecount, 1) <= 1)
		{
			if (!ct->isnative)
			{
				FreeBitMap(ct->bm);
			}
			free(ct);
		}
		else
		{
			PDB(("Bitmap not freed because usecount is %ld\n", ct->usecount));
		}
	}
}


BOOL gfx_bitmap_check_vram(APTR bm)
{
	ULONG addr;
	APTR handle;
	BOOL rc = FALSE;

	if ((handle = LockBitMapTags(gfx_bitmap_bm(bm), LBMI_BASEADDRESS, &addr, TAG_DONE)))
	{
		if (TypeOfMem((APTR) addr) == 0)
			rc = TRUE;

		UnLockBitMap(handle);
	}

	return rc;
}


APTR gfx_bitmap_clone(APTR bm, LONG vmem, struct Screen *screen_friend)
{
	struct bitmap_ctx *src = bm;
	struct bitmap_ctx *dst;

	if (src->ref)
		src = src->ref;

	if (vmem && gfx_bitmap_check_vram(src) == 0)
	{
		PDB(("Source not in VMEM. Create new bitmap...\n"));

		dst = gfx_bitmap_create(src->width, src->height, src->depth,
			BITMAPTAG_VMem, vmem, BITMAPTAG_ScreenFriend, screen_friend, TAG_DONE);

		if (dst)
		{
			if (vmem && gfx_bitmap_check_vram(dst) == FALSE)
			{
				PDB(("Could not allocate bitmap from VMEM\n"));
				gfx_bitmap_delete(dst);
				dst = NULL;
			}
			else
			{
				gfx_blit(src, dst, TAG_DONE);
			}
		}
	}
	else
	{
		dst = malloc(sizeof(*dst));

		if (dst)
		{
			ATOMIC_ADD(&src->usecount, 1);
			bcopy(src, dst, sizeof(*dst));

			dst->ref = src;
		}
	}

	return dst;
}


#ifdef DEBUG
void gfx_bitmap_check_vmem(APTR bm)
{
	if (gfx_bitmap_check_vram(bm) == FALSE)
	{
		kprintf("warning: bitmap %p is not in VRAM\n", bm);
	}
}
#endif
