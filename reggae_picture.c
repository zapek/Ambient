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
 * $Id: reggae_picture.c,v 1.16 2017/08/11 23:32:05 cyfm Exp $
 */

#include "ambient.h"

/* public */
#include <cybergraphx/cybergraphics.h>
#include <datatypes/pictureclass.h>
#include <proto/datatypes.h>
#include <proto/cybergraphics.h>
#include <proto/multimedia.h>
#include <classes/multimedia/video.h>
#include <proto/graphics.h>
#include <proto/dos.h>

/* private */
#include "reggae_picture.h"
#include "mui_func.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "gfx_alpha.h"
#include "gfx_cmap.h"
#include "dos_internal.h"
#include "multimedia.h"
#include "gfx.h"
#include "common_picture.h"

static void reggae_picture_set_bitmap(APTR ctx, APTR bm);
static void reggae_picture_delete(APTR ctx);


#define CDF_DISPLAYABLE (1 << 0UL)
#define CDF_ARGB32      (1 << 1UL)

APTR v_reggae_picture_create(CONST_STRPTR filename, struct TagItem *tags)
{
	struct common_picture *rtp;
	ULONG flags = 0;
	LONG *errptr = NULL;
	APTR dataptr = NULL;
	ULONG datasize = 0, blend = TRUE;

	THREAD;

	FORTAG(tags)
	{
		case PICTAG_VMem:
			if (tag->ti_Data)
			{
				flags |= CDF_DISPLAYABLE;
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

	if (multimedia_open())
	{
		if ( (rtp = malloc(sizeof(*rtp))) )
		{
			struct TagItem tags[10];
			ULONG reggae_error = 0;
			APTR obj;
			
			tags[0].ti_Tag = MMA_StreamType;
			tags[0].ti_Data = (ULONG)"file.stream";
			tags[1].ti_Data = (ULONG)filename;
			tags[1].ti_Tag = MMA_StreamName;
			tags[2].ti_Tag = MMA_MediaType;
			tags[2].ti_Data = MMT_PICTURE,
			tags[3].ti_Tag = MMA_ErrorCode;
			tags[3].ti_Data = (ULONG)&reggae_error;
			tags[3].ti_Tag = TAG_END;
			tags[3].ti_Data = 0;

			if((obj = MediaNewObjectTagList(tags)))
			{
				APTR bm = NULL;
			
				LONG w = MediaGetPort(obj, 0, MMA_Video_Width);
				LONG h = MediaGetPort(obj, 0, MMA_Video_Height);
				LONG a = MediaGetPort(obj, 0, MMA_Video_UseAlpha);
		
				if ((bm = gfx_bitmap_create(w, h, 32,
					BITMAPTAG_Format, BITMAPVAL_Format_ARGB32,
					BITMAPTAG_Clear, a, TAG_DONE)))
				{
					CHECKARGB32(gfx_bitmap_bm(bm));
					
					if (a)
					{
						ULONG modulo = w * 4;
						APTR buffer = AllocMemAligned(modulo * h, MEMF_ANY, 16, 0);

						if (buffer)
						{
							struct RastPort rp;

							DoMethod(obj, MMM_Pull, 0, buffer, modulo * h);
							InitRastPort(&rp);
							rp.BitMap = gfx_bitmap_bm(bm);

							if (blend)
								WritePixelArrayAlpha(buffer, 0, 0, modulo, &rp, 0, 0, w, h, 0xffffffff);
							else
								WritePixelArray(buffer, 0, 0, modulo, &rp, 0, 0, w, h, RECTFMT_ARGB);

							FreeMem(buffer, modulo * h);
						}
					}
					else
					{
						size_t addr, bpr;
						APTR handle;

						if ((handle = LockBitMapTags(gfx_bitmap_bm(bm),
							LBMI_BASEADDRESS, &addr,
							LBMI_BYTESPERROW, &bpr, TAG_DONE)))
						{
							ULONG y;

							for (y = 0; y < h; y++)
							{
								DoMethod(obj, MMM_Pull, 0, addr, w*4);
								addr += bpr;
							}

							UnLockBitMap(handle);
						}
					}

					DisposeObject(obj);

					rtp->picture_o = NULL;
					rtp->bm = bm;
					rtp->picture_delete      = reggae_picture_delete;
					rtp->picture_set_bitmap  = reggae_picture_set_bitmap;
					rtp->picture_errorstring = NULL;
					rtp->type = PICTURE_TYPE_REGGAE;
					return rtp;
				}
				DisposeObject(obj);
			}
			else
			{
				if (errptr)
				{
					*errptr = reggae_error;
				}
			}
			free(rtp);
		}
		multimedia_close();
	}
	return (NULL);
}


static void reggae_picture_delete(APTR ctx)
{
	struct common_picture *ct = ctx;

	DisposeObject(ct->picture_o);
	multimedia_close();
}


static void reggae_picture_set_bitmap(APTR ctx, __unused APTR bm)
{
	struct common_picture *ct = ctx;
	
	DisposeObject(ct->picture_o);
	multimedia_close();
}
