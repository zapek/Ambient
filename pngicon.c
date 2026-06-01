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
 * $Id: pngicon.c,v 1.15 2025/07/24 01:26:38 geit Exp $
 */

#include "ambient.h"

#if USE_PNGICONS || defined(BUILD_ICONLIB)


/* public */
#include <proto/exec.h>
#if USE_SHARED_LIBPNG
#if _JBLEN != 59
/* shared libpng (png.library) requires use of very specific setjmp */
#define PNG_SKIP_SETJMP_CHECK 1
#endif
#include <proto/png.h>
#endif
#ifdef BUILD_ICONLIB
#include <cybergraphx/cybergraphics.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#endif

/* private */
#include "pngicon.h"
#include "iconmem.h"
#include "file_io.h"
#include "mui_func.h"
#include "methodstack.h"
#include "pngio.h"
#include "prefs.h"
#include "iconio.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#if !USE_SHARED_LIBPNG
#include "libs/pnglib/libpng/png.h"
#include "png_calls.h"
#endif

/*
 * Protos (yeah, we cannot use static in there !)
 */
png_voidp png_icon_malloc(png_structp png_ptr, png_size_t size);
void png_icon_free(png_structp png_ptr, png_voidp ptr);
void user_read_data(png_structp png_ptr, png_bytep data, png_uint_32 length);
void user_write_data(png_structp png_ptr, png_bytep data, png_uint_32 length);
void user_flush_data(png_structp png_ptr);
int read_chunk_callback(png_structp png_ptr, png_unknown_chunkp chunk);

#ifndef BUILD_ICONLIB
/*
 * Memory allocation stubs.
 */
png_voidp png_icon_malloc(png_structp png_ptr UNUSED, png_size_t size)
{
	return (icon_malloc(size));
}

void png_icon_free(png_structp png_ptr UNUSED, png_voidp ptr)
{
	icon_free(ptr);
}


void user_write_data(png_structp png_ptr UNUSED, png_bytep data UNUSED, png_uint_32 length UNUSED)
{

}

void user_flush_data(png_structp png_ptr UNUSED)
{

}

#else

png_voidp png_icon_malloc(png_structp png_ptr UNUSED, png_size_t size)
{
	return (AllocVecTaskPooled(size));
}

void png_icon_free(png_structp png_ptr UNUSED, png_voidp ptr)
{
	if (ptr)
		FreeVecTaskPooled(ptr);
}

#endif

/*
 * I/O stubs.
 */
void user_read_data(png_structp png_ptr, png_bytep data, png_uint_32 length)
{
	APTR fh = png_get_io_ptr(png_ptr);

	if (!file_read(fh, data, length))
	{
		/* XXX: print some error message there.. */
		PDB(("argh! failed to read\n"));
		png_error(png_ptr, "Read error");
	}
}

/*
 * Subroutine. Used by iconlib too.
 * XXX: retval maybe..
 */
#define ADVANCE_DATA(x) data += (x); len -= (x)
#ifdef BUILD_ICONLIB
void pngio_read_tags(APTR obj, UBYTE *data, ULONG len, struct FreeList *fl)
#else
static void pngio_read_tags(APTR obj, UBYTE *data, ULONG len)
#endif
{
	struct TagItem *tag;
	ULONG did_setpos = FALSE;
	ULONG did_setdrawer = FALSE;
	/* XXX: we don't check for duplicate tags.. is it needed ? */
	
	while (len >= sizeof(struct TagItem *))
	{
		tag = (struct TagItem *)data;

		ADVANCE_DATA(sizeof(struct TagItem *));

		//dprintf("tag: %p\n", tag->ti_Tag);

		switch (tag->ti_Tag)
		{
			case PNGICON_LocationX:
				methodstack_push(obj, 3,
					MUIM_Set,
					MA_Icon_X, tag->ti_Data
				);
				if (!did_setpos)
				{
					methodstack_push(obj, 3,
						MUIM_Set,
						MA_Icon_HasPos, TRUE
					);
					did_setpos = TRUE;
				}
				ADVANCE_DATA(sizeof(ULONG));
				break;

			case PNGICON_LocationY:
				methodstack_push(obj, 3,
					MUIM_Set,
					MA_Icon_Y, tag->ti_Data
				);
				if (!did_setpos)
				{
					methodstack_push(obj, 3,
						MUIM_Set,
						MA_Icon_HasPos, TRUE
					);
					did_setpos = TRUE;
				}
				ADVANCE_DATA(sizeof(ULONG));
				break;

			case PNGICON_DrawerLeft:
				methodstack_push(obj, 3,
					MUIM_Set,
					MA_Icon_WindowLeft, tag->ti_Data
				);
				if (!did_setdrawer)
				{
					methodstack_push(obj, 3,
						MUIM_Set,
						MA_Icon_HasDrawerData, TRUE
					);
					did_setdrawer = TRUE;
				}
				ADVANCE_DATA(sizeof(ULONG));
				break;

			case PNGICON_DrawerTop:
				methodstack_push(obj, 3,
					MUIM_Set,
					MA_Icon_WindowTop, tag->ti_Data
				);
				if (!did_setdrawer)
				{
					methodstack_push(obj, 3,
						MUIM_Set,
						MA_Icon_HasDrawerData, TRUE
					);
					did_setdrawer = TRUE;
				}
				ADVANCE_DATA(sizeof(ULONG));
				break;

			case PNGICON_DrawerWidth:
				methodstack_push(obj, 3,
					MUIM_Set,
					MA_Icon_WindowWidth, tag->ti_Data
				);
				if (!did_setdrawer)
				{
					methodstack_push(obj, 3,
						MUIM_Set,
						MA_Icon_HasDrawerData, TRUE
					);
					did_setdrawer = TRUE;
				}
				ADVANCE_DATA(sizeof(ULONG));
				break;

			case PNGICON_DrawerHeight:
				methodstack_push(obj, 3,
					MUIM_Set,
					MA_Icon_WindowHeight, tag->ti_Data
				);
				if (!did_setdrawer)
				{
					methodstack_push(obj, 3,
						MUIM_Set,
						MA_Icon_HasDrawerData, TRUE
					);
					did_setdrawer = TRUE;
				}
				ADVANCE_DATA(sizeof(ULONG));
				break;

			case PNGICON_DrawerViewMode:
				methodstack_push(obj, 3,
					MUIM_Set,
					MA_Icon_ViewMode, tag->ti_Data
				);
				if (!did_setdrawer)
				{
					methodstack_push(obj, 3,
						MUIM_Set,
						MA_Icon_HasDrawerData, TRUE
					);
					did_setdrawer = TRUE;
				}
				ADVANCE_DATA(sizeof(ULONG));
				break;

			case PNGICON_DrawerSortMode:
				methodstack_push(obj, 3,
					MUIM_Set,
					MA_Icon_SortMode, tag->ti_Data
				);
				if (!did_setdrawer)
				{
					methodstack_push(obj, 3,
						MUIM_Set,
						MA_Icon_HasDrawerData, TRUE
					);
					did_setdrawer = TRUE;
				}
				ADVANCE_DATA(sizeof(ULONG));
				break;

			case PNGICON_StackSize:
				methodstack_push(obj, 3,
					MUIM_Set,
					MA_Icon_StackSize, tag->ti_Data
				);
				ADVANCE_DATA(sizeof(ULONG));
				break;

			/* XXX: we should have a special strlen() that stops after a certain size or so.. */
			case PNGICON_DefaultTool:
				{
					ULONG size;
					ULONG filetype;

					methodstack_push_sync(obj, 3,
						OM_GET,
						MA_Icon_FileType, &filetype
					);

					methodstack_push(obj, 3,
						MUIM_Set,
						MA_Icon_Type, MV_Icon_Type_Project
					);

					size = strlen((STRPTR)&tag->ti_Data) + 1;
					methodstack_push_sync(obj, 3,
						MUIM_Set,
						MA_Icon_DefaultTool, &tag->ti_Data
					);
					ADVANCE_DATA(size);
				}
				break;

			case PNGICON_ToolType:
				{
					ULONG size;
					size = strlen((STRPTR)&tag->ti_Data) + 1;
					D(ICONIO,bug("inserting PNGicon tooltype <%s>, size: %ld\n", &tag->ti_Data, size));
					methodstack_push_sync(obj, 3,
						MM_Icon_InsertToolType, size, &tag->ti_Data
					); /* XXX: check retval? hm.. */
					ADVANCE_DATA(size);
				}
				break;
		}
	}
}

#ifndef BUILD_ICONLIB

/*
 * Handle our private chunk.
 */
int read_chunk_callback(png_structp png_ptr, png_unknown_chunkp chunk)
{
	D(ICONIO,bug("there..\n"));
	if (chunk->name[0] == 'i'
		&& chunk->name[1] == 'c'
		&& chunk->name[2] == 'O'
		&& chunk->name[3] == 'n'
	)
	{
		pngio_read_tags(png_get_user_chunk_ptr(png_ptr), chunk->data, chunk->size);
		// return (-1); /* XXX: chunk had an error.. will that make libpng fail? */
		return (1); /* chunk was ok */
	}
	else
	{
		return (0);
	}
}

#endif


#ifndef CTABFMT_RGB8 /* XXX: fix that mess in the cgx includes.. and in glowicon/newicon.c too :) */
#define CTABFMT_RGB8 2
#endif

#ifdef BUILD_ICONLIB
static ULONG pngicon_readimage(png_structp png_ptr, png_infop info_ptr, APTR obj, ULONG width, ULONG height, struct FreeList *fl, ULONG imagetype)
{
	ULONG **row_ptr;
	struct BitMap *bm;
	ULONG is32;

	is32 = (info_ptr->pixel_depth == 32);

	row_ptr = (ULONG **)png_get_rows(png_ptr, info_ptr);

	if ( (bm = AllocBitMap(width, height, 32, BMF_MINPLANES | BMF_SPECIALFMT | SHIFT_PIXFMT(PIXFMT_ARGB32), NULL)) )
	{
		struct RastPort rp;
		ULONG i;
		ULONG bytesperrow = png_get_rowbytes(png_ptr, info_ptr);

		InitRastPort(&rp);
		rp.BitMap = bm;

		/*
		 * This sucks but it looks like libpng doesn't set bytesperrow properly
		 * and adds some padding anyway.
		 */
		for (i = 0; i < info_ptr->height; i++)
		{
			WritePixelArray(row_ptr[i], 0, 0, bytesperrow, &rp, 0, i, width, 1, is32 ? RECTFMT_ARGB : RECTFMT_RGB);
		}

		methodstack_push(obj, 6,
			MM_Icon_AddBitMap, bm, width, height, MV_Icon_BitMap_PNGicon, imagetype == MV_Icon_BitMap_Normal ? MV_Icon_BitMap_Normal : MV_Icon_BitMap_Selected
		);
		return (TRUE);
	}
	return (FALSE);
}
#else
static ULONG pngicon_readimage(png_structp png_ptr, png_infop info_ptr, APTR obj, ULONG width, ULONG height, ULONG imagetype)
{
	ULONG **row_ptr;
	APTR bm;
	ULONG is32;
	
	is32 = (info_ptr->pixel_depth == 32);

	row_ptr = (ULONG **)png_get_rows(png_ptr, info_ptr);

	if ( (bm = gfx_bitmap_create(width, height, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE)) )
	{
		ULONG i;

		/*
		 * This sucks but it looks like libpng doesn't set bytesperrow properly
		 * and adds some padding anyway.
		 */
		for (i = 0; i < info_ptr->height; i++)
		{
			gfx_blit(row_ptr[i], bm,
				BLITTAG_SrcType, BLITVAL_SrcType_Array,
				BLITTAG_DstY, i,
				BLITTAG_DstHeight, 1,
				BLITTAG_SrcFormat, is32 ? BLITVAL_SrcFormat_ARGB : BLITVAL_SrcFormat_RGB,
			TAG_DONE);
		}

		methodstack_push(obj, 4,
			MM_Icon_AddBitMap, bm, MV_Icon_BitMap_PNGicon, imagetype == MV_Icon_BitMap_Normal ? MV_Icon_BitMap_Normal : MV_Icon_BitMap_Selected
		);
		return (TRUE);
	}
	return (FALSE);
}
#endif



#ifdef BUILD_ICONLIB
static ULONG pngicon_read2(APTR fh, APTR obj, struct FreeList *fl, ULONG imagetype)
#else
static ULONG pngicon_read2(APTR fh, APTR obj, ULONG end UNUSED, ULONG getimage, ULONG imagetype)
#endif
{
	volatile ULONG retval = FALSE;
	png_structp png_ptr;
		
	THREAD;

	if ( (png_ptr = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL, NULL, png_icon_malloc, png_icon_free)) ) /* XXX: set error funcptrs */
	//if (png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) // keep that line when I decide to fix the memtracker one day.. see ambient.todo
	{
		png_infop info_ptr;

		D(ICONIO,bug("read_struct created\n"));
		if ( (info_ptr = png_create_info_struct(png_ptr)) )
		{
			png_infop end_info;

			D(ICONIO,bug("info_struct created\n"));
			if ( (end_info = png_create_info_struct(png_ptr)) )
			{
#if USE_SHARED_LIBPNG && _JBLEN != 59
				/* Call compatible setjmp function with shared libpng (png.library) */
				if (setjmp59(png_jmpbuf(png_ptr)))
#else
				if (setjmp(png_jmpbuf(png_ptr)))
#endif
				{
					DB(("PNG fatal error\n"));
					png_destroy_read_struct((APTR)&png_ptr, &info_ptr, &end_info);
						
					/* XXX: this is tricky.. beware */

					return (FALSE);
				}

				D(ICONIO,bug("setting read function..\n"));
				png_set_read_fn(png_ptr, fh, (png_rw_ptr)user_read_data);

				#ifndef BUILD_ICONLIB
				png_set_sig_bytes(png_ptr, 8);

				if (imagetype == MV_Icon_BitMap_Normal)
				{
					D(ICONIO,bug("setting user chunk reading..\n"));
					png_set_read_user_chunk_fn(png_ptr, obj, read_chunk_callback);
				}

				png_set_keep_unknown_chunks(png_ptr, 2, 0, 0); /* libpng sucks.. that's the only way I can get it to work */
				#endif

				#ifndef BUILD_ICONLIB
				if (getimage)
				#endif
				{
					png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_SWAP_ALPHA | PNG_TRANSFORM_EXPAND, NULL);

					/*
					 * We want RGB or ARGB only.
					 */
					D(ICONIO,bug("got everything: width: %ld, height: %ld, depth: %ld, color type: %ld\n", info_ptr->width, info_ptr->height, (ULONG)info_ptr->pixel_depth, (ULONG)info_ptr->color_type));
					if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE
						|| info_ptr->color_type == PNG_COLOR_TYPE_RGB
						|| info_ptr->color_type == PNG_COLOR_TYPE_RGB_ALPHA
						|| info_ptr->color_type == PNG_COLOR_TYPE_GRAY_ALPHA /* XXX: not sure about that grey alpha one.. */
					)
					{
						D(ICONIO,bug("reading image..\n"));
						#ifdef BUILD_ICONLIB
						if (info_ptr->color_type != PNG_COLOR_TYPE_PALETTE) /* jaca wants to be able to load paletted stuff, so we fail and he can retry with datatypes */
						{
							pngicon_readimage(png_ptr, info_ptr, obj, info_ptr->width, info_ptr->height, fl, imagetype); /* XXX: check retcode */
						}
						/* XXX: do *NOT* return failure here! the icon still has to load, it just won't have picture */
						#else
						pngicon_readimage(png_ptr, info_ptr, obj, info_ptr->width, info_ptr->height, imagetype); /* XXX: check retcode */
						#endif

						D(ICONIO,bug("all done\n"));

						retval = TRUE;
					}
					else
					{
						D(ICONIO,bug("this is not a PNG supported format\n"));
						/* XXX */
					}
				}
				#ifndef BUILD_ICONLIB
				else
				{
					/* XXX: there must be better way but I cant figure it out... */

					D(ICONIO,bug("load structure but ignore image data\n"));
					png_read_png(png_ptr, info_ptr, 0, NULL);
					retval = TRUE;
				}
				#endif

				png_destroy_read_struct((APTR)&png_ptr, &info_ptr, &end_info);
			}
			else
			{
				png_destroy_read_struct((APTR)&png_ptr, &info_ptr, NULL);
			}
		}
		else
		{
			png_destroy_read_struct((APTR)&png_ptr, NULL, NULL);
		}
	}
	return (retval);
}

#endif

#ifdef BUILD_ICONLIB

ULONG pngicon_read(APTR fh, APTR obj, struct FreeList *fl)
{
	ULONG retval = pngicon_read2(fh, obj, fl, MV_Icon_BitMap_Normal);

	if (retval)
	{
		UBYTE buf[8];

     		/* XXX: Looks like it doesn't work for ICONLIB build (fab) */
		if (file_read(fh, buf, 8) && pngio_sigvalid(buf))
		{
			pngicon_read2(fh, obj, fl, MV_Icon_BitMap_Selected);
		}
	}

	return retval;
}
#else
ULONG pngicon_read(APTR fh, APTR obj, ULONG end, ULONG getimage)
{
	ULONG retval = pngicon_read2(fh, obj, FALSE, getimage, MV_Icon_BitMap_Normal);

	if (retval)
	{
		if (getimage && _conf(icon_dualpng))
		{
			UBYTE buf[8];

			if (file_read(fh, buf, 8) && pngio_sigvalid(buf))
			{
				pngicon_read2(fh, obj, FALSE, getimage, MV_Icon_BitMap_Selected);
			}
		}

		if (end)
			methodstack_push_sync(obj, 1, MM_Icon_End);
	}

	return retval;
}
#endif

