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
 * $Id: datatypes_picture.c,v 1.14 2015/12/20 20:25:20 itix Exp $
 */

#include "ambient.h"

/* public */
#include <cybergraphx/cybergraphics.h>
#include <datatypes/pictureclass.h>
#include <datatypes/pictureclassext.h>
#include <proto/datatypes.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <proto/dos.h>

/* private */
#include "datatypes_picture.h"
#include "common_picture.h"
#include "mui_func.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "gfx_alpha.h"
#include "gfx_cmap.h"


APTR datatypes_picture_clone_bm(APTR ctx)
{
	struct common_picture *ct = ctx;
	struct BitMapHeader *bmh;
	APTR bm = NULL;

	ASSERT(ct);

	if (GetDTAttrs(ct->picture_o,
			PDTA_BitMapHeader, &bmh,
			TAG_DONE
			) == 1 && bmh)
	{
		if (bmh->bmh_Depth <= 8)
		{
			struct ColorMap *cm;
			ULONG *cregs, numcols;

			/*
			 * CLUT to ARGB32 conversion.
			 */
			if (GetDTAttrs(ct->picture_o,
				PDTA_NumColors, &numcols,
				PDTA_CRegs, &cregs,
				TAG_DONE) == 2)
			{
				if (numcols <= 256 && cregs)
				{
					if ( (cm = gfx_cmap_create(cregs, numcols)) )
					{
						if ( (bm = gfx_bitmap_create(bmh->bmh_Width, bmh->bmh_Height, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE)) )
						{
							if (gfx_cmap_remap(ct->bm, bm, cm))
							{
								gfx_alpha_set(bm, 0, 0, bmh->bmh_Width, bmh->bmh_Height, 0xff);
							}
							else
							{
								gfx_bitmap_delete(bm);
								bm = NULL;
							}
						}
						gfx_cmap_delete(cm);
					}
				}
			}
		}
		else
		{
			/*
			 * Some 16/24-bit to ARGB32 conversion. We set the
			 * alpha to the maximum value.
			 */
			if ( (bm = gfx_bitmap_create(bmh->bmh_Width, bmh->bmh_Height, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE)) )
			{
				gfx_blit(ct->bm, bm, TAG_DONE);
				gfx_alpha_set(bm, 0, 0, bmh->bmh_Width, bmh->bmh_Height, 0xff);
			}
		}
	}

	return (bm);
}


#define CDF_DISPLAYABLE (1 << 0UL)
#define CDF_ARGB32      (1 << 1UL)

APTR v_datatypes_picture_create(CONST_STRPTR filename, struct TagItem *tags)
{
	struct common_picture *dtp;
	ULONG flags = 0;
	struct Screen *scr = NULL;
	LONG *errptr = NULL;
	APTR dataptr = NULL;
	ULONG datasize = 0, blend = TRUE;

	THREAD;

	FORTAG(tags)
	{
		case PICTAG_Screen:
			scr = (struct Screen *)tag->ti_Data;
			break;

		case PICTAG_VMem:
			if (tag->ti_Data)
			{
				flags |= CDF_DISPLAYABLE;
			}
			break;

		case PICTAG_ARGB32:
			if (tag->ti_Data)
			{
				flags |= CDF_ARGB32;
			}
			break;

		case PICTAG_ErrorPtr:
			errptr = (LONG *)tag->ti_Data;
			break;

		case PICTAG_DataAddress:
			dataptr	= (APTR)tag->ti_Data;
			break;

		case PICTAG_DataSize:
			datasize = tag->ti_Data;
			break;

		case PICTAG_Blend:
			blend = tag->ti_Data;
			break;

		#ifdef DEBUG
		default:
			PDB(("no such tag 0x%lx\n", tag->ti_Tag));
			break;
		#endif
	}
	NEXTTAG

	if ( filename == NULL && ( dataptr == NULL || datasize == 0 ) )
	{
		PDB(("No filename or memory stream given\n"));
		return NULL;
	}

	if ( (dtp = malloc(sizeof(*dtp))) )
	{
		#if USE_DTFRIENDFORMAT
		APTR bmfriend;

		if (flags & CDF_ARGB32)
		{
			bmfriend = gfx_bitmap_create(32, 1, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE);
		}
		else
		{
			bmfriend = NULL;
		}
		#endif

		dtp->picture_o = NULL;
		dtp->bm = NULL;

		dtp->picture_o = NewDTObject((STRPTR)filename,
			DTA_GroupID, GID_PICTURE,
			DTA_SourceType, filename ? DTST_FILE : DTST_MEMORY,
			dataptr ? DTA_SourceAddress : TAG_IGNORE, dataptr,
			dataptr ? DTA_SourceSize : TAG_IGNORE, datasize,
			(flags & CDF_ARGB32) ? PDTA_Remap : TAG_IGNORE, TRUE,
			(flags & CDF_ARGB32) ? PDTA_AlphaChannel : TAG_IGNORE, TRUE,
			PDTA_FreeSourceBitMap, FALSE,
			OBP_Precision, PRECISION_EXACT,
			PDTA_DestMode, PMODE_V43,
			#if USE_DTFRIENDFORMAT
			PDTA_UseFriendBitMap, (bmfriend || scr) ? TRUE : FALSE,
			bmfriend ? PDTA_BitMap : TAG_IGNORE, gfx_bitmap_bm(bmfriend),
			(scr && !bmfriend) ? PDTA_Screen : TAG_IGNORE, scr,
			#else
			PDTA_UseFriendBitMap, scr ? TRUE : FALSE,
			scr ? PDTA_Screen : TAG_IGNORE, scr,
			#endif
			PDTA_Displayable, flags & CDF_DISPLAYABLE,
			TAG_DONE
		);

		#if USE_DTFRIENDFORMAT
		if (dtp->picture_o) /* don't deallocate if it failed, picture.datatype does it */
		{
			gfx_bitmap_delete(bmfriend);
		}
		#endif

		if (dtp->picture_o)
		{
			struct BitMapHeader *bmh;

			dtp->picture_delete      = datatypes_picture_delete;
			dtp->picture_set_bitmap  = datatypes_picture_set_bitmap;
			dtp->picture_errorstring = NULL;
			dtp->type = PICTURE_TYPE_DATATYPES;

			if (GetDTAttrs(dtp->picture_o, PDTA_BitMapHeader, &bmh, TAG_DONE) == 1)
			{
				int w = bmh->bmh_Width;
				int h = bmh->bmh_Height;

				if (bmh->bmh_Masking == mskHasAlpha)
				{
					APTR bm = gfx_bitmap_create(w, h, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, BITMAPTAG_Clear, TRUE, TAG_DONE);

					if (bm)
					{
						ULONG modulo = ((w * 4) + 15) & ~15;
						APTR buffer = AllocMemAligned(modulo * h, MEMF_ANY, 16, 0);

						if (buffer)
						{
							struct RastPort rp;

							DoMethod(dtp->picture_o, DTM_READPIXELARRAY, buffer, PBPAFMT_ARGB, modulo, 0, 0, w, h);

							InitRastPort(&rp);
							rp.BitMap = gfx_bitmap_bm(bm);

							if (blend)
								WritePixelArrayAlpha(buffer, 0, 0, modulo, &rp, 0, 0, w, h, 0xffffffff);
							else
								WritePixelArray(buffer, 0, 0, modulo, &rp, 0, 0, w, h, RECTFMT_ARGB);

							FreeMem(buffer, modulo * h);
							picture_set_bitmap(dtp, bm);
							return (dtp);
						}
						else
						{
							gfx_bitmap_delete(bm);
						}
					}
				}
				else
				{
					struct BitMap *dtbm;

					DoMethod(dtp->picture_o, DTM_PROCLAYOUT, NULL, TRUE);

					if (GetDTAttrs(dtp->picture_o,
						PDTA_DestBitMap, &dtbm,
						TAG_DONE
						) == 1)
					{
						if ( (dtp->bm = gfx_bitmap_create_from_native(dtbm, w, h)) )
						{
							if (flags & CDF_ARGB32 && (!GetCyberMapAttr(gfx_bitmap_bm(dtp->bm), CYBRMATTR_ISCYBERGFX) || GetCyberMapAttr(gfx_bitmap_bm(dtp->bm), CYBRMATTR_PIXFMT) != PIXFMT_ARGB32))
							{
								APTR bm = datatypes_picture_clone_bm(dtp);

								if (bm)
								{
									picture_set_bitmap(dtp, bm);
									return (dtp);
								}
							}
							else
							{
								return (dtp);
							}
							gfx_bitmap_delete(dtp->bm);
						}
					}
				}
			}

			DisposeDTObject(dtp->picture_o);
		}
		else
		{
			if (errptr)
			{
				*errptr = IoErr();
			}
		}
		free(dtp);
	}
	return (NULL);
}

void datatypes_picture_set_bitmap(APTR ctx, __unused APTR bm)
{
	struct common_picture *ct = ctx;

	DisposeDTObject(ct->picture_o);
}


void datatypes_picture_delete(APTR ctx)
{
	struct common_picture *ct = ctx;

	DisposeDTObject(ct->picture_o);
}
