/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
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
 * $Id: panelitem_listclass.c,v 1.18 2025/09/16 15:52:30 kronos Exp $
 */

#include "ambient.h"

#if USE_INTERNAL_PANELS

/* public */

/* private */
#include "mui_func.h"
#include "panelitem.h"
#include "gfx_bitmap.h"
#include "panelclasslist.h"
#include "paneltags.h"

/************************************************************************/

struct Data {
	int dummy;
};

/************************************************************************/

static APTR create_image( APTR obj, APTR bm )
{
	APTR iobj;

	if ( ( iobj = BitmapObject,
		MUIA_Bitmap_Bitmap, gfx_bitmap_bm( bm ),
		MUIA_Bitmap_Height, PANELIMAGE_HEIGHT, 
		MUIA_Bitmap_Width,  PANELIMAGE_WIDTH,
		MUIA_FixHeight,     PANELIMAGE_HEIGHT, 
		MUIA_FixWidth,      PANELIMAGE_WIDTH,
		MUIA_Bitmap_Alpha,  0xffffffff,
		End ) )
	{
		return( (APTR) DoMethod( obj, MUIM_List_CreateImage, iobj, 0 ) );
	}

	return( NULL );
}

/************************************************************************/

DEFMMETHOD( List_Display )
{
	#define DISPLAYTMP_SIZEOF 40
	static TEXT tmp[ DISPLAYTMP_SIZEOF ];
	struct PanelItem *pi = msg->entry;

	if( !pi->pi_Image && pi->pi_BitMap ) /* no image, but bitmap -> create image */
	{
		pi->pi_Image = create_image( obj, pi->pi_BitMap ); /* XXX is it a good idea to create stuff in list display method? (geit) */
	}

	if( pi->pi_Image )
	{
		snprintf( tmp, sizeof( tmp ), "\033O[%08lx] %s", (ULONG) pi->pi_Image, pi->pi_DisplayName );
	} else {
		snprintf( tmp, sizeof( tmp ), "%s", pi->pi_DisplayName );
	}

	msg->array[0] = tmp;

	return (0);
}

/************************************************************************/

DEFMMETHOD(List_Destruct)
{
	PanelItem_Delete( (struct PanelItem *) msg->entry );

	return (0);
}

/************************************************************************/

DEFNEW
{
//	struct Data *data;
	struct PanelItem *pi;
	struct List *classlist;

	if( ( obj = DoSuperNew( cl, obj,
								InputListFrame,
								MUIA_List_MinLineHeight, PANELIMAGE_HEIGHT,
								TAG_MORE, INITTAGS ) ) )
	{

//		data = INST_DATA( cl, obj );

		/*  create internal panel items. */
		{
			LONG pti;

			for( pti = 0 ; pti < PT_END ; pti++ ) {
				if( ( pi = PanelItem_Create( pti, NULL ) ) ) { /* insert only if creation succeded. */
					DoMethod( obj, MUIM_List_InsertSingle, pi, MUIV_List_Insert_Bottom );
				}
			}
		}
		/*  create external panel items. */
		if( ( classlist = GetPanelclassList() ) )
		{
			struct Node *node;
			ITERATELIST( node, classlist )
			{
				if( ( pi = PanelItem_Create( PT_EXTERNAL, node->ln_Name ) ) ) {
					DoMethod( obj, MUIM_List_InsertSingle, pi, MUIV_List_Insert_Bottom );
				}
			}
		}
	}
	return( (ULONG) obj );
}

/************************************************************************/

DEFGET
{
	switch( msg->opg_AttrID )
	{
		case MA_Panelitem_list_IsList:
			*msg->opg_Storage = TRUE;
			return( TRUE );
	}
	return( DOSUPER );
}

/************************************************************************/

DEFTMETHOD( PanelItemList_RefreshList )
{
	APTR classlistobj, classlist = 0;
	struct PanelItem *pi;
	ULONG entries, i;

	set( obj, MUIA_List_Quiet, TRUE );
	get( obj, MUIA_List_Entries, &entries );

	/* remove all external classes from list*/
	for( i = entries ; i > 0; i-- )
	{
		DoMethod( obj, MUIM_List_GetEntry, i, (APTR*) &pi );
		if( ( pi ) && ( pi->pi_Type == PT_EXTERNAL ) ) {
			DoMethod( obj, MUIM_List_Remove, i );
		}
	}
	/* rescan for new disk based classes and create them all */
	get( _app( obj ), MA_Application_PanelClassList, (ULONG*) &classlistobj );
	if( ( classlistobj ) && ( classlist = GetPanelclassList() ) )
	{
		struct Node  *node;

		DoMethod( classlistobj,MM_Panelclasslist_RefreshClasslist ); /* scan for new classes */

		ITERATELIST( node, classlist )
		{
			if( ( pi = PanelItem_Create( PT_EXTERNAL, node->ln_Name ) ) ) /* create external classes */
			{
				DoMethod( obj, MUIM_List_InsertSingle, pi, MUIV_List_Insert_Bottom );   /* and re-insert them */
			}
		}
	}
	set( obj, MUIA_List_Quiet, FALSE );

	return(0);
}

/************************************************************************/

DEFMMETHOD(Setup)
{
	ULONG i;
	struct PanelItem *pi;

	if( !DOSUPER )
	{
		return( FALSE );
	}
	
	for( i = 0 ; ; i++ )
	{
		DoMethod( obj, MUIM_List_GetEntry, i, &pi );

		if( pi ) {
			if( !pi->pi_Image && pi->pi_BitMap ) { /* no image, but bitmap -> create image */
				pi->pi_Image = create_image( obj, pi->pi_BitMap );
			}
		} else {
			break;
		}
	}

	return( TRUE );
}

/************************************************************************/

DEFMMETHOD( Cleanup )
{
	ULONG i;
	struct PanelItem *pi;

	for ( i = 0; ; i++ )
	{
		DoMethod( obj, MUIM_List_GetEntry, i, &pi );

		if( pi ) {
			if( pi->pi_Image ) {
				DoMethod( obj, MUIM_List_DeleteImage, pi->pi_Image );
				pi->pi_Image = NULL;
			}
		} else {
			break;
		}
	}
	return( DOSUPER );
}

/************************************************************************/

BEGINMTABLE
DECNEW
DECGET
DECMMETHOD(List_Display)
DECMMETHOD(List_Destruct)
DECTMETHOD(PanelItemList_RefreshList)
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
ENDMTABLE

DECSUBCLASS_NC(MUIC_List, panelitem_listclass)
#endif