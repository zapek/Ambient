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
 * $Id: panelitem.c,v 1.1 2026/02/08 13:43:39 kronos Exp $
 */


#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/panel.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>
#include <string.h>
/* public */


#include <proto/ambient.h>
#include <libraries/ambient.h>
#include "prefs.h"
/* private */
//include "ambient_cat.h"
#include "panelitem.h"
//#include "gfx_bitmap.h"
//#include "gfx_blit.h"
#warning
//#include "panelitem_button_logo.h"
//#include "mui_func.h"
#include "../panel/paneltags.h"
#include "../panel/MUIClasses.h"
//#include "debug.h"
//#include "panelclasslist.h"

/************************************************************************/


//TEXT PANEL_DISKPATHSYS[] = "SYS:Classes/Panels";
//TEXT PANEL_DISKPATHMOSSYS[] = "MOSSYS:Classes/Panels";

/************************************************************************/


#warning pfusch
struct prefsnode {
	struct MinNode n;
	ULONG id;
	ULONG size;
	UBYTE data[0];
};

void PanelItem_Insert( APTR ppool, ULONG ID,struct PanelItem *panelitem,ULONG sub_ID)
{
	APTR pl,pi = 0;
	APTR d;
	ULONG size;
	ULONG i = ID;
	if((pl = (APTR)GetPPoolItem(ppool,0,  DSI_LISTPOOL_PANEL ,NULL,&size)))
	{
		struct prefsnode* plp =(struct prefsnode*)pl;
		kprintf("DSI_LISTPOOL_PANEL %x %d\n",pl,plp->size);
		while((pi = (APTR)GetPPoolItem(ppool, pl, i| DSF_LISTPOOL , &d, &size)))
		{
			plp =(struct prefsnode*)pi;
			kprintf("PANEL %x %d %x %d\n",DSF_LISTPOOL,i,plp->id,plp->size);
			i++;
			//		ChangePPoolItemID(ppool,pl, i| DSF_LISTPOOL,i-1);
		}
		while(i > ID)
		{
			i--;
			ChangePPoolItemID(ppool,pl, i| DSF_LISTPOOL,(i+1)| DSF_LISTPOOL);
		}
		if((pi = (APTR) AddPPoolItem(ppool, pl, ID | DSF_LISTPOOL, NULL, (ULONG)NULL )))
		{
			AddPPoolItem(ppool,pi, DSI_LISTPOOL_PANEL_TYPE, &panelitem->pi_Type, 4 );
			AddPPoolItem(ppool,pi, DSI_LISTPOOL_PANEL_SUBPANEL, &sub_ID, 4 );
			if(panelitem->pi_Type == MV_Panel_Type_External)// PT_EXTERNAL)
			{
				kprintf("extern %s\n",panelitem->pi_ClassName);
				AddPPoolItem(ppool, pi, DSI_LISTPOOL_PANEL_EXT_NAME, (APTR) panelitem->pi_ClassName , strlen(panelitem->pi_ClassName)+1);
			}				
		}
		kprintf("panelitem insert %d %d %x %d type %d %d\n",ID,i,pl,sub_ID,panelitem->pi_Type,PT_EXTERNAL);
		if(PanelBase)ReLoadPanels();
		//
	/*	switch (pi->pi_Type)
		{
			case PT_BUTTON:
				
				break;
			case PT_SPACER:
				kprintf("add spacer\n");
				break;
		}*/
	}		
}



struct PanelItem *PanelItem_Create( ULONG type, STRPTR panelname )
{
struct PanelItem *pi = NULL;
	#warning !
APTR panelimage = 0;//panelitem_button; 
	/* this image is used if class
									** does not provide own image
									*/
#if 1
	if ( ( pi = AllocVecTaskPooled( sizeof( struct PanelItem ) ) ) )
	{
		memset( pi, 0, sizeof( struct PanelItem ) );
		pi->pi_Type  = 0;

		switch( type )
		{

			case PT_BUTTON:
			//	PanelItem_Delete( pi );
				pi = NULL;
				break;

			case PT_SPACER:
				pi->pi_Type = MV_Panel_Type_Spacer;
				pi->pi_Object = (Object*)CreatePanelItem(MV_Panel_Type_Spacer, 0);
				break;

			case PT_SEPARATOR:
				pi->pi_Type = MV_Panel_Type_Separator;
				pi->pi_Object = (Object*)CreatePanelItem(MV_Panel_Type_Separator, 0);
				break;

			case PT_VIEWWATCHER:
				pi->pi_Type = MV_Panel_Type_ViewWatcher;
				pi->pi_Object = (Object*)CreatePanelItem(MV_Panel_Type_ViewWatcher, 0);
			//	pi->pi_Class       = getpanelviewwatcherclass();
				break;

			case PT_BOOKMARKS:
				pi->pi_Type = MV_Panel_Type_Bookmarks;
				pi->pi_Object = (Object*)CreatePanelItem(MV_Panel_Type_Bookmarks, 0);
			//	pi->pi_Class       = getpanelbookmarksclass();
				break;

			case PT_SUBPANEL:
				pi->pi_Type = MV_Panel_Type_SubPanel;
				pi->pi_Object = (Object*)CreatePanelItem(MV_Panel_Type_SubPanel, 0);
			//	pi->pi_Class       = GetClass("SubPanelButton");
				break;

			case PT_DIRPANEL:
			//	pi->pi_Class       = getpaneldirpanelbuttonclass();
				break;
#if 1
			case PT_EXTERNAL:  /* try to create a panel object */
				pi->pi_Type = MV_Panel_Type_External;
				{
				#define PANELNAME_SIZEOF 200
				TEXT panelpath[ PANELNAME_SIZEOF ];
#warning proper path
					
					//	strcpy( panelpath, PANEL_DISKPATHMOSSYS );
					strcpy( panelpath, "mossys:Classes/Panels2" );
					AddPart( panelpath, panelname, PANELNAME_SIZEOF );
				//	kprintf("a %x %s\n",pi,panelpath);
					/* try to flush before open (required for rescan button) */

					{ struct Library *flush;
					if( !( flush = OpenLibrary( panelpath, 0 ) ) ) {  /* try to open from MOSSYS: */
						if( !( flush = OpenLibrary( &panelpath[3], 0 ) ) ) { /* try to open from SYS: */
							PanelItem_Delete( pi ); /* no class found, so kill our self */
							pi = NULL;
							break;
						}
					}
				//	kprintf("b %x %s\n",pi,panelpath);
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
			//		kprintf("cC %x %x\n",pi,pi->pi_Object);
				}
				break;
#endif
			default:
				
			//	PanelItem_Delete( pi );
				pi = NULL;
				break;
		}
		/* get data from class */
		if( pi ) 
			{
	//		PDB(("%x %x %x\n",pi,pi->pi_Class,pi->pi_Object));
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
			//	PDB(("%d %s\n",pi->pi_Version,pi->pi_Author)); 
				if( obj != pi->pi_Object ) { /* do not free real object */
					DisposeObject( obj );
				//	kprintf("d %x %x\n",pi,pi->pi_Object);
				}
			}
			if( !pi->pi_DisplayName || !pi->pi_Author || !pi->pi_Description ) {
				PanelItem_Delete( pi ); /* we reject any panel which is not properly implemented */
				pi = NULL;
			}
		//	kprintf("e %x %x\n",pi,pi->pi_Object);
		}
		if ( pi )//&& panelimage )
		{
		#warning
		/*	if ( ( pi->pi_BitMap = gfx_bitmap_create( PANELIMAGE_WIDTH, PANELIMAGE_HEIGHT, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE ) ) )
			{
				gfx_blit( panelimage, pi->pi_BitMap,
					BLITTAG_SrcType, BLITVAL_SrcType_Array,
					BLITTAG_SrcFormat, BLITVAL_SrcFormat_ARGB,
				TAG_DONE);
			} *//* If it fails, no image then */
		}
	}
	#endif
	return( pi );
}

/************************************************************************/

void PanelItem_Delete( struct PanelItem *pi )
{
	#warning
#if 1
	if( pi ) {

		if( pi->pi_BitMap ) { /* free bitmap if exists */
		//	gfx_bitmap_delete( pi->pi_BitMap );
			//pi->pi_BitMap = NULL;
		}
		if( pi->pi_Object ) { /* dispose object if available */
			MUI_DisposeObject( pi->pi_Object );
			pi->pi_Object = NULL;
		}
	//	free( pi );
	}
#endif
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

