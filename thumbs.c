/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber
 * Copyright 2005-2006 Ambient Open Source Team
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
 * $Id: thumbs.c,v 1.25 2025/07/23 23:54:26 geit Exp $
 */

#include "ambient.h"

#if USE_THUMBS

/* public */
#include <proto/dos.h>
#include <proto/thumbnails.h>
#include <libraries/thumbnails.h>

/* private */
#include "thumbs.h"
#include "datatypes.h"
#include "multimedia.h"
#include "datatypes_picture.h"
#include "gfx_bitmap.h"
#include "gfx_scale.h"
#include "gfx_blit.h"
#include "mui_func.h"
#include "methodstack.h"
#include "threads.h"
#include "layout.h"
#include "icondata.h"
#include "cache.h"
#include "hash.h"
#include "mimetype.h"
#include "mimeuri.h"
#include "str.h"
#include "avcodec.h"
#include "thumbs.h"
#include "time_func.h"

struct Library *ThumbnailsBase;

/*
 * Checks if a file can be thumbnailed.
 */
ULONG thumb_validate(STRPTR path)
{
	THREAD;

	if (datatypes_findtype(path) == DTTYPE_IMAGE)
	{
		return (TRUE);
	}
	else if (multimedia_findtype(path) == MULTIMEDIATYPE_VIDEO)
	{
		#warning "add reggae support to thumbnail.library" 
		return (FALSE);
	}
	
	return (FALSE);
	
}

ULONG picture_validate(STRPTR path, APTR mimetype)
{
	if(mimetype)
	{
		if(stristr( ((struct internal_mimetype_node *) mimetype)->mimetype, "image/") )
		{
			return (TRUE);
		}
		else if(!stricmp(((struct internal_mimetype_node *) mimetype)->mimetype, MIMETYPE_INTERNAL_MULTIMEDIA))
		{
			/* XXX: hm, i wonder if it's smart to try that, tho it will allow thumbs to work on filetypes-less setups */
			return thumb_validate(path);
		}
		else if(!stricmp(((struct internal_mimetype_node *) mimetype)->mimetype, MIMETYPE_INTERNAL_DATATYPES))
		{
			/* XXX: hm, i wonder if it's smart to try that, tho it will allow thumbs to work on filetypes-less setups */
			return thumb_validate(path);
		}
		else
		{
			return (FALSE);
		}
	}
	else
	{
		return thumb_validate(path);
	}
}

#define SCALE_SIZE 96 /* XXX: lame defaults.. */

APTR tr_thumb_createbitmap(STRPTR path, APTR mimetype UNUSED, ULONG flags)
{
	APTR bm = NULL;

	THREAD;
	ASSERT(path);

	if (flags & TCF_CREATE)
	{
		ULONG hash = 0;
		ULONG time0 = timedm();

		/* try to load from cache */

		{
			APTR cached_image;

			BPTR l = Lock(path, ACCESS_READ);

			if (l != (BPTR)NULL)
			{
				D_S(struct FileInfoBlock, fib);

				if (Examine(l, fib))
				{
					/*
					 * Check in cache.
					 */

					hash = hash_nocase_seconds(path, datestamp_to_seconds(&fib->fib_Date));
				}
				UnLock(l);
			}

			if (cache_get(hash, CACHETAG_THUMBNAIL, &cached_image))
			{
				ULONG *header = cached_image;

				bm = gfx_bitmap_create(header[ 0 ], header[ 1 ], 32,
					BITMAPTAG_Format, BITMAPVAL_Format_ARGB32,
					TAG_DONE);

				if (bm != NULL)
				{
					gfx_blit((UBYTE*)cached_image + 12, bm,
								BLITTAG_SrcType, BLITVAL_SrcType_Array,
								BLITTAG_DstWidth, header[ 0 ],
								BLITTAG_DstHeight, header[ 1 ],
								BLITTAG_SrcFormat, header[ 2 ] == 4 ? BLITVAL_SrcFormat_ARGB : BLITVAL_SrcFormat_RGB,
								TAG_DONE);
				}

				// we must unlock manually for CACHETAG_THUMBNAIL as soon as we're done - Piru
				cache_unlock();
			}
		}

		if (bm == NULL)
		{
			if(ThumbnailsBase != NULL)
			{
				struct TagItem tags[] = {{THB_Width, SCALE_SIZE}, {THB_Height, SCALE_SIZE}, {THB_Format, THB_FORMAT_ARGB8888}, {TAG_END, TAG_END}};
				APTR thumb = ThbGenerateThumbnailA(path, tags);

				if (thumb != NULL)
				{
					LONG width, height;
					LONG owidth, oheight, odepth;
					UBYTE *data;
					struct TagItem attrs[] = {{THB_Width, (ULONG)&width},
											  {THB_Height, (ULONG)&height},
											  {THB_Data, (ULONG)&data},
											  {THB_OriginalWidth, (ULONG)&owidth},
											  {THB_OriginalHeight, (ULONG)&oheight},
											  {THB_OriginalDepth, (ULONG)&odepth},
											  {TAG_END, TAG_END}};

					ThbGetAttrsA(thumb, attrs);

					bm = gfx_bitmap_create(width, height, 32,
						BITMAPTAG_Format, BITMAPVAL_Format_ARGB32,
						TAG_DONE);

					if (bm != NULL)
					{
						gfx_blit(data, bm,
					          BLITTAG_SrcType, BLITVAL_SrcType_Array,
					          BLITTAG_SrcFormat, BLITVAL_SrcFormat_ARGB,
							  BLITTAG_DstWidth, width,
							  BLITTAG_DstHeight, height,
							  BLITTAG_Modulo, width * 4,
					          TAG_DONE );
					}

					ThbDeleteThumbnail(thumb);
				}
			}

			/* insert into cache (not if thumbnail was generated in less than 1/10s) */

			if (bm && hash && (timedm() - time0) > 100)
				cache_set(hash, CACHETAG_THUMBNAIL, bm);
		}
	}
	return bm;
}

ULONG tr_thumb_createicon(APTR obj UNUSED, APTR o, STRPTR path, ULONG flags)
{
	THREAD;
	CHECKOBJECT(obj);
	CHECKOBJECT(o);
	ASSERT(path);

	if (flags & TCF_CREATE)
	{
		APTR mimetype = NULL;
		APTR bm;

		methodstack_push_sync(o, 3, OM_GET, MA_Icon_MimeType, &mimetype);
		bm = tr_thumb_createbitmap(path, mimetype, flags);

		if (bm != NULL)
		{
			methodstack_push(o, 4,
				MM_Icon_AddBitMap,
				bm,
				MV_Icon_BitMap_Thumbicon,
				MV_Icon_BitMap_Normal
			);

			/* XXX: hack! implement it in MUIM_Draw properly or so! */

			methodstack_push(o, 1, MM_Icon_UpdateBitMapBuffer);

			/* Execute methods */

			methodstack_push_sync(o, 2,	MM_Icon_Generate, TRUE);

			return (TRUE);
		}
	}
	return (FALSE);
}

ULONG tr_thumb_createicon_list(APTR obj, struct MinList *l, ULONG flags UNUSED)
{
	ULONG rc = TRUE;
	struct thumbnode *tn;

	ULONG abort = FALSE;
	LONG toppos = 0xffffff;	   /* NAN */

	THREAD;
	CHECKOBJECT(obj);
	ASSERT(l);

	tn = FIRSTNODE( l );

	while ( !ISLISTEMPTY( l ) )
	{
		if (threads_check_abort())
		{
			abort = TRUE;
			rc = ABORTED;
		}
		
		if (!abort)
		{
			LONG new_toppos;
			struct Rect32 display_area;

			/*
			 * Check if view scrolled. If it did we will recalculate
			 * first node to be scanned. Some better strategy?
			 */

			methodstack_push_sync( obj, 2, MM_View_QueryDisplayArea, &display_area );
			new_toppos = display_area.MinY;

			if ( new_toppos != toppos )
			{
				struct thumbnode *ttn;

				toppos = new_toppos;
				tn = NULL;

				/*
				 * Get them once here to not use pushmethod all the time.
				 */

				ITERATELIST( ttn, l )
				{
					if ( tn == NULL && methodstack_push_sync( obj, 2, MM_View_CheckFocus, ttn->o ) == TRUE )
					{
						tn = ttn;
						break;
					}
				}

				/*
				 * There is a case when all visible objects are already processed.
				 */

				if ( !tn )
				{
					tn = FIRSTNODE( l );
				}
			}
			#if 0
			if ( tr_thumb_createicon(obj, tn->o, tn->path, flags))
			{
				/*
				 * This will pack icons if thumbnails are shorter than default ones
				 * but also causes ugly flickering. I suggest to not relayout as
				 * this flickering is really not nice.
				 */
				//methodstack_push_sync(obj, 1, MM_Iconview_DoLayout); /* XXX: adjust that ? */
				//DoMethod( _app(obj), MUIM_Application_PushMethod, obj, 2, MM_Iconview_RedrawIcon, tn->o );
				methodstack_push_sync(obj, 2, MM_View_RedrawEntry, tn->o);
			}
			else
			{
				rc = FALSE;
			}
			#endif
		}

		methodstack_push(tn->o, 3,
			MUIM_Set,
			MA_Icon_ThumbIt,
			FALSE
		);

		/*
		 * If we reached end of list we need to check if we skipped some nodes.
		 */

		{
			struct thumbnode *ttn;

			if ( tn != LASTNODE( l ) )
			{
				ttn = NEXTNODE( tn );
			}
			else
			{
				ttn = FIRSTNODE( l );
			}

			REMOVE(tn);
			free(tn);
			tn = ttn;
		}
	}
	free(l);

	return (rc);
}

ULONG thumb_init(void)
{
	ThumbnailsBase = OpenLibrary("thumbnails.library", 50);
	return TRUE;
}

void thumb_cleanup(void)
{
	if (ThumbnailsBase != NULL)
		CloseLibrary(ThumbnailsBase);
}

#endif
