/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2017 Ambient Open Source Team
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
 * $Id: typescanner.c,v 1.19 2021/12/31 18:01:28 piru Exp $
 */

#include "ambient.h"

/* public */
#include <proto/dos.h>
#include <hardware/atomic.h>

/* private */
#include "mui_func.h"
#include "name.h"
#include "methodstack.h"
#include "threads.h"
#include "typescanner.h"
#include "mimeuri.h"
#include "mimetype.h"
#include "hash.h"
#include "cache.h"
#include "deficonpool.h"
#include "thumbs.h"
#include "avcodec.h"
#include "time_func.h"
#include "icondata.h"


APTR gettype(CONST_STRPTR path, ULONG seconds, CONST_STRPTR mimetypepattern)
{
	return typescanner_get(path, seconds, mimetypepattern, FALSE);
}

APTR typescanner_get(CONST_STRPTR path, ULONG seconds, CONST_STRPTR mimetypepattern, BOOL no_cache)
{
	struct internal_mimetype_node *mimetype = NULL;

	ULONG hash = 0;
	BPTR l = 0;

	/*
	 * Check in cache. We need file date for that.
	 */

	//DB(("mimetype lookup for <%s>\n", path ));

	if (seconds || (l = Lock(path, ACCESS_READ)))
	{
		D_S(struct FileInfoBlock, fib);

		if (seconds || Examine(l, fib))
		{
			/*
			 * Check in cache.
			 */
			if (!seconds)
				hash = hash_nocase_seconds(path, datestamp_to_seconds(&fib->fib_Date));
			else
				hash = hash_nocase_seconds(path, seconds);

			if (!no_cache)
				cache_get(hash, CACHETAG_MIMETYPE, (APTR)&mimetype);
		}
		UnLock(l);
	}

	if (mimetype == NULL)
	{
		//DB(("no cached mimetype for <%s>\n", path));
		mimetype = mimetype_find_pattern("file://", path, MTF_FILEIO, mimetypepattern);

		if (hash && mimetype)
		{
			/*
			 * Store in cache.
			 */

			cache_set(hash, CACHETAG_MIMETYPE, mimetype);
		}
	}
    else if ( mimetype != NULL && mimetypepattern != NULL )
	{
		/*
		 * Found in cache, check if it matches patern.
		 */

		if (name_match( mimetype->mimetype, mimetypepattern ) == FALSE)
			mimetype = NULL;
	}

	if (mimetype != NULL)
	{
		//DB(("Type for <%s> is <%s>\n",path, ((struct internal_mimetype_node*)mimetype)->description ));
	}
	else
	{
		//DB(("No type found\n"));
	}

	return mimetype;
}


/*
 * We get a list of nodes containing object. For each we should analyze it's type and set
 * it to object.
 */

/* plain traverse */
#if 0
ULONG tr_typescanner_scan_list(APTR obj, struct MinList *l)
{
	ULONG rc = TRUE;
	struct typescannernode *tn, *nexttn;
	ULONG abort = FALSE;

	THREAD;
	CHECKOBJECT(obj);
	ASSERT(l);

	ITERATELISTSAFE(tn, nexttn, l)
	{
		if (threads_check_abort())
		{
			abort = TRUE;
			rc = ABORTED;
		}
		
		if (!abort)
		{
			APTR mimetype;

			mimetype = gettype( tn->path, 0, NULL );

			if ( mimetype )
			{
				methodstack_push_sync(tn->obj, 3,
					MUIM_Set,
					MA_Icon_MimeType, mimetype
				);
				methodstack_push_sync(tn->obj, 1, MM_ListviewEntry_Redraw);
			}
		}
		free(tn);
	}
	free(l);

	return (rc);
}
#endif

/* smart traverse, but linked to listview attributes */
ULONG tr_typescanner_scan_list(APTR obj, struct MinList *l, ULONG gen_deficons, ULONG gen_thumbs)
{
	ULONG rc = TRUE;
	struct typescannernode *tn;

	ULONG abort = FALSE;
	LONG toppos = 0xffffff;	   /* NAN */
	LONG n = 0;
	LONG new_toppos;
	LONG isiconview = FALSE;

	THREAD;
	CHECKOBJECT(obj);
	ASSERT(l);

	/* some actions work only for icons */

	methodstack_push_sync(obj, 3, OM_GET, MA_View_Type, &isiconview);
	isiconview = isiconview == MV_View_Type_Icon ? TRUE : FALSE;

	tn = FIRSTNODE(l);

	while (!ISLISTEMPTY(l))
	{
		struct IconData *idata = isiconview ? (struct IconData *)muiUserData(tn->obj) : NULL;

		if (threads_check_abort())
		{
			abort = TRUE;
			rc = ABORTED;
		}

		/* in iconview, don't scan detached icons */

		if (abort == FALSE && (isiconview == FALSE || _parent(tn->obj) != NULL))
		{
			LONG thumb_generated = FALSE;
			LONG deficon_generated = FALSE;
			APTR mimetype = NULL;
			ULONG seconds;

			struct Rect32 display_area;

			/*
			 * Check if view scrolled. If it did we will recalculate
			 * first node to be scanned. Some better strategy?
			 */

			if (n == 0)
			{
				methodstack_push_sync(obj, 2, MM_View_QueryDisplayArea, &display_area);
				new_toppos = display_area.MinY;

				if (new_toppos != toppos)
				{
					struct typescannernode *ttn;

					toppos = new_toppos;
					tn = NULL;

					ITERATELIST(ttn, l)
					{
						ULONG isvisible = methodstack_push_sync(obj, 2, MM_View_CheckFocus, ttn->obj);

						if (tn == NULL && isvisible)
						{
							tn = ttn;
							break;
						}
					}

					/*
					 * There is a case when all visible objects are already processed.
					 */

					if (tn == NULL)
					{
						tn = FIRSTNODE(l);
					}
				}
			}

			n++;
			if (n == 10)
				n = 0;

			/* update after refocusing */

			idata = isiconview ? (struct IconData *)muiUserData(tn->obj) : NULL;

			/* if we are ordered to generate thumbs, then we do it before other stuff */
			/* not anymore, we get mimetype first, as mimetype can be needed to create thumb (fab) */
		
			methodstack_push_sync(tn->obj, 3, OM_GET, MA_Icon_FileDate, &seconds);
			mimetype = gettype(tn->path, seconds, NULL);

			if (mimetype != NULL)
			{
				methodstack_push_sync(tn->obj, 3,
					MUIM_Set,
					MA_Icon_MimeType, mimetype
				);
			}	 

			if (gen_thumbs)
				thumb_generated = tr_thumb_createicon(obj, tn->obj, tn->path, TCF_CREATE);

			/* this part is iconclass-dependant */
			if (!thumb_generated)
			{
				if (mimetype != NULL)
				{
					if (gen_deficons)
					{
						ULONG def;
						ULONG type;

						methodstack_push(tn->obj, 3, OM_GET, MA_Icon_IsDefault, &def);
						methodstack_push_sync(tn->obj, 3, OM_GET, MA_Icon_Type, &type);

						if ( def )
						{
							deficon_generated = deficonpool_apply_default_icon(tn->obj, type, tn->path, mimetype);

							if (deficon_generated)
								methodstack_push_sync(tn->obj, 2, MM_Icon_Generate, TRUE);
						}
					}
				}
			}

			/* refresh */

			if (mimetype || thumb_generated || deficon_generated)
				methodstack_push_sync(obj, 2, MM_View_RedrawEntry, tn->obj);

		}

		/* unlock icon object. */

		if (idata != NULL)
		{
			ATOMIC_STORE(&idata->locked, FALSE);
		}

		/*
		 * If we reached end of list we need to check if we skipped some nodes.
		 */

		{
			struct typescannernode *ttn;

			if (tn != LASTNODE(l))
			{
				ttn = NEXTNODE(tn);
			}
			else
			{
				ttn = FIRSTNODE(l);
			}

			REMOVE(tn);
			free(tn);
			tn = ttn;
		}
	}

	/* at the end, do relayout. some icons might be in remove queue */
	methodstack_push(obj, 1, MM_Iconview_DoLayout);

	free(l);
	return (rc);
}

ULONG tr_typescanner_scan_entry(APTR obj UNUSED, CONST_STRPTR uri, Object *mimetypeptr)
{
	THREAD;
	// keep 2 set calls here, it's important
	set(mimetypeptr, MA_Mimetype_Type, gettype(uri, 0, NULL));
	set(mimetypeptr, MA_Mimetype_TypeResolved, TRUE);
	return TRUE;
}

static void delete_typescanner_list(struct MinList *l)
{
	struct typescannernode *tn, *ttn;

	ITERATELISTSAFE(tn, ttn, l)
	{
		free(tn);
	}

	free(l);
}

ULONG tr_typescanner_scan_matchfirst(APTR obj UNUSED, struct MinList *l, Object *mimetypeptr)
{
	struct internal_mimetype_node *basemimetype;
	struct typescannernode *tsn;
	TEXT mimetypepattern[ 64 ];
	ULONG strict = TRUE;

	/* first get first entry and examine type */

	THREAD;

	tsn = FIRSTNODE(l);

	basemimetype = NEXTNODE(tsn) ? gettype(tsn->path, 0, NULL) : NULL;

	if ( basemimetype == NULL )
	{
		set(mimetypeptr, MA_Mimetype_TypeResolved, TRUE);
		delete_typescanner_list(l);
		return FALSE;
	}

	/* It can apparently sometimes be called with only one node, handle it anyway */
	if ( tsn == LASTNODE(l) )
	{
		set(mimetypeptr, MA_Mimetype_Type, basemimetype);
		set(mimetypeptr, MA_Mimetype_TypeResolved, TRUE);
		delete_typescanner_list(l);
		return TRUE;
	}

	/*
	 * Next we do 2 comparisions. Check if other entries have exacly same type,
	 * and if they have then return it, or if not, try family and return family type.
	 */

	tsn = NEXTNODE(tsn);

	D(MIMETYPE,bug("Base mimetype:%s\n", basemimetype->mimetype));

	while(tsn != NULL)
	{
		if (strict == TRUE && mimetype_checkpath(basemimetype, tsn->path) == FALSE)
		{
			/* setup pattern */

			STRPTR separator = strchr(basemimetype->mimetype, '/'); /* assume it's present? */

			if (separator == NULL)
			{
				/* XXX: That shouldn't happen, bail out... */

				set(mimetypeptr, MA_Mimetype_TypeResolved, TRUE);
				delete_typescanner_list(l);

				return FALSE;
			}

			stccpy(mimetypepattern, basemimetype->mimetype, separator - basemimetype->mimetype + 1);
			strcat(mimetypepattern, "/*");

			strict = FALSE;

			D(MIMETYPE,bug("Family pattern:<%s>\n", mimetypepattern));
		}

		if (strict == FALSE)
		{
			APTR mimetype = gettype(tsn->path, 0, mimetypepattern);

			/* if not found either, then we couldn't find common type.. */

			if (mimetype == NULL)
			{
				D(MIMETYPE,bug("File <%s> failed a check for <%s>\n", tsn->path, mimetypepattern));
				D(MIMETYPE,bug("File <%s> has type <%s>\n", tsn->path, ((struct internal_mimetype_node*)mimetype_find("file://", tsn->path, MTF_FILEIO))->mimetype));
		
				set(mimetypeptr, MA_Mimetype_TypeResolved, TRUE);
				delete_typescanner_list(l);

				return FALSE;
			}
		}

		{
			struct typescannernode *ntsn;

			if (tsn != LASTNODE(l))
			{
				ntsn = NEXTNODE(tsn);
			}
			else
			{
				ntsn = NULL;
			}

			REMOVE(tsn);
			free(tsn);
			tsn = ntsn;
		}
	}

	if (strict == FALSE)
	{
		D(MIMETYPE,bug("Lookup mimetype for:<%s>\n", mimetypepattern));
		basemimetype = mimetype_find_generic_by_mimetype(mimetypepattern);
	}

	set(mimetypeptr, MA_Mimetype_Type, basemimetype);
	set(mimetypeptr, MA_Mimetype_TypeResolved, TRUE);
	delete_typescanner_list(l);

	return TRUE;
}
