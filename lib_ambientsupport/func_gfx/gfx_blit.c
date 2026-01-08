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
 * $Id: gfx_blit.c,v 1.1 2015/03/02 13:50:54 geit Exp $
 */

#include "../config.h"
#include "../macros.h"
#include "../library.h"

#include <cybergraphx/cybergraphics.h>
#include <graphics/gfx.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/cybergraphics.h>
#include <workbench/ambientsupport.h>

/* 
 * Taken from MUI
 */

#define RECTSIZEX(r) ((r)->MaxX - (r)->MinX + 1)
#define RECTSIZEY(r) ((r)->MaxY - (r)->MinY + 1)

//void gfx_blit_tiled(struct BitMap *Src,WORD offx,WORD offy,WORD SrcSizeX,WORD SrcSizeY,struct BitMap *Dst,struct Rectangle *DstBounds)
void LIB_gfx_BlitTiled(APTR src, WORD offx, WORD offy, struct BitMap *Dst, struct Rectangle *DstBounds, struct AmbientSupportBase *AmbientSupportBase UNUSED)
{
	WORD FirstSizeX;  // the width of the rectangle to blit as the first column
	WORD FirstSizeY;  // the height of the rectangle to blit as the first row
	WORD SecondMinX;  // the left edge of the second column
	WORD SecondMinY;  // the top edge of the second column
	WORD SecondSizeX; // the width of the second column
	WORD SecondSizeY; // the height of the second column
	WORD Pos;         // used as starting position in the "exponential" blit
	WORD Size;        // used as bitmap size in the "exponential" blit
	struct BitMap *Src = gfx_bitmap_bm(src);
	WORD SrcSizeX = gfx_bitmap_width(src);
	WORD SrcSizeY = gfx_bitmap_height(src);

	CHECKCYBERMAP(Src);
	CHECKCYBERMAP(Dst);

	FirstSizeX = min(SrcSizeX-offx,RECTSIZEX(DstBounds)); // the width of the first tile, this is either the rest of the tile right to offx or the width of the dest rect, if the rect is narrow
	SecondMinX = DstBounds->MinX+FirstSizeX; // the start for the second tile (if used)
	SecondSizeX = min(offx,DstBounds->MaxX-SecondMinX+1); // the width of the second tile (we want the whole tile to be SrcSizeX pixels wide, if we use SrcSizeX-offx pixels for the left part we'll use offx for the right part)

	FirstSizeY = min(SrcSizeY-offy,RECTSIZEY(DstBounds)); // the same values are calculated for y direction
	SecondMinY = DstBounds->MinY+FirstSizeY;
	SecondSizeY = min(offy,DstBounds->MaxY-SecondMinY+1);

	BltBitMap(Src,offx,offy,Dst,DstBounds->MinX,DstBounds->MinY,FirstSizeX,FirstSizeY,0xC0,-1,NULL); // blit the first piece of the tile

	if (SecondSizeX>0) // if SrcOffset was 0 or the dest rect was to narrow, we won't need a second column
		BltBitMap(Src,0,offy,Dst,SecondMinX,DstBounds->MinY,SecondSizeX,FirstSizeY,0xC0,-1,NULL);
	if (SecondSizeY>0) // is a second row necessary?
	{
		BltBitMap(Src,offx,0,Dst,DstBounds->MinX,SecondMinY,FirstSizeX,SecondSizeY,0xC0,-1,NULL);
		if (SecondSizeX>0)
			BltBitMap(Src,0,0,Dst,SecondMinX,SecondMinY,SecondSizeX,SecondSizeY,0xC0,-1,NULL);
	}

	// this loop generates the first row of the tiles
	for (Pos = DstBounds->MinX+SrcSizeX,Size = min(SrcSizeX,DstBounds->MaxX-Pos+1);Pos<=DstBounds->MaxX;)
	{
		BltBitMap(Dst,DstBounds->MinX,DstBounds->MinY,Dst,Pos,DstBounds->MinY,Size,min(SrcSizeY,RECTSIZEY(DstBounds)),0xC0,-1,NULL);
		Pos += Size;
		Size = min(Size<<1,DstBounds->MaxX-Pos+1);
	}

	// this loop blit the first row down several times to fill the whole dest rect
	for (Pos = DstBounds->MinY+SrcSizeY,Size = min(SrcSizeY,DstBounds->MaxY-Pos+1);Pos<=DstBounds->MaxY;)
	{
		BltBitMap(Dst,DstBounds->MinX,DstBounds->MinY,Dst,DstBounds->MinX,Pos,RECTSIZEX(DstBounds),Size,0xC0,-1,NULL);
		Pos += Size;
		Size = min(Size<<1,DstBounds->MaxY-Pos+1);
	}
}

#define INITRP \
	struct RastPort rp; \
	({ if (!dstrp) \
	{ \
		InitRastPort(&rp); \
		rp.BitMap = dstbm; \
		dstrp = &rp; \
	} \
	})

void LIB_gfx_BlitA(APTR src, APTR dst, struct TagItem *tags)
{
	LONG srcx = 0;
	LONG srcy = 0;
	LONG dstx = 0;
	LONG dsty = 0;
	LONG dstxs = 0;
	LONG dstys = 0;
	ULONG srctype = BLITVAL_SrcType_Context;
	ULONG dsttype = BLITVAL_DstType_Context;
	ULONG has_srcformat = FALSE;
	ULONG srcformat = srcformat;
	ULONG has_alpha = FALSE;
	ULONG alphaval = alphaval;
	ULONG modulo = 0;
	ULONG cmapfmt = cmapfmt;
	ULONG has_cmapfmt = FALSE;
	struct BitMap *srcbm = NULL;
	struct BitMap *dstbm = NULL;
	UBYTE *maskplane = NULL;
	struct RastPort *dstrp = NULL;
	UBYTE *cmap = NULL;
	UBYTE *srcarray = NULL;
	ULONG minterm = 0xc0;
	ULONG mask = 0xff;

	/* XXX: check if the arguments are out of bounds (eg < 0 but only if we don't have a rastport! */

	FORTAG(tags)
	{
		case BLITTAG_SrcX:
			srcx = tag->ti_Data;
			break;

		case BLITTAG_SrcY:
			srcy = tag->ti_Data;
			break;

		case BLITTAG_DstX:
			dstx = tag->ti_Data;
			break;

		case BLITTAG_DstY:
			dsty = tag->ti_Data;
			break;

		case BLITTAG_DstWidth:
			dstxs = tag->ti_Data;
			break;

		case BLITTAG_DstHeight:
			dstys = tag->ti_Data;
			break;

		case BLITTAG_Minterm:
			minterm = tag->ti_Data;
			break;

		case BLITTAG_SrcType:
			srctype = tag->ti_Data;
			break;

		case BLITTAG_DstType:
			dsttype = tag->ti_Data;
			break;

		case BLITTAG_SrcFormat:
			has_srcformat = TRUE;
			srcformat = tag->ti_Data;
			break;

		case BLITTAG_Alpha:
			has_alpha = TRUE;
			alphaval = tag->ti_Data;
			break;

		case BLITTAG_Mask:
			mask = tag->ti_Data;
			break;

		case BLITTAG_MaskPlane:
			maskplane = (UBYTE *)tag->ti_Data;
			break;

		case BLITTAG_Modulo:
			modulo = tag->ti_Data;
			break;
		
		case BLITTAG_CMAP:
			cmap = (UBYTE *)tag->ti_Data;
			break;

		case BLITTAG_CMAPFormat:
			has_cmapfmt = TRUE;
			cmapfmt = tag->ti_Data;
			break;

		#ifdef DEBUG
		default:
			PDB(("unknown tag 0x%lx\n", tag->ti_Tag));
			break;
		#endif
	}
	NEXTTAG

	switch (srctype)
	{
		case BLITVAL_SrcType_Context:
			srcbm = gfx_bitmap_bm(src);
			break;

		case BLITVAL_SrcType_RastPort:
			srcbm = ((struct RastPort *)src)->BitMap;
			break;

		case BLITVAL_SrcType_Array:
			srcarray = src;
			break;

		#ifdef DEBUG
		default:
			PDB(("unknown srctype %ld\n", srctype));
			break;
		#endif
	}

	switch (dsttype)
	{
		case BLITVAL_DstType_Context:
			dstbm = gfx_bitmap_bm(dst);
			break;

		case BLITVAL_DstType_RastPort:
			dstbm = ((struct RastPort *)dst)->BitMap;
			dstrp = dst;
			break;

		#ifdef DEBUG
		default:
			PDB(("unknown srctype %ld\n", srctype));
			break;
		#endif
	}

	if (!dstxs)
	{
		if (srctype == BLITVAL_SrcType_Context)
		{
			if (srcbm)
			{
				dstxs = gfx_bitmap_width(src);
			}
		}

		if (dsttype == BLITVAL_DstType_Context)
		{
			if (dstbm)
			{
				if (dstxs)
				{
					dstxs = min(dstxs, gfx_bitmap_width(dst));
				}
				else
				{
					dstxs = gfx_bitmap_width(dst);
				}
			}
		}
	}

	if (!dstxs)
	{
		PDB(("no width given and unable to compute it, giving up\n"));
		return;
	}

	if (!dstys)
	{
		if (srctype == BLITVAL_SrcType_Context)
		{
			if (srcbm)
			{
				dstys = gfx_bitmap_height(src);
			}
		}

		if (dsttype == BLITVAL_DstType_Context)
		{
			if (dstbm)
			{
				if (dstys)
				{
					dstys = min(dstys, gfx_bitmap_height(dst));
				}
				else
				{
					dstys = gfx_bitmap_height(dst);
				}
			}
		}
	}

	if (!dstys)
	{
		PDB(("no height given and unable to compute it, giving up\n"));
		return;
	}

	if (has_alpha)
	{
		if (maskplane)
		{
			PDB(("maskplane and alpha not supported\n"));
		}
		else
		{
			if (srcarray != NULL || srcbm != NULL)
			{
				INITRP;

				if (srcbm != NULL)
				{
					struct TagItem tags[] = {{BLTBMA_USESOURCEALPHA, TRUE},
											{BLTBMA_MIXLEVEL, alphaval},
											{NULL, NULL}};

					BltBitMapRastPortAlpha(srcbm, srcx, srcy, dstrp, dstx, dsty, dstxs, dstys, tags);
				}
				else
				{
					ULONG xmod;

					if (!modulo)
						xmod = dstxs * 4; /* ARGB or BGRA assumed (XXX: add some check or so) */
					else
						xmod = modulo;

					if (xmod)
					{
						WritePixelArrayAlpha(srcarray, srcx, srcy, xmod, dstrp, dstx, dsty, dstxs, dstys, alphaval);
					}
				}
			}
			else
			{
				PDB(("no srcarray for alpha blitting\n"));
			}
		}
	}
	else
	{
		if (cmap)
		{
			if (has_cmapfmt)
			{
				if (srcarray)
				{
					if (!modulo)
					{
						switch (cmapfmt)
						{
							case BLITVAL_CMAPFormat_XRGB8:
								modulo = dstxs;
								break;

							case BLITVAL_CMAPFormat_RGB8:
								modulo = dstxs; /* XXX: alignement needed ? */
								break;
						}
					}

					if (modulo)
					{
						INITRP;
						WriteLUTPixelArray(srcarray, srcx, srcy, modulo, dstrp, cmap, dstx, dsty, dstxs, dstys, cmapfmt);
					}
					else
					{
						PDB(("no way to compute the modulo for cmap copy\n"));
					}
				}
				else
				{
					PDB(("no srcarray for cmap copy\n"));
				}
			}
			else
			{
				PDB(("missing cmapfmt\n"));
			}
		}
		else
		{
			if (maskplane)
			{
				if (srcbm)
				{
					INITRP;
					BltMaskBitMapRastPort(srcbm, srcx, srcy, dstrp, dstx, dsty, dstxs, dstys, minterm, maskplane);
				}
				else
				{
					PDB(("no source bitmap for mask blit\n"));
				}
			}
			else
			{
				if (srcbm)
				{
					if (dstrp)
					{
						BltBitMapRastPort(srcbm, srcx, srcy, dstrp, dstx, dsty, dstxs, dstys, minterm);
					}
					else
					{
						if (dstbm)
						{
							BltBitMap(srcbm, srcx, srcy, dstbm, dstx, dsty, dstxs, dstys, minterm, mask, NULL);
						}
						else
						{
							PDB(("no destination bitmap for blit\n"));
						}
					}
				}
				else if (srcarray)
				{
					if (has_srcformat)
					{
						if (!modulo)
						{
							switch (srcformat)
							{
								case BLITVAL_SrcFormat_LUT8:
								case BLITVAL_SrcFormat_GREY8:
									modulo = dstxs;  /* alignement needed ? */
									break;

								case BLITVAL_SrcFormat_RGB:
									modulo = dstxs * 3; /* alignement needed ? */
									break;

								case BLITVAL_SrcFormat_RGBA:
								case BLITVAL_SrcFormat_ARGB:
									modulo = dstxs * 4;
									break;

								#ifdef DEBUG
								default:
									PDB(("no automatic modulo generation for format %ld\n", srcformat));
									break;
								#endif
							}
						}

						if (modulo)
						{
							INITRP;
							WritePixelArray(srcarray, srcx, srcy, modulo, dstrp, dstx, dsty, dstxs, dstys, srcformat);
						}
						else
						{
							PDB(("missing modulo\n"));
						}
					}
					else
					{
						PDB(("missing srcformat\n"));
					}
				}
				else
				{
					PDB(("no source bitmap for blit\n"));
				}
			}
		}
	}
}
