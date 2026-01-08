/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2007 Ambient Open Source Team
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
 * $Id: deficonpool.c,v 1.20 2022/01/08 01:29:26 piru Exp $
 */

#include "ambient.h"

/* public */
#include <proto/dos.h> /* XXX: have a file_exits() or so */
#include <exec/semaphores.h>

/* private */
#include "deficon_getpath.h"
#include "deficonpool.h"
#include "mui_func.h"
#include "methodstack.h"
#include "iconio.h"
#include "hash.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "deficon.h"
#include "def_disk_logo.h"
#include "def_tool_logo.h"
#include "def_drawer_logo.h"
#include "def_mymorphos_logo.h"
#include "def_view_logo.h"
#include "name.h"
#include "file_func.h"
#include "cache.h"
#include "mimetype.h"
#include "dosnotify.h"
#include "typescanner.h"
#include "time_func.h"
#include "icondata.h"


#define USE_ICONMIME_HACK 1

#if USE_ICONMIME_HACK
static struct MinList deficon_mime_lists;
#endif
static APTR deficon_drawer = NULL;
static APTR deficon_tool = NULL;
static APTR deficon_mymorphos = NULL;
static APTR deficon_view = NULL;
static APTR deficon_bookmarks = NULL;

static struct SignalSemaphore deficonpoolsem;
static APTR notifyctx;

struct deficon_device_node {
	struct MinNode n;
	ULONG hash;
	STRPTR name;
	APTR obj;
};

#if USE_ICONMIME_HACK
struct deficon_mime_node {
	struct MinNode n;
	ULONG hash;
	STRPTR name;
	APTR obj;
};

struct deficon_mime_list_node {
	struct MinNode n;
	ULONG viewid;
	struct MinList list;
};

static void deficonpool_delete_mimeext_name(STRPTR name);


/*
 * Allocate new list of deficons.
 */

static struct deficon_mime_list_node *deficonpool_new_mime_list(ULONG viewid)
{
	struct deficon_mime_list_node *dmln = malloc(sizeof(*dmln));

	if (dmln != NULL)
	{
		dmln->viewid = viewid;
		NEWLIST(&dmln->list);
		ADDTAIL(&deficon_mime_lists, dmln);
	}

	return dmln;
}

/*
 * Get list with deficons for specified view.
 */

static struct MinList *deficonpool_get_mime_list(ULONG viewid)
{
	struct deficon_mime_list_node *dmln;

	ITERATELIST(dmln, &deficon_mime_lists)
	{
		if (dmln->viewid == viewid)
			return &dmln->list;
	}

	D(DEFICONPOOL,bug("Allocating deficon pool for view %d\n", viewid));

	dmln = deficonpool_new_mime_list(viewid);

	if (dmln != NULL)
		return &dmln->list;
	else
		return NULL;
}

/*
 * Delete all deficons associated with specified view.
 */

void deficonpool_delete_mime_list(ULONG viewid)
{
	struct deficon_mime_list_node *dmln;

	ITERATELIST(dmln, &deficon_mime_lists)
	{
		if (dmln->viewid == viewid)
		{
			deficonpool_flush_mime_list(viewid);
			REMOVE(dmln);
			free(dmln);
			return;
		}
	}
}

/*
 * Flushes deficons pool associated with a view (called fex when switching from icon to list).
 */

void deficonpool_flush_mime_list(ULONG viewid)
{
	struct MinList *deficon_mime_list = deficonpool_get_mime_list(viewid);

	if (deficon_mime_list != NULL)
	{
		struct deficon_mime_node *mmn, *nextmmn;
		LONG unflushed = 0;

		ITERATELISTSAFE(mmn, nextmmn, deficon_mime_list)
		{
			if ( getv( mmn->obj, MA_Icon_ReferenceCount ) == 0 )
			{
				MUI_DisposeObject(mmn->obj);
				deficonpool_delete_mimeext_name(mmn->name);
				free(mmn);
			}
			else
			{
				unflushed++;
			}
		}

		NEWLIST(deficon_mime_list);

		D(DEFICONPOOL,bug("[View:%d]:Unflushed icons:%d\n", viewid, unflushed));
	}
}


#if 0
static STRPTR deficonpool_build_mimeext_name(CONST_STRPTR path)
{
	TEXT iconmime[PATH_SIZE];
	STRPTR name = NULL;
	STRPTR p, q;

	ASSERT(path);

	D(DEFICONPOOL,bug("path <%s>\n", path));

	if ( (q = strrchr(path, '.')) )
	{
#warning "FIXME: What if path begins with a '.' and it's the only dot ?"
		p = q - 1;
						
		while (p != path && *p != '.')
			p--;

		if (p != path && *p == '.')
		{
			BPTR l;
			LONG len = q - ++p ;
			UBYTE tmp[4 + len + 5 + 1]; /* "def_" ".info" */

			memcpy(tmp, "def_", 4);
			memcpy(tmp + 4, p, len);
			memcpy(tmp + 4 + len, ".info", 6);

			if (deficon_getpath(iconmime, PATH_SIZE, tmp))
			{
				if ( (l = Lock(iconmime, ACCESS_READ)) )
				{
					UnLock(l);
					name = name_build(iconmime);
				}
			}
		}
	}
	return (name);
}
#endif

static struct deficon_mime_node *find_cached_toolicon(ULONG viewid, STRPTR defname)
{
	ULONG hash = hash_nocase(defname);
	struct deficon_mime_node *mmn;
	struct MinList *deficon_mime_list = NULL;

	if (viewid != MV_ViewID_Unknown)
		deficon_mime_list = deficonpool_get_mime_list(viewid);

	if (deficon_mime_list == NULL)
		return NULL;

	ITERATELIST(mmn, deficon_mime_list)
	{
		if (mmn->hash == hash)
		{
			if (!stricmp(mmn->name, defname))
			{
				return mmn;
			}
		}
	}

	/*
	kprintf("no icon cached for <%s>\n", defname);
	ITERATELIST(mmn, deficon_mime_list)
	{
		kprintf("  :%s\n", mmn->name);
	}
	*/
	return NULL;
}

STRPTR new_deficonpool_build_name(ULONG viewid, STRPTR path_info, APTR mimetype, ULONG *refine, STRPTR buff, ULONG buff_size)
{
	STRPTR p = NULL;

	ASSERT(path_info);

	/*
	 * build icon name based on it's type. this is 2-pass process. First one
	 * returns proper icon name only if type is cached to not slow down dir
	 * reading. If type is not found, then icon is marked as 'dirty' and
	 * fast-guess is used to provide temporary icon image for it (old way).
	 */


	if (mimetype == NULL)
	{
		APTR truncation = name_truncateinfo(path_info);

		BPTR l = Lock(path_info, ACCESS_READ);

		if (l != (BPTR)NULL)
		{
			D_S(struct FileInfoBlock, fib);

			if (Examine( l, fib ))
			{
				ULONG hash = hash_nocase_seconds(path_info, datestamp_to_seconds(&fib->fib_Date));

				cache_get(hash, CACHETAG_MIMETYPE, &mimetype);
			}
		
			UnLock(l);
		}

		name_restoreinfo(path_info, truncation);
	}

	if (mimetype != NULL)
	{
		/* build icon path. we need family and member */

		STRPTR mime = (( struct internal_mimetype_node* )mimetype)->mimetype;

		if (mime != NULL)
		{
			STRPTR separator = strchr(mime, '/'); /* assume it's present? */
			TEXT family[ 32 ];
			STRPTR type;
			TEXT deficon_path[ PATH_SIZE ];
			TEXT deficon_name[ PATH_SIZE ];

			/* that is final image */

			*refine = FALSE;

			/* name */

			stccpy(family, mime, separator - mime + 1);
			type = separator + 1;

			if (type[ 0 ] == '*')
			{
				type = "default";
			}

			if (!deficon_getpath(deficon_path, sizeof(deficon_path), family))
			{
				return p;
			}

			/* check if such icon exists (exact match) */

			snprintf(deficon_name, sizeof( deficon_name ), "%s/%s.info", deficon_path, type);
			D(DEFICONPOOL,bug("[View:%d]:Trying:<%s>\n", viewid, deficon_name));

			if (find_cached_toolicon(viewid, deficon_name) || exists( deficon_name))
			{
				/* we got a match which we can return */

				if (buff != NULL)
				{
					stccpy(buff, deficon_name, buff_size);
					p = buff;
				}
				else
					p = name_build(deficon_name);
			}
			else
			{
				/* try with family/default.info */

				snprintf(deficon_name, sizeof(deficon_name), "%s/default.info", deficon_path);
				D(DEFICONPOOL,bug("[View:%d]:Trying:<%s>\n", viewid, deficon_name));

				if (find_cached_toolicon(viewid, deficon_name) || exists(deficon_name))
				{
					/* we got a match which we can return */

					if (buff != NULL)
					{
						stccpy(buff, deficon_name, buff_size);
						p = buff;
					}
					else
						p = name_build(deficon_name);
				}
			}
		}
	}
	else
	{
		//DB(("Not found\n" ));
	}

	return p;//deficonpool_build_mimeext_name( path );
}


static void deficonpool_delete_mimeext_name(STRPTR name)
{
	ASSERT(name);

	name_delete(name);
}

#endif

STRPTR deficonpool_get_icon_name(ULONG viewid, STRPTR path, ULONG *isdefault)
{
	STRPTR p;

	/* does icon exist ? */
	p = name_build_info(path);

	if(p)
	{
		if (exists(p))
		{
			return(p);
		}
		name_delete(p);
	}
	else
	{
		return NULL;
	}

	*isdefault = TRUE;

	/* is it a device name ? */
	if( isdevicename(path) )
	{
		if ( (p = deficon_build_devicename(path)) )
		{
			return(p);
		}
		else
		{
			return NULL;
		}
	}

	/* is it a directory ? */
	if(isdir(path))
	{
		TEXT name[PATH_SIZE];

		if(deficon_getpath(name, PATH_SIZE, "def_drawer.info") &&
		   (p = (STRPTR)malloc(strlen(name) + 1)))
		{
			strcpy(p, name);
			return(p);
		}
		return NULL;
	}

#if USE_ICONMIME_HACK
	/* use iconmime hack to get icon name */

	{
		STRPTR q;
		ULONG refine;
		
		q = new_deficonpool_build_name(viewid, path, NULL, &refine, NULL, 0);

		if( q )
		{
			return(q);
		}
	}
#endif

	/* fallback to default.info then */
	if(exists(path))
	{
		TEXT name[PATH_SIZE];

		if(deficon_getpath(name, PATH_SIZE, "default.info") &&
		   (p = (STRPTR)malloc(strlen(name) + 1)))
		{
			strcpy(p, name);
			return(p);
		}
	}

	return NULL;
}

static APTR make_default_icon(ULONG viewid, ULONG type, STRPTR path, APTR mimetype)
{
	#if USE_ICONMIME_HACK
	struct deficon_mime_node *mmn = NULL;
	#endif

	APTR o;
	TEXT name[PATH_SIZE];
	LONG filetype = MV_Icon_FileType_None;
	ULONG refine = TRUE;

	name[0] = '\0';

	D(DEFICONPOOL,bug("[View:%d]: Make default icon for: type:%d, path:%s\n", viewid, type, path));

	switch (type)
	{
		case MV_Icon_Type_MyComputer:
			//filetype = MV_Icon_FileType_Device;
			/* XXX: check for overflow */
			deficon_getpath(name, PATH_SIZE, "def_mymorphos.info");
			break;

		case MV_Icon_Type_Disk:
		case MV_Icon_Type_Device:
			{
				ASSERT(path);

				filetype = MV_Icon_FileType_Device;

				{
					STRPTR dev;

					if (!(dev = deficon_build_devicename(path)))
					{
						/* XXX: check for overflow */
						deficon_getpath(name, PATH_SIZE, "def_device.info");
					}
					else
					{
						strcpy(name, dev);
						deficon_delete_devicename(dev);	/* Is free() really */
					}

					if ((mmn = malloc(sizeof(*mmn))))
					{
						if ((mmn->name = name_build(name)))
						{
							mmn->hash = hash_nocase(mmn->name);
							break;
						}
						free(mmn);
						mmn = NULL;
					}
				}

				break;
			}
			break;

		case MV_Icon_Type_Drawer:
			filetype = MV_Icon_FileType_Directory;
			/* XXX: check for overflow */
			deficon_getpath(name, PATH_SIZE, "def_drawer.info");

			if ((mmn = malloc(sizeof(*mmn))))
			{
				if ((mmn->name = name_build(name)))
				{
					mmn->hash = hash_nocase(mmn->name);
					break;
				}
				free(mmn);
				mmn = NULL;
			}
			break;

		case MV_Icon_Type_View:
			/* XXX: check for overflow */
			deficon_getpath(name, PATH_SIZE, "def_view.info");
			break;

		case MV_Icon_Type_Bookmarks:
			/* XXX: check for overflow */
			deficon_getpath(name, PATH_SIZE, "def_bookmarks.info");
			break;

		case MV_Icon_Type_Tool:
			filetype = MV_Icon_FileType_File;
			#if USE_ICONMIME_HACK
			{
				ASSERT(path);

				if ( (mmn = malloc(sizeof(*mmn))) )
				{
					if ( (mmn->name = new_deficonpool_build_name(viewid, path, mimetype, &refine, NULL, 0)) )
					{
						strcpy(name, mmn->name);
						mmn->hash = hash_nocase(mmn->name);
						break;
					}
					else
					{
						/* XXX: check for overflow */
						deficon_getpath(name, PATH_SIZE, "def_tool.info");
						if ((mmn->name = name_build(name)))
						{
							mmn->hash = hash_nocase(mmn->name);
							break;
						}
					}
					free(mmn);
				}
				return (NULL);
			}
			#else
			/* XXX: check for overflow */
			deficon_getpath(name, PATH_SIZE, "def_tool.info");
			#endif
			break;
	
		#ifdef DEBUG
		default:
			PDB(("aargh\n"));
			break;
		#endif
	}

	D(DEFICONPOOL,bug("[View:%d]: Create icon...:%s\n", viewid, name));

	if (name[0] != '\0' && (o = (APTR)methodstack_push_sync(app, 3, MM_Application_CreateIcon, TRUE, viewid)))
	{
		D(DEFICONPOOL,bug("[View:%d]:Trying to load icon: %s\n", viewid, name));
		if (icon_read(name, o, ICONTAG_FileType, filetype, TAG_DONE))
		{
			#if USE_ICONMIME_HACK
			if (mmn)
			{
				struct MinList *deficon_mime_list = deficonpool_get_mime_list(viewid);
				D(DEFICONPOOL,bug("[View:%d]: Adding icon to cache:%s\n", viewid, name));
				ASSERT(mmn->name);
				mmn->obj = o;
				ADDTAIL(deficon_mime_list, mmn);
			}
			#endif
			return (o);
		}
		else
		{
			/* internal fallback */
			ULONG *srcarray = NULL;
			APTR tbm;
			ULONG xs = 0;
			ULONG ys = 0;

			D(DEFICONPOOL,bug("[View:%d]:internal fallback for type %ld\n", viewid, type));

			switch (type)
			{
				case MV_Icon_Type_MyComputer:
					srcarray = def_mymorphos;
					xs = DEF_MYMORPHOS_WIDTH;
					ys = DEF_MYMORPHOS_HEIGHT;
					#if !defined(DEPEND) && (DEF_MYMORPHOS_DEPTH < 32)
					#error "def_mymorphos logo depth < 32 bits, this code doesn't handle that"
					#endif
					break;

				case MV_Icon_Type_Disk:
				case MV_Icon_Type_Device:
					srcarray = def_disk;
					xs = DEF_DISK_WIDTH;
					ys = DEF_DISK_HEIGHT;
					#if !defined(DEPEND) && (DEF_DISK_DEPTH < 32)
					#error "def_disk logo depth < 32 bits, this code doesn't handle that"
					#endif
					break;

				case MV_Icon_Type_Drawer:
					srcarray = def_drawer;
					xs = DEF_DRAWER_WIDTH;
					ys = DEF_DRAWER_HEIGHT;
					#if !defined(DEPEND) && (DEF_DRAWER_DEPTH < 32)
					#error "def_drawer logo depth < 32 bits, this code doesn't handle that"
					#endif
					break;

				case MV_Icon_Type_Tool:
					srcarray = def_tool;
					xs = DEF_TOOL_WIDTH;
					ys = DEF_TOOL_HEIGHT;
					#if !defined(DEPEND) && (DEF_TOOL_DEPTH < 32)
					#error "def_tool logo depth < 32 bits, this code doesn't handle that"
					#endif
					break;

				case MV_Icon_Type_View:
				case MV_Icon_Type_Bookmarks: /* XXX: This is really a bad choice. need bookmarks icon */
					srcarray = def_view;
					xs = DEF_VIEW_WIDTH;
					ys = DEF_VIEW_HEIGHT;
					#if !defined(DEPEND) && (DEF_TOOL_DEPTH < 32)
					#error "def_tool logo depth < 32 bits, this code doesn't handle that"
					#endif
					break;

				#ifdef DEBUG
				default:
					PDB(("urgl\n"));
					break;
				#endif
			}

			if ( (tbm = gfx_bitmap_create(xs, ys, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE)) )
			{
				struct MinList *deficon_mime_list = deficonpool_get_mime_list(viewid);

				gfx_blit(srcarray, tbm,
					BLITTAG_SrcType, BLITVAL_SrcType_Array,
					BLITTAG_SrcFormat, BLITVAL_SrcFormat_ARGB,
				TAG_DONE);

				methodstack_push(o, 4, MM_Icon_AddBitMap, tbm, MV_Icon_BitMap_PNGicon, MV_Icon_BitMap_Normal);
				methodstack_push(o, 3, MUIM_Set, MA_Icon_ImageType, MV_Icon_ImageType_PNGicon);
				methodstack_push(o, 3, MUIM_Set, MA_Icon_Refine, refine);
				methodstack_push(o, 1, MM_Icon_End); /* XXX: should be enough I think.. */

				if (mmn != NULL)
				{
					/* add generated icon to cache */

					D(DEFICONPOOL,bug("[View:%d]: Adding builtin icon to cache:%s\n", viewid, name));
					ASSERT(mmn->name);
					mmn->obj = o;
					ADDTAIL(deficon_mime_list, mmn);
				}
				return (o);
			}

			methodstack_push(o, 1, OM_RELEASE);
		}
	}

	#if USE_ICONMIME_HACK
	if (mmn)
	{
		deficonpool_delete_mimeext_name(mmn->name);
		free(mmn);
	}
	#endif
	return (NULL);
}

#if 0 /* was defined, but not used, so disabled (geit) */
static void add_bitmap(APTR srcobj, APTR dstobj, ULONG type, ULONG state)
{
	APTR bm;

	if ( (bm = (APTR)methodstack_push_sync(srcobj, 3,
		MM_Icon_CreateBitMap,
		type, state
	)) )
	{
		methodstack_push(dstobj, 4,
			MM_Icon_AddBitMap,
			bm, type, state
		);
	}
}
#endif

ULONG deficonpool_apply_default_icon(APTR obj, ULONG type, STRPTR path, APTR mimetype)
{
	APTR *o;
	APTR dummy = NULL;
	ULONG refine = TRUE;
	struct IconData *idata = (struct IconData *)muiUserData(obj); /* shortcut */
	ULONG viewid = idata->viewid;

	THREAD;

	o = &dummy;

	if (!type)
	{
		type = MV_Icon_Type_Tool;
	}

	D(DEFICONPOOL,bug("[View:%d]:applying default icon for type %ld\n", viewid, type));

	if ( mimetype == (APTR)DEFICON_MIMETYPE_RECOGNIZE )
	{
		D(DEFICONPOOL,bug("[View:%d]:Checking mimetype\n", viewid));

		/* XXX: Execute the thread maybe? */
		Object *mtObject = NewObject(getmimetypeclass(), NULL, TAG_DONE);

		if ( tr_typescanner_scan_entry(obj, path, mtObject) == FALSE )
		{
			D(DEFICONPOOL,bug("[View:%d]:Failed\n", viewid));
		}

		mimetype = xget(mtObject, MA_Mimetype_Type);
		DoMethod(mtObject, OM_RELEASE);
	}

	switch (type)
	{
		case MV_Icon_Type_MyComputer:
			o = &deficon_mymorphos;
			break;

		case MV_Icon_Type_View:
			o = &deficon_view;
			break;

		case MV_Icon_Type_Bookmarks:
			o = &deficon_bookmarks;
			break;

		case MV_Icon_Type_Disk:
		case MV_Icon_Type_Device:
			{
				#if USE_ICONMIME_HACK
				STRPTR defname;

				ASSERT(path);

				if ( (defname = deficon_build_devicename(path)) )
				{
					struct deficon_mime_node *mmn;
					
					D(DEFICONPOOL,bug("[View:%d]: Lookup defice deficon:<%s>\n", viewid, defname));

					mmn	= find_cached_toolicon(viewid, defname);
					if (mmn != NULL)
					{
						D(DEFICONPOOL,bug("[View:%d]:found cached object for <%s>\n", viewid, mmn->name));
						o = &mmn->obj;
					}
					deficon_delete_devicename(defname);
				}
				#endif
			}
			break;

		case MV_Icon_Type_Drawer:
			{
				struct deficon_mime_node *mmn;
				TEXT name[PATH_SIZE];

				deficon_getpath(name, PATH_SIZE, "def_drawer.info");
				mmn	= find_cached_toolicon(viewid, name);

				if (mmn != NULL)
				{
					D(DEFICONPOOL,bug("[View:%d]:found cached object for <%s>\n", viewid, mmn->name));
					o = &mmn->obj;
				}
			}
			//o = &deficon_drawer;
			break;

		case MV_Icon_Type_Tool:
			#if USE_ICONMIME_HACK
			{
				STRPTR defname;
				TEXT name[PATH_SIZE];

				ASSERT(path);

				if ((defname = new_deficonpool_build_name(viewid, path, mimetype, &refine, name, sizeof( name ))))
				{
					struct deficon_mime_node *mmn;

					mmn	= find_cached_toolicon(viewid, defname);

					if (mmn != NULL)
					{
						D(DEFICONPOOL,bug("[View:%d]:found cached object for <%s>\n", viewid, mmn->name));
						o = &mmn->obj;
					}
				}
				else
				{
					struct deficon_mime_node *mmn;
					TEXT name[PATH_SIZE];

					deficon_getpath(name, PATH_SIZE, "def_tool.info");
					mmn	= find_cached_toolicon(viewid, name);

					if (mmn != NULL)
					{
						D(DEFICONPOOL,bug("[View:%d]:found cached object for <%s>\n", viewid, mmn->name));
						o = &mmn->obj;
					}
				}
			}
			#else
			o = &deficon_tool;
			#endif
			break;

		#ifdef DEBUG
		default:
			PDB(("eek, wrong type %ld\n", type));
			return (FALSE);
			break;
		#endif
	}

	/*
	 * check if we got different icon that was already assigned
	 * refine == FALSE means we got final image.
	 */

	if (*o)
	{
		STRPTR oldname;
		STRPTR newname;

		methodstack_push(obj, 3, OM_GET, MA_Icon_IconPath, &oldname);
		methodstack_push_sync(*o, 3, OM_GET, MA_Icon_IconPath, &newname); /* yep, that's correct one */

		if (oldname && newname && !stricmp(oldname, newname))
		{
			return FALSE;
		}
		else if (newname != NULL)
		{
			methodstack_push_sync(obj, 3, MUIM_Set, MA_Icon_IconPath, newname);
		}
	}

	/* */


	if (!*o)
	{
		D(DEFICONPOOL,bug("[View:%d]:no existing default, making one..\n", viewid));
		*o = make_default_icon(viewid, type, path, mimetype);
	}

	if (*o)
	{
		if(mimetype)
		{
			methodstack_push(obj, 3, MUIM_Set, MA_Icon_MimeType, mimetype);
		}
		methodstack_push_sync( obj, 3, MUIM_Set, MA_Icon_Reference, *o );
		//methodstack_push(obj, 3, MUIM_Set, MA_Icon_Refine, refine );
		methodstack_push_sync(obj, 3, MUIM_Set, MA_Icon_Refine, refine );
		/* if we not use "_sync", then we get these 
		Notify_SET                    : MAJOR BUG: OM_SET on dead obj obj=0x1f1a9298 INVALID task=24cb8bd8
		Notify_SET                    :    Attribute: feca0053 = 00000000
		*/
		methodstack_push_sync(obj, 1, MM_Icon_SetIcon);
		return (TRUE);
	}
	return (FALSE);
}


ULONG deficonpool_init(void)
{
	#if USE_ICONMIME_HACK
	NEWLIST(&deficon_mime_lists);
	#endif

	InitSemaphore(&deficonpoolsem);

	/* dosnotify for deficonpath */

	notifyctx = dosnotify_start( DEFICONPATH_FILE, 0 );

	return (TRUE);
}


void deficonpool_cleanup(void)
{
	dosnotify_stop(notifyctx);

	#if USE_ICONMIME_HACK

	deficonpool_flush();

	#endif

	if (deficon_drawer != NULL)
	{
		MUI_DisposeObject(deficon_drawer);
		deficon_drawer = NULL;
	}

	if (deficon_tool != NULL)
	{
		MUI_DisposeObject(deficon_tool);
		deficon_tool = NULL;
	}

	if (deficon_mymorphos != NULL)
	{
		MUI_DisposeObject(deficon_mymorphos);
		deficon_mymorphos = NULL;
	}

	if (deficon_view != NULL)
	{
		MUI_DisposeObject(deficon_view);
		deficon_view = NULL;
	}

	if (deficon_bookmarks != NULL)
	{
		MUI_DisposeObject(deficon_bookmarks);
		deficon_bookmarks = NULL;
	}
}

void deficonpool_flush(void)
{
	struct deficon_mime_list_node *dmln, *nextdmln;

	D(DEFICONPOOL,bug("Flushing unused deficons..\n"));

	ObtainSemaphore(&deficonpoolsem);

	/*
	 * Iterate all lists and check for unused icons.
	 */

	ITERATELISTSAFE(dmln, nextdmln, &deficon_mime_lists)
	{
		deficonpool_flush_mime_list(dmln->viewid);

		if (ISLISTEMPTY(&dmln->list))
		{
			REMOVE(dmln);
			free(dmln);
		}
	}

	ReleaseSemaphore(&deficonpoolsem);
}
