/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2005-2015 Ambient Open Source Team
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
 * $Id: imagecache.c,v 1.14 2015/12/20 20:25:20 itix Exp $
 */

#include "ambient.h"

/* public */

#include <proto/dos.h>

/* private */

#include "imagecache.h"
#include "gfx.h"
#include "gfx_bitmap.h"
#include "datatypes.h"
#include "datatypes_picture.h"
#include "common_picture.h"
#include "gfx_scale.h"
#include "gfx_blit.h"

/* default images */

#include "toolbar_prev_logo.h"
#include "toolbar_next_logo.h"
#include "toolbar_up_logo.h"
#include "toolbar_search_logo.h"
#include "def_tool_logo.h"
#include "def_drawer_logo.h"
#include "def_disk_logo.h"

struct imagenode
{
	struct Node node;
	APTR        bitmap;
	UBYTE       name[0];
};

static struct MinList image_list;

ULONG imagecache_init( void )
{
	/* initialize list. nothing else to do here i think */

	NEWLIST( &image_list );

	return TRUE;
}

void imagecache_cleanup( void )
{
	/* delete all bitmaps allocated */

	struct imagenode *n, *nextn;

	ITERATELISTSAFE(n, nextn, &image_list)
	{
		gfx_bitmap_delete( n->bitmap );
		free( n );
	}
}

static APTR imagecache_loadimage( CONST_STRPTR path, ULONG height )
{
	APTR dtp;
	LONG err = 0;

	ASSERT(path);
	ASSERT(height);

	if ((dtp = datatypes_picture_create(path,
		PICTAG_ARGB32, TRUE,
		PICTAG_ErrorPtr, &err,
		PICTAG_Blend, FALSE,
		TAG_DONE)))
	{
		ULONG rc = FALSE;
		APTR bm;
		ULONG xs, ys;

		/* calculate dimensions */

		xs = 999; /* = don't care about width */
		ys = height;

		gfx_scale_calc_aspect_constraints(
			(ULONG)picture_getattr(dtp, PICTURE_WIDTH),
			(ULONG)picture_getattr(dtp, PICTURE_HEIGHT),
			&xs,
			&ys
		);

		if ((bm = gfx_bitmap_create(xs, ys, 32,
			BITMAPTAG_Format, BITMAPVAL_Format_ARGB32,
			TAG_DONE)))
		{
			if (gfx_scale(picture_getattr(dtp, PICTURE_BITMAP), bm, xs, ys,
				SCALETAG_Bilinear, TRUE,
				SCALETAG_Average, TRUE,
			TAG_DONE))
			{
				rc = TRUE;
			}
		}
		picture_delete(dtp);

		if (rc)
			return (bm);

		/* error */

		gfx_bitmap_delete(bm);
	}

	return (NULL);
}

APTR imagecache_getbitmap( CONST_STRPTR name, ULONG height )
{
	/* scan the list to check if we already don't have suck image */

	struct imagenode *n;

	ASSERT( name );
	ASSERT( height );

	ITERATELIST( n, &image_list )
	{
		if ( !strcmp( n->name, name ) )
		{
			if ( n->bitmap )
			{
				if ( gfx_bitmap_height( n->bitmap ) == height )
				{
					/* found matching image */

					return n->bitmap;
				}
			}
		}
	}

	/* we need new image. */

	{
		APTR bitmap = NULL;

		/* try to load image from current skin directory */

		#if !USE_LEGACY
// #warning "Uncompleted 1.5 code fragment..disabled...needs to be checked"
#if 0
		struct Screen *scr = LockPubScreen(NULL);

		if ( scr )
		{
			struct DrawInfo *di;

			if (di = GetScreenDrawInfo( scr ) )
			{
				STRPTR skin_path;

				...finish it, since it doesn't work on 1.4 for now it's left as it is....
#endif
		#endif

		/* try to load image from PROGDIR:data/images */

		if ( !bitmap )
		{
			ULONG plen = sizeof( "PROGDIR:images/" ) - 1 + strlen( name ) + 1;
			STRPTR path = malloc( plen );
			if ( path )
			{
				strcpy( path, "PROGDIR:images/" );
				AddPart( path, name, plen );

				bitmap = imagecache_loadimage( path, height );

				free( path );
			}
		}

		/* use builtin image (XXX: use some macro to simplify) */

		if ( !bitmap )
		{
			ULONG im_width = 0;
			ULONG im_height = 0;
			APTR im_data = NULL;
			APTR tmp_bitmap = NULL;

			/* handle builtin images */

			if ( !strcmp( name, "prev" ) )
			{
					im_width = TOOLBAR_PREV_WIDTH;
					im_height = TOOLBAR_PREV_WIDTH;
					im_data	= toolbar_prev;
			}
			else if ( !strcmp( name, "next" ) )
			{
					im_width = TOOLBAR_NEXT_WIDTH;
					im_height = TOOLBAR_NEXT_WIDTH;
					im_data	= toolbar_next;
			}
			else if ( !strcmp( name, "up" ) )
			{
					im_width = TOOLBAR_UP_WIDTH;
					im_height = TOOLBAR_UP_WIDTH;
					im_data	= toolbar_up;
			}
			else if ( !strcmp( name, "search" ) )
			{
					im_width = TOOLBAR_SEARCH_WIDTH;
					im_height = TOOLBAR_SEARCH_WIDTH;
					im_data	= toolbar_search;
			}
			else if ( !strcmp( name, "tool" ) )
			{
					im_width = DEF_TOOL_WIDTH;
					im_height = DEF_TOOL_HEIGHT;
					im_data	= def_tool;
			}
			else if ( !strcmp( name, "drawer" ) )
			{
					im_width = DEF_DRAWER_WIDTH;
					im_height = DEF_DRAWER_HEIGHT;
					im_data	= def_drawer;
			}
			else if ( !strcmp( name, "disk" ) )
			{
					im_width = DEF_DISK_WIDTH;
					im_height = DEF_DISK_HEIGHT;
					im_data	= def_disk;
			}
			else /*default */
			{
					im_width = TOOLBAR_PREV_WIDTH;
					im_height = TOOLBAR_PREV_WIDTH;
					im_data	= toolbar_prev;
			}

			/* build bitmap from data */

			if ((tmp_bitmap = gfx_bitmap_create( im_width, im_height, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE)))
			{
				ULONG xs, ys;

				gfx_blit( im_data, tmp_bitmap,
					BLITTAG_SrcType, BLITVAL_SrcType_Array,
					BLITTAG_SrcFormat, BLITVAL_SrcFormat_ARGB,
					TAG_DONE);

				/* calculate dimensions */

				xs = 999; /* = don't care about width */
				ys = height;

				gfx_scale_calc_aspect_constraints( im_width, im_height,	&xs, &ys );

				/* scale */

				if ((bitmap = gfx_bitmap_create( xs, ys, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE)))
				{
					if (!gfx_scale( tmp_bitmap, bitmap, xs, ys,
						SCALETAG_Bilinear, TRUE,
						SCALETAG_Average, TRUE,
					TAG_DONE))
					{
						/* We failed to scale image. Probably are very low on memory (will we crash soon?:) */

						gfx_bitmap_delete( bitmap );
						bitmap = NULL;
					}
				}

				gfx_bitmap_delete( tmp_bitmap );
			}
		}

		/* If we still don't have image loaded, then we can't do much more about it */

		if ( bitmap )
		{
			/* build node, setup, add to list and return bitmap */
			ULONG nlen = strlen(name) + 1;
			struct imagenode *imagenode = malloc( sizeof( *imagenode ) + nlen);

			if ( imagenode )
			{
				imagenode->bitmap = bitmap;
				memcpy(imagenode->name, name, nlen);
				ADDTAIL( &image_list, imagenode );
			}
		}

		return bitmap;
	}

	return (NULL);
}

void imagecache_releasebitmap(APTR bitmap UNUSED)
{
	/*
	 * XXX: Implement. Harmless while being unimplemented tho.
	 */

}
