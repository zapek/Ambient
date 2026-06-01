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
 * $Id: panelitem.c,v 1.25 2025/09/16 15:52:29 kronos Exp $
 */

#include "ambient.h"

#if USE_INTERNAL_PANELS

#include <proto/dos.h>
/* public */

/* private */
#include "ambient_cat.h"
#include "panelitem.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "panelitem_button_logo.h"
#include "mui_func.h"
#include "paneltags.h"
#include "panelclasslist.h"

/************************************************************************/

#ifndef DEPEND
#if PANELIMAGE_WIDTH != PANELITEM_BUTTON_WIDTH
#error "panelitem_button width doesn't match"
#endif

#if PANELIMAGE_HEIGHT != PANELITEM_BUTTON_HEIGHT
#error "panelitem_button height doesn't match"
#endif
#endif

TEXT PANEL_DISKPATHSYS[] = "SYS:Classes/Panels";
TEXT PANEL_DISKPATHMOSSYS[] = "MOSSYS:Classes/Panels";

/************************************************************************/

struct PanelItem *PanelItem_Create( ULONG type, STRPTR panelname )
{
struct PanelItem *pi = NULL;
APTR panelimage = panelitem_button; /* this image is used if class
									** does not provide own image
									*/

	if ( ( pi = malloc( sizeof( struct PanelItem ) ) ) )
	{
		memset( pi, 0, sizeof( struct PanelItem ) );
		pi->pi_Type     = type;

		switch( type )
		{

			case PT_BUTTON:
				PanelItem_Delete( pi );
				pi = NULL;
				break;

			case PT_SPACER:
				pi->pi_Class       = getpanelspacerclass();
				break;

			case PT_SEPARATOR:
				pi->pi_Class       = getpanelseparatorclass();
				break;

			case PT_VIEWWATCHER:
				pi->pi_Class       = getpanelviewwatcherclass();
				break;

			case PT_BOOKMARKS:
				pi->pi_Class       = getpanelbookmarksclass();
				break;

			case PT_SUBPANEL:
				pi->pi_Class       = getpanelsubpanelbuttonclass();
				break;

			case PT_DIRPANEL:
				pi->pi_Class       = getpaneldirpanelbuttonclass();
				break;

			case PT_EXTERNAL:  /* try to create a panel object */
				{
				#define PANELNAME_SIZEOF 200
				TEXT panelpath[ PANELNAME_SIZEOF ];
					strcpy( panelpath, PANEL_DISKPATHMOSSYS );
					AddPart( panelpath, panelname, PANELNAME_SIZEOF );

					/* try to flush before open (required for rescan button) */

					{ struct Library *flush;
					if( !( flush = OpenLibrary( panelpath, 0 ) ) ) {  /* try to open from MOSSYS: */
						if( !( flush = OpenLibrary( &panelpath[3], 0 ) ) ) { /* try to open from SYS: */
							PanelItem_Delete( pi ); /* no class found, so kill our self */
							pi = NULL;
							break;
						}
					}
					if( flush ) {
						RemLibrary( flush );
						CloseLibrary( flush );
					}
					}
					/* now try to create panel object */

					if( !( pi->pi_Object = MUI_NewObject( panelpath, TAG_DONE ) ) ) { /* try to open from MOSSYS: */
						if( !( pi->pi_Object = MUI_NewObject( &panelpath[3], TAG_DONE ) ) ) { /* try to open from SYS: */
							PanelItem_Delete( pi ); /* no class found, so kill our self */
							pi = NULL;
							break;
						}
					}
				}
				break;

			default:
				PDB(("\n\n### type %ld is out of bounds\n", type ));
				PanelItem_Delete( pi );
				pi = NULL;
				break;
		}
		/* get data from class */
		if( pi ) {
			Object *obj = pi->pi_Object;
			if( !obj && pi->pi_Class ) { /* no object then create one */
				obj = NewObject( pi->pi_Class, NULL, TAG_DONE );
			}
			if( obj ) {
				get( obj, MA_Panel_Extern_ClassName,   (ULONG*) &pi->pi_ClassName   );
				get( obj, MA_Panel_Extern_DisplayName, (ULONG*) &pi->pi_DisplayName );
				get( obj, MA_Panel_Extern_Author,      (ULONG*) &pi->pi_Author      );
				get( obj, MA_Panel_Extern_Description, (ULONG*) &pi->pi_Description );
				get( obj, MA_Panel_Extern_Version,     (ULONG*) &pi->pi_Version     );
				get( obj, MA_Panel_Extern_Revision,    (ULONG*) &pi->pi_Revision    );
				get( obj, MA_Panel_Extern_Image,       (ULONG*) &panelimage         );
				if( obj != pi->pi_Object ) { /* do not free real object */
					DisposeObject( obj );
				}
			}
			if( !pi->pi_DisplayName || !pi->pi_Author || !pi->pi_Description ) {
				PanelItem_Delete( pi ); /* we reject any panel which is not properly implemented */
				pi = NULL;
			}
		}
		if ( pi && panelimage )
		{
			if ( ( pi->pi_BitMap = gfx_bitmap_create( PANELIMAGE_WIDTH, PANELIMAGE_HEIGHT, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE ) ) )
			{
				gfx_blit( panelimage, pi->pi_BitMap,
					BLITTAG_SrcType, BLITVAL_SrcType_Array,
					BLITTAG_SrcFormat, BLITVAL_SrcFormat_ARGB,
				TAG_DONE);
			} /* If it fails, no image then */
		}
	}
	return( pi );
}

/************************************************************************/

void PanelItem_Delete( struct PanelItem *pi )
{

	if( pi ) {

		if( pi->pi_BitMap ) { /* free bitmap if exists */
			gfx_bitmap_delete( pi->pi_BitMap );
			//pi->pi_BitMap = NULL;
		}
		if( pi->pi_Object ) { /* dispose object if available */
			MUI_DisposeObject( pi->pi_Object );
			//pi->pi_Object = NULL;
		}
		free( pi );
	}
}

/************************************************************************/

LONG PanelItem_NameToType( CONST_STRPTR name )
{
	if( !stricmp( "button", name ) ) {
		return( MV_Panel_Type_Button );
	} else {
		if( !stricmp( "spacer", name ) ) {
			return( MV_Panel_Type_Spacer );
		} else {
			if(!stricmp( "separator", name ) ) {
				return( MV_Panel_Type_Separator );
			} else {
				if( !stricmp( "viewwatcher", name ) ) {
					return( MV_Panel_Type_ViewWatcher );
				} else {
					if( !stricmp( "bookmarks", name ) ) {
						return( MV_Panel_Type_Bookmarks );
					} else {
						if( !stricmp( "subpanel", name ) ) {
							return( MV_Panel_Type_ViewWatcher );
						} else {
							if( !stricmp( "dirpanel", name ) ) {
								return( MV_Panel_Type_ViewWatcher );
							} else { /* XXX: Oh rly? */
								return( MV_Panel_Type_Button );
							}
						}
					}
				}
			}
		}
	}
}
#endif
