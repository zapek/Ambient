/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006 Ambient Open Source Team
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
 * $Id: image.c,v 1.7 2025/01/18 21:53:31 cyfm Exp $
 */

#include "ambient.h"

/* public */
#include <graphics/gfx.h>
#include <intuition/intuition.h>

/* private */
#ifdef BUILD_ICONLIB
#include <exec/memory.h>
#include <proto/icon.h>
#include <libraries/mui.h>
#include "classes.h"
#else
#include <proto/graphics.h>
#include "mui_func.h"
#include "gfx_mask.h"
#include "gfx_alpha.h"
#include "gfx_bitmap.h"
#include "gfx_cmap.h"
#include "colormap.h"
#endif
#include "image.h"
#include "methodstack.h"
#include "iconmem.h"
#include "file_io.h"
#include "iconio.h"



/*
 * Maximum pixel picture size
 */
#define IMAGE_MAX_X 512
#define IMAGE_MAX_Y 512
#define IMAGE_MAX_DEPTH 8

/*
 * Remaps an Image structure.
 */
#ifndef BUILD_ICONLIB
ULONG remap_image(APTR obj, ULONG state, struct Image *img, ULONG imgwidth, APTR imgdata)
{
	LONG i;
	struct BitMap sbm;
	APTR bm;
	APTR tbm;
	APTR mbm;
	UBYTE *planeptr;
	APTR p;

	/*
	 * We remap, although it would be smarter to know if it's a newicon to
	 * avoid that step, so XXX
	 */
	InitBitMap(&sbm, img->Depth, img->Width, img->Height);

	planeptr = (UBYTE *)imgdata;

	for (i = 0; i < img->Depth; i++)
	{
		if (img->PlanePick & (1 << i))
		{
			/* normal plane */
			sbm.Planes[i] = (APTR)planeptr;
			planeptr += imgwidth * img->Height;
		}
		else
		{
			if ( (p = icon_malloc(imgwidth * img->Height)) )
			{
				if (img->PlaneOnOff & (1 << i))
				{
					/* full 1 plane */
					memset(p, 0xff, imgwidth * img->Height);
				}
				else
				{
					/* full 0 plane */
					memset(p, 0, imgwidth * img->Height);
				}
				sbm.Planes[i] = p;
			}
			else
			{
				/* XXX */
			}
		}
	}

	if ( (bm = gfx_bitmap_create_from_native(&sbm, img->Width, img->Height)) )
	{
		if ( (mbm = gfx_mask_create_planar(bm, img->Width, img->Height, img->Depth)) )
		{
			if ( (tbm = gfx_bitmap_create(img->Width, img->Height, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE)) )
			{
				gfx_cmap_remap(bm, tbm, magicwb_cm);
			
				gfx_alpha_set_mask(tbm, 0, 0, img->Width, img->Height, mbm, 0xff, TRUE);

				methodstack_push_sync(obj, 4, MM_Icon_AddBitMap, tbm, MV_Icon_BitMap_Standard, state);
			}
			gfx_bitmap_delete(mbm);
		}
		gfx_bitmap_delete(bm);
	}

	for (i = 0 ; i < img->Depth ; i++)
	{
		if (!(img->PlanePick & (1 << i)))
		{
			icon_free(sbm.Planes[i]);
		}
	}
	return (TRUE); /* XXX: bogus.. */
}
#endif


/*
 * Reads an Image structure.
 */
#ifdef BUILD_ICONLIB
ULONG read_image(APTR fh, APTR obj, ULONG state, struct BitMap *fri, struct FreeList *fl)
#else
ULONG read_image(APTR fh, APTR obj, ULONG state, ULONG ancillary)
#endif
{
	struct Image img;
	ULONG retval = FALSE;
	ULONG val;

	THREAD;
	ASSERT(fh);
	CHECKOBJECT(obj);
	#ifdef BUILD_ICONLIB
	ASSERT(fl);
	#endif

	if (file_read(fh, &img, sizeof(img)))
	{
		APTR imgdata;
		ULONG imgwidth = RASSIZE(img.Width, 1);
		ULONG imgsize = imgwidth * img.Height * img.Depth;
		val = sizeof(struct Image);

		if ( (imgwidth && imgwidth <= IMAGE_MAX_X && img.Height && img.Height <= IMAGE_MAX_Y && img.Depth && img.Depth <= IMAGE_MAX_DEPTH) )
		{
			img.PlanePick  = (1UL << img.Depth)-1;	/* reset PlanePick/OnOff fields to fix some broken icons like legacy icon.library does */
			img.PlaneOnOff = 0;

			if ( (imgdata = icon_malloc(imgsize)) )
			{
				if (file_read(fh, imgdata, imgsize))
				{
					img.ImageData = imgdata;
					val += imgsize;
					D(ICONIO, bug("imgdata read (%ld bytes), width: %ld, height: %ld, depth: %ld\n", (LONG)imgsize, (LONG)img.Width, (LONG)img.Height, (LONG)img.Depth));
					retval = val;
					#ifdef BUILD_ICONLIB
					retval = val + imgsize;
					methodstack_push(obj, 5, MM_Icon_AddImage,
						state,
						&img, imgdata, imgsize
					);
					#else
					D(ICONIO, bug("read %ld of total data\n", val));
					if (ancillary)
					{
						methodstack_push_sync(obj, 4,
							MM_Icon_AddAncillary, (state == MV_Icon_BitMap_Normal) ? MV_Icon_Ancillary_ImageNormal : MV_Icon_Ancillary_ImageSelected, imgsize, &img
						);
					}
					remap_image(obj, state, &img, imgwidth, imgdata); /* XXX: check retcode.. */
					#endif /* !BUILD_ICONLIB */
				}
				else
				{
					methodstack_push(obj, 2,
						MM_Icon_ErrorString, "image; I/O error"
					);
				}
				icon_free(imgdata);
			}
			else
			{
				methodstack_push(obj, 2,
					MM_Icon_ErrorString, "image; not enough memory"
				);
			}
		}
		else
		{
			methodstack_push(obj, 2,
				MM_Icon_ErrorString, "image; bogus image specs"
			);
		}
	}
	else
	{
		methodstack_push(obj, 2,
			MM_Icon_ErrorString, "image; I/O error reading image"
		);
	}
	
	return (retval);
}
