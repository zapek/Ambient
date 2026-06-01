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
 * $Id: scandir.c,v 1.20 2026/04/25 23:05:00 jacadcaps Exp $
 */

#include "ambient.h"

/* public */
#include <dos/exall.h>
#include <dos/dosextens.h>
#include <workbench/workbench.h>
#include <proto/dos.h>

/* private */
#include "scandir.h"
#include "mui_func.h"
#include "time_func.h"
#include "methodstack.h"
#include "iconio.h"
#include "exdir.h"
#include "threads.h"
#include "deficonpool.h"
#include "name.h"
#include "file_func.h"
#include "examine64.h"
#include "prefs.h"

/************************************************************************/

enum {
	ANIF_ALL,
	ANIF_NOINFO,
	ANIF_THUMBS,
};

/************************************************************************/

static ULONG addiconfunc( APTR obj, CONST_STRPTR path, struct ExAllData *ead, APTR userdata UNUSED )
{
	APTR o;
	ULONG viewid;

	if( ead->ed_Type > 0 && ead->ed_Type != ST_LINKFILE )
	{
		return( TRUE ); /* skip directories */
	}

	D(SCANDIR, bug("creating object..\n"));

	methodstack_push_sync( obj, 3, OM_GET, MA_Viewgroup_ID, &viewid );

	if( ( o = (APTR) methodstack_push_sync( app, 3, MM_Application_CreateIcon, FALSE, viewid ) ) )
	{
		TEXT t[ PATH_SIZE ];
		STRPTR dot;
		struct fileinfo64 fi;

		stccpy( t, path, sizeof( t ) );
		AddPart( t, ead->ed_Name, sizeof( t ) );
		D(SCANDIR, bug("name: <%s>\n", t));

		icon_read( t, o, ICONTAG_FileSize, ead->ed_Size, ICONTAG_Deficon, TRUE, TAG_DONE ); /* XXX */

		dot = strrchr( t, '.' );

		ASSERT(dot);

		*dot = 0;

		if (examine64( t, &fi ) )
		{
			methodstack_push( o, 3, MUIM_Set, MA_Icon_FileSize, &fi.fi_Size );

			methodstack_push( o, 3, MUIM_Set, MA_Icon_FileDate, datestamp_to_seconds( &fi.fi_Date ) );
		}

		/* resource we are pointing to */

		{
			methodstack_push_sync( o, 3, MUIM_Set, MA_Icon_Path, t );
		}

		methodstack_push( obj, 3, MM_Iconview_AddIcon, o, FALSE ); /* XXX: could that be true? hm.. */

		return( TRUE );
	}
	return( FALSE );
}

/************************************************************************/

#if USE_THUMBS
static ULONG addthumb( APTR obj, STRPTR CONST_path UNUSED )
{
	#if 0
	if( thumb_validate( path ) )
	{
		#if 0
		if( tr_thumb_createicon( obj, path, TCF_CACHE ) ) /* that one fails if the icon is not in the cache */
		{
			return( TRUE );
		}
		else
		#endif
		{
			methodstack_push_sync( obj, 3, MUIM_Set, MA_Icon_ThumbIt, TRUE );
		}
	}
	#else
	if( 1 )
	{
		#if 0
		if( tr_thumb_createicon( obj, path, TCF_CACHE ) ) /* that one fails if the icon is not in the cache */
		{
			return( TRUE );
		}
		else
		#endif
		{
			/*
			 * We set it to TRUE for each file. Thumanail generator has to check if it's a picture.
			 */

			methodstack_push( obj, 3, MUIM_Set, MA_Icon_ThumbIt, TRUE );
		}
	}
	#endif
	return( FALSE );
}
#endif

/************************************************************************/

static ULONG addnoiconfunc( APTR obj, CONST_STRPTR path, struct ExAllData *ead, APTR userdata )
{
	BPTR l = 0;
	TEXT t[ PATH_SIZE ];
	STRPTR iconname;

	stccpy( t, path, sizeof( t ) );
	AddPart( t, ead->ed_Name, sizeof( t ) );

	D(SCANDIR,bug("got <%s>..\n", t));

	if( name_isinfo( t ) && ( userdata != (APTR) ANIF_NOINFO ) )
	{
		return( addiconfunc( obj, path, ead, NULL ) );
	}

	if( ( iconname = name_build_info( t ) ) )
	{
		D(SCANDIR,bug("trying to lock <%s>\n", iconname));
		if( ( l = Lock( iconname, ACCESS_READ ) ) )
		{
			BPTR lock = l;

#if 1
			D_S( struct FileInfoBlock, fib );

			/*
			 * The same workaround as below, except:
			 * - Examine() is way way lighter operation than NameFromLock().
			 * - this version isn't confused by upper/lowercase special chars.
			 *   we're after all only interested about the cutting .info
			 *   extension.
			 * - we're not messing up the ST_SOFTLINK if ( isdir( t ) )
			 *   condition in the default icon generation below. - piru
			 */
			if( Examine( l, fib ) )
			{
				if( name_isinfo( fib->fib_FileName ) )
				{
					D(SCANDIR,bug("locked\n"));
				} else {
					D(SCANDIR,bug("locked by the silly filesystem, rejecting\n"));
					l = (BPTR) NULL;
				}
			}
#else
			/*
			 * Workaround for stupid filesystems where
			 * Lock("filenamelongerthanwhatthefssupports", ACCESS_READ); works.
			 */
			if( NameFromLock( l, t, sizeof(t) ) )
			{
				if( !stricmp( iconname, t ) )
				{
					D(SCANDIR,bug("locked\n"));
				} else {
					D(SCANDIR,bug("locked by the silly filesystem, rejecting\n"));
					l = NULL;
				}
			}
#endif
			else
			{
				D(SCANDIR,bug("locked\n"));
			}
			UnLock( lock );
		}
	}

	if( !l )
	{
		APTR o;
		ULONG viewid;

		D(SCANDIR,bug("no icon, applying a default\n"));

		methodstack_push_sync( obj, 3, OM_GET, MA_Viewgroup_ID, &viewid );

		if( ( o = (APTR) methodstack_push_sync( app, 3, MM_Application_CreateIcon, FALSE, viewid ) ) )
		{
			ULONG rc = FALSE;
			ULONG is_link = FALSE;
			LONG type = ead->ed_Type;

			if( type == ST_SOFTLINK )
			{
				is_link = TRUE;

				if ( isdir( t ) )
					type = ST_USERDIR;
				else
					type = ST_FILE;
			}

			switch( type )
			{
				case ST_LINKDIR:
					is_link = TRUE;
				case ST_USERDIR:
					rc = deficonpool_apply_default_icon( o, MV_Icon_Type_Drawer, iconname, NULL );
					methodstack_push( o, 3, MUIM_Set, MA_Icon_Type, WBDRAWER );
					methodstack_push( o, 3, MUIM_Set, MA_Icon_IsLink, is_link );
					break;

				case ST_LINKFILE:
					is_link = TRUE;
				case ST_FILE:
					/*
					 * XXX: find out if it's a picture or so
					 */
					#if USE_THUMBS
					if( userdata != (APTR) ANIF_THUMBS || !( rc = addthumb( o, t ) ) )
					#endif
					{
						/* this one will apply deficon based on type if type is stored in cache only */
						rc = deficonpool_apply_default_icon( o, MV_Icon_Type_Tool, iconname, NULL );
					}

					methodstack_push( o, 3, MUIM_Set, MA_Icon_Type, WBTOOL );
					methodstack_push( o, 3, MUIM_Set, MA_Icon_IsLink, is_link );
					break;

				#ifdef DEBUG
				default:
					PDB(("unknown type? wtf\n"));
					break;
				#endif
			}

			if (rc)
			{
				UQUAD size;
				struct TagItem tags[ 8 ];
				ULONG tind = 0;

				#if USE_LEGACY
				size = (ULONG) ead->ed_Size;
				#else
				size = (UQUAD) ead->ed_Size64;
				#endif

			
				tags[ tind   ].ti_Tag  = MA_Icon_FileSize;
				tags[ tind++ ].ti_Data = (ULONG) ( &size );

				tags[ tind   ].ti_Tag  = MA_Icon_FileDate;
				tags[ tind++ ].ti_Data = ead->ed_Ticks / TICKS_PER_SECOND + ead->ed_Mins * 60 + ead->ed_Days * 60 * 24 * 60;

				tags[ tind   ].ti_Tag  = MA_Icon_IsDefault;
				tags[ tind++ ].ti_Data = TRUE;

				tags[ tind   ].ti_Tag  = MA_Icon_FileType;
				tags[ tind++ ].ti_Data = type; /* XXX: ead->ed_Type but modified to file/dir when ST_SOFTLINK */

				tags[ tind   ].ti_Tag  = MA_Icon_PathInfo;
				tags[ tind++ ].ti_Data = (ULONG) iconname;

				tags[ tind   ].ti_Tag  = MA_Icon_Path;
				tags[ tind++ ].ti_Data = (ULONG) t;

				tags[ tind   ].ti_Tag  = 0;
				tags[ tind++ ].ti_Data = 0;

				methodstack_push_sync( o, 2, OM_SET, tags );

				methodstack_push( o, 1, MM_Icon_End );

				methodstack_push_sync( obj, 3, MM_Iconview_AddIcon, o, FALSE );
			}
			else
			{
				methodstack_push( o, 1, OM_RELEASE );
			}
		}
	}

	if( iconname )
	{
		name_delete( iconname ); /* XXX: beware of not skipping that by returning above.. remove that comment once I'm sure :) */
	}
	return( TRUE ); /* XXX: wrong.. */
}

/************************************************************************/

static ULONG addffilefunc( APTR obj, CONST_STRPTR path, struct ExAllData *ead, APTR userdata UNUSED )
{
	ULONG rc;
	#if 0
	ULONG size = ead->ed_Size;
	#if !USE_LEGACY
	UQUAD size64 = ead->ed_Size64;
	#endif
	#endif
	STRPTR target = NULL;
	struct DevProc* devproc;

	if (gprefs->hide_dot_filenames && ead->ed_Name[0] == '.')
		return TRUE;
	
	if( ead->ed_Type == ST_SOFTLINK || ead->ed_Type == ST_LINKFILE || ead->ed_Type == ST_LINKDIR )
	{
		TEXT t[ PATH_SIZE ];
		TEXT linktarget[ PATH_SIZE ];
		BPTR l;

		stccpy( t, path, sizeof(t) );
		AddPart( t, ead->ed_Name, sizeof(t) );

		D(SCANDIR, bug("link at: [%s], name: [%s]...\n", path, ead->ed_Name));

		if( ( ead->ed_Type == ST_SOFTLINK ) && isdir( t ) )
		{
			ead->ed_Size = -1; /* XXX: Yes, bit sucky but easiest without touching tons of code */
			#if !USE_LEGACY
			ead->ed_Size64 = NO_FILESIZE;
			#endif
		}

		devproc = GetDeviceProc( path, NULL );
		if( devproc )
		{
			if( ( l = Lock( path, ACCESS_READ ) ) )
			{

				if( ead->ed_Type == ST_SOFTLINK )
				{
					D(SCANDIR, bug("Resolving softlink.\n"));
					/* resolve softlinks, fall back to ReadLink(), if NewReadLink() fails */
					if( NewReadLink( devproc->dvp_Port, l, ead->ed_Name, linktarget, sizeof(linktarget) ) > 0 )
					{
						target = linktarget;
					} else {
						D(SCANDIR, bug("using ReadLink() fallback while resolving softlink [%s].\n", t));
						if ( ReadLink( devproc->dvp_Port, l, ead->ed_Name, linktarget, sizeof(linktarget) ) > 0 ) {
							target = linktarget;
						}
					}
				} else {
					D(SCANDIR, bug("Resolving hardlink.\n"));
					/* resolve hardlinks */
					if ( NewReadLink( devproc->dvp_Port, l, ead->ed_Name, linktarget, sizeof(linktarget) ) )
					{
						target = linktarget;
					}
				}
				UnLock( l );

				/* if NewReadLink()/ReadLink() magic didn't help, try to resolve links the old-fashioned way */
				if( ( !target ) && ( l = Lock( t, ACCESS_READ ) ) )
				{
					D(SCANDIR, bug("using NameFromLock() fallback while resolving link [%s].\n", t));
					if( NameFromLock( l, linktarget, sizeof(linktarget) ) )
					{
						target = linktarget;
					}
					UnLock( l );
				}

			} else {
				D(SCANDIR, bug("failed to lock path of link [%s].\n", t));
			}
			FreeDeviceProc( devproc );
		} else {
			D(SCANDIR, bug("failed to access deviceproc of link [%s].\n", t));
		}

		if( !target )
		{
			D(SCANDIR, bug("link [%s] failed to resolve.\n", t));
		} else {
			D(SCANDIR, bug("link [%s] resolved to [%s].\n", t, target));
		}
	}

	rc = methodstack_push_sync( obj, 5, MM_Listview_AddFile, ead, FALSE, MV_Listview_AddFile_Normal, target );

	#if 0
	ead->ed_Size = size;
	#if !USE_LEGACY
	ead->ed_Size64 = size64;
	#endif
	#endif

	return( rc );
}

/************************************************************************/

static ULONG addffilenoiconfunc( APTR obj, CONST_STRPTR path, struct ExAllData *ead, APTR userdata )
{
	BPTR l = 0;
	TEXT t[ PATH_SIZE ];
	ULONG add = TRUE;

	stccpy( t, path, sizeof(t) );
	AddPart( t, ead->ed_Name, sizeof(t) );

	D(SCANDIR,bug("got <%s>..\n", t));

	if( name_isinfo( t ) )
	{
		APTR truncation = name_truncateinfo( t );

		if( ( l = Lock( t, ACCESS_READ ) ) )
		{
			add = FALSE;
			UnLock( l );
		} else {
			add = TRUE;
		}

		name_restoreinfo( t, truncation );
	} else {
		add = TRUE;
	}

	if( add )
	{
		return( addffilefunc( obj, path, ead, userdata ) );
	}

	return( TRUE );
}

/************************************************************************/

static ULONG addffileiconfunc( APTR obj, CONST_STRPTR path, struct ExAllData *ead, APTR userdata )
{
	BPTR l = 0;
	TEXT t[ PATH_SIZE ];
	ULONG add = FALSE;

	stccpy( t, path, sizeof(t) );
	AddPart( t, ead->ed_Name, sizeof(t) );

	D(SCANDIR,bug("got <%s>..\n", t));

	if( !name_isinfo( t ) )
	{
		STRPTR file = name_build_info( t );

		if( ( l = Lock( file, ACCESS_READ ) ) )
		{
			add = TRUE;

			UnLock( l );
		} else {
			add = FALSE;
		}
	} else {
		APTR truncation = name_truncateinfo( t );

		if( ( l = Lock( t, ACCESS_READ ) ) )
		{
			add = FALSE;
			UnLock(l);
		} else {
			add = TRUE;
		}

		name_restoreinfo( t, truncation );
	}


	if( add )
	{
		return( addffilefunc( obj, path, ead, userdata ) );
	}

	return( TRUE );
}

/************************************************************************/

static void notifyview( APTR obj, struct dirinfo *di )
{
	methodstack_push(      obj, 3, MUIM_Set, MA_View_TotalFiles, di->files );
	methodstack_push(      obj, 3, MUIM_Set, MA_View_TotalDirs, di->dirs );
	methodstack_push(      obj, 3, MUIM_Set, MA_View_Links, di->links );
	methodstack_push(      obj, 3, MUIM_Set, MA_View_IconFiles, di->icons );
	methodstack_push(      obj, 3, MUIM_Set, MA_View_SelectedDiskUsage, &( di->selecteddiskusage ) );
	methodstack_push_sync( obj, 3, MUIM_Set, MA_View_DiskUsage, &( di->diskusage ) );
}

#if USE_LEGACY
#define FILESCANTYPE ED_OWNER
#else
#define FILESCANTYPE ED_SIZE64
#endif

/************************************************************************/

ULONG tr_scandir( APTR obj, CONST_STRPTR path, ULONG mode )
{
	ULONG rc = rc; /* shut up gcc */
	ULONG isdevice;
	struct dirinfo di;

	THREAD;
	ASSERT(path);
	memset( &di, 0, sizeof( struct dirinfo ) );

	isdevice = isdevicename( path );

	D(SCANDIR,bug("scanning <%s>\n", path));

	/*
	 * Only mask the 'disk.info'
	 * if it's in the device.
	 */
	switch( mode )
	{
		case TV_File_ScanDir_Mode_Icons:
			D(SCANDIR,bug("icons mode\n"));
			rc = exdir( obj, path, isdevice ? "~(disk|%).info" : "~(%).info", ED_DATE, 0, addiconfunc, NULL, &di, NULL );
			notifyview( obj, &di );
			break;

		case TV_File_ScanDir_Mode_ShowAll:
			D(SCANDIR,bug("showall mode\n"));
			rc = exdir( obj, path, isdevice ? "~(disk.info)" : "#?", FILESCANTYPE, 0, addnoiconfunc, NULL, &di, (APTR) ANIF_ALL ); /* show everything */
			notifyview( obj, &di );
			break;

		case TV_File_ScanDir_Mode_NoIcons:
			D(SCANDIR,bug("noicons mode\n"));
			rc = exdir( obj, path, "~(#?.info)", FILESCANTYPE, 0, addnoiconfunc, NULL, &di, (APTR) ANIF_NOINFO ); /* only show icons without corresponding .info */
			notifyview( obj, &di );
			break;

		#if USE_THUMBS
		case TV_File_ScanDir_Mode_Thumbs:
			D(SCANDIR,bug("thumbs mode\n"));
			rc = exdir( obj, path, isdevice ? "~(disk.info)" : "#?", FILESCANTYPE, 0, addnoiconfunc, NULL, &di, (APTR) ANIF_THUMBS ); /* only show icons without corresponding .info */
			notifyview( obj, &di );
			break;
		#endif

		case TV_File_ScanDir_Mode_Files:
			D(SCANDIR,bug("files mode\n"));
			rc = exdir( obj, path, "#?", FILESCANTYPE, 0, addffilefunc, NULL, &di, NULL );
			notifyview( obj, &di );
			break;

		case TV_File_ScanDir_Mode_FilesNoIcons:
			D(SCANDIR,bug("files mode without .info files\n"));
			rc = exdir( obj, path, "#?", FILESCANTYPE, 0, addffilenoiconfunc, NULL, &di, NULL );
			notifyview( obj, &di);
			break;

		case TV_File_ScanDir_Mode_FilesIcons:
			D(SCANDIR,bug("files with icons only\n"));
			rc = exdir( obj, path, isdevice ? "~(disk.info)" : "#?", FILESCANTYPE, 0, addffileiconfunc, NULL, &di, NULL );
			notifyview( obj, &di );
			break;

		#ifdef DEBUG
		default:
			PDB(("wrong scanmode %ld\n", mode));
			break;
		#endif
	}
	return( rc );
}
