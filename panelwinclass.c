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
 * $Id: panelwinclass.c,v 1.27 2025/09/16 15:52:30 kronos Exp $
 */

#include "ambient.h"

#if USE_INTERNAL_PANELS

#include <proto/gadtools.h>
#include <proto/dos.h>
#include <proto/timer.h>
#include <proto/graphics.h>
#include <mui/Listtree_mcc.h>
#include <libraries/asl.h>
/* public */

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "screen.h"
#include "prefs.h"
#include "threads.h"
#include "panelprefs.h"
#include "legacy.h"
#include "paneltags.h"
#include "panelclasslist.h"
#include "classes.h"

/************************************************************************/

#define PANELNAME_SIZEOF 300

struct Data {
	APTR pctx;
	ULONG panelwin_delete;
	ULONG attaching;
	ULONG position;
	ULONG fixed,depth;
	APTR tbar;
    APTR stayopen_mi;
    BOOL stayopen;
	BOOL settings_active;
	TEXT name[ PANELNAME_SIZEOF ];
};

/************************************************************************/

static void doset( APTR obj, struct Data *data, struct TagItem *tags )
{
	FORTAG( tags )
	{
		case MA_Panelwin_Group:
			data->tbar = (APTR) tag->ti_Data;
			break;
		case MA_Panelwin_Depth:
			data->depth = tag->ti_Data;
			if( getv( obj, MUIA_Window_Open ) )
			{
				SetAttrs( obj, MUIA_NoNotify, TRUE, MUIA_Window_Open,  FALSE, TAG_DONE );
				SetAttrs( obj, MUIA_Window_Open, TRUE, TAG_DONE ); /* XXX: what happens if the window doesn't open again ? */
			}
			break;
		case MA_Panelwin_Position:
			data->position = tag->ti_Data;
			if( data->position == MV_Panelwin_Position_Attached )
			{
				DoMethod( obj, MM_Panelwin_AttachToBorder, FALSE );
			}
			break;
		case  MA_Panelwin_Name:
			strcpy( data->name, (STRPTR) tag->ti_Data );
			break;
		case  MA_AmbientPanel_SettingsPanel:
			if(tag->ti_Data == 0) data->settings_active = FALSE;

	}
	NEXTTAG
}

/************************************************************************/

DEFNEW
{
	APTR tbar;
	APTR pctx;
	struct TagItem *ti;
	BOOL newpanel = FALSE;

	if( ( ti = FindTagItem( MA_Panelwin_Prefspool, INITTAGS ) ) )
	{
		pctx = (APTR) ti->ti_Data;
	} else {
		pctx = prefspool_create( panelprefs_getnewnum() );
		newpanel = TRUE;
	}

	if( !pctx )
	{
		PDB(("sigh, no prefspool\n"));
		return(0);
	}

	panelprefs_fix( pctx );

	setprefslong_default_ctx( pctx, DSI_PANELGROUP_SIZE                , MV_Panelgroup_Size_Large );
	setprefslong_default_ctx( pctx, DSI_PANELGROUP_DEPTH               , MV_Panelgroup_Depth_Normal );
	setprefslong_default_ctx( pctx, DSI_PANELGROUP_ISHORIZ             , TRUE );
	setprefslong_default_ctx( pctx, DSI_PANELGROUP_DRAGGADGET_PLACEMENT, MV_Panelgroup_DragMode_LeftUp );
	setprefslong_default_ctx( pctx, DSI_PANELGROUP_ZIPPING_ENABLED     , TRUE );

	setprefslong_default_ctx( pctx, DSI_PANELGROUP_POSMODE             , 0  );
	setprefslong_default_ctx( pctx, DSI_PANELGROUP_X                   , 400 );
	setprefslong_default_ctx( pctx, DSI_PANELGROUP_Y                   , 300 );
	setprefslong_default_ctx( pctx, DSI_PANELGROUP_LOCKED              , FALSE );
	setprefslong_default_ctx( pctx, DSI_PANELGROUP_BACKMODE            , MV_Panelgroup_BackMode_Color );
	setprefslong_default_ctx( pctx, DSI_PANELGROUP_BACKCOLOR           , 0xa0a0a0a0 );
	setprefslong_default_ctx( pctx, DSI_PANELGROUP_HIDEDRAGBAR         , 0 );
	setprefsstr_default_ctx ( pctx, DSI_PANELGROUP_BACKDROP            , "" );

	if( ( obj = DoSuperNew( cl, obj,
				MUIA_Window_Screen      , get_screen(),
				MUIA_Window_ScreenTitle , screentitle,
				MUIA_Window_CloseGadget , FALSE,
				MUIA_Window_Borderless  , TRUE,
				MUIA_Window_CloseGadget , FALSE,
				MUIA_Window_DepthGadget , FALSE,
				MUIA_Window_SizeGadget  , FALSE,
				MUIA_Window_DragBar     , FALSE,
				MUIA_Window_BackfillHook, LAYERS_NOBACKFILL,
				MUIA_Window_HasAlpha    , TRUE,
				EnableExtensions ? MUIA_Window_PanelWindow : TAG_IGNORE, TRUE,
				MUIA_Window_LeftEdge    , getprefslong_ctx( pctx, DSI_PANELGROUP_X ),
				MUIA_Window_TopEdge     , getprefslong_ctx( pctx, DSI_PANELGROUP_Y ),
				WindowContents,  tbar = (Object*) NewObject( getpanelgroupclass(), NULL,
												MUIA_Group_Horiz, TRUE,
												MUIA_FillArea, FALSE,
												MA_Panelgroup_Locked     , getprefslong_ctx( pctx, DSI_PANELGROUP_LOCKED ),
												MA_Panelgroup_Size       , getprefslong_ctx( pctx, DSI_PANELGROUP_SIZE ),
												MA_Panelgroup_Zipping    , getprefslong_ctx( pctx, DSI_PANELGROUP_ZIPPING_ENABLED ),
												MA_Panelgroup_Horiz      , getprefslong_ctx( pctx, DSI_PANELGROUP_ISHORIZ ),
												MA_Panelgroup_AutoZip    , getprefslong_ctx( pctx, DSI_PANELGROUP_AUTOZIP ),
												MA_Panelgroup_DragMode   , getprefslong_ctx( pctx, DSI_PANELGROUP_DRAGGADGET_PLACEMENT ),
												MA_Panelgroup_BackMode   , getprefslong_ctx( pctx,  DSI_PANELGROUP_BACKMODE ),
												MA_Panelgroup_BackColor  , getprefslong_ctx( pctx, DSI_PANELGROUP_BACKCOLOR ),
												MA_Panelgroup_Backdrop   , getprefsstr_ctx ( pctx, DSI_PANELGROUP_BACKDROP ),
												MA_Panelgroup_HideDragBar, getprefslong_ctx( pctx, DSI_PANELGROUP_HIDEDRAGBAR ),
												TAG_DONE ),
				TAG_DONE ) ) )
	{
		GETDATA;
		data->pctx     = pctx;
		data->depth    = getprefslong_ctx( pctx, DSI_PANELGROUP_DEPTH );
		data->position = getprefslong_ctx( pctx, DSI_PANELGROUP_POSMODE );
		data->tbar = tbar;
		data->settings_active = FALSE;
		SetAttrs( obj, MUIA_Window_DefaultObject, data->tbar                  , TAG_DONE );
		SetAttrs( obj, MUIA_Window_Width        , MUIV_Window_Width_MinMax(0) , TAG_DONE );
		SetAttrs( obj, MUIA_Window_Height       , MUIV_Window_Height_MinMax(0), TAG_DONE );

		if( newpanel )
		{
			panelprefs_add( obj, pctx );
			SetAttrs( tbar, MA_Panelgroup_New, TRUE, TAG_DONE );
		}
		panelprefs_getname( pctx, data->name );
	}
	return( (ULONG) obj );
}

/************************************************************************/

DEFGET
{
	GETDATA;
	ULONG result = TRUE;
	switch( msg->opg_AttrID )
	{
		case MA_Panelwin_Group:
			*msg->opg_Storage = (ULONG) data->tbar;
			break;
		case MA_Panelwin_PrefsNr:
			*msg->opg_Storage = prefspool_uid( data->pctx );
			break;
		case MA_Panelwin_Name:
			if( !( data->name ) )
			{
				panelprefs_getname( data->pctx, data->name );
			}
			*msg->opg_Storage = (ULONG) data->name;
			break;
		case MA_Panelwin_Type:
			*msg->opg_Storage = MV_Panelwin_Type_Root;
			break;
		case MA_Panelwin_Depth:
			*msg->opg_Storage = data->depth;
			break;
		case MA_Panelwin_Position:
			*msg->opg_Storage = data->position;
			break;
		case MA_Window_Type:
			*msg->opg_Storage = MV_Window_Type_Panel;
			break;
		case MA_Panelwin_Closed:
			/* Always TRUE just to trigger the notify. */
			*msg->opg_Storage = TRUE;
			break;
		default:
			result = DOSUPER;
	}
	return( result );
}

/************************************************************************/

DEFSET
{
	GETDATA;

	doset( obj, data, INITTAGS );

	return( DOSUPER );
}

/************************************************************************/

#define CORNER_ATTRACTION 6 /* percent of the screen size */

DEFSMETHOD(Panelwin_AttachToBorder)
{
	GETDATA;
	struct Window *win;

	if( data->position == MV_Panelwin_Position_Attached )
	{
		if( !data->attaching && ( win = (struct Window *) getv( obj, MUIA_Window_Window ) ) )
		{
			ULONG dir;
			ULONG dist, distcomp;
			ULONG tx = tx; /* shut up, gcc */
			ULONG ty = ty; /* really */
			struct Screen *scr = win->WScreen;
			ULONG was_offscreen = FALSE;

			/*
			 * If we are in offscreen mode, we need to
			 * put back the panels inside and ignore the rest.
			 */
			ty = win->TopEdge;
			tx = win->LeftEdge;

			if( win->TopEdge < 0 )
			{
				ty = 0;
				was_offscreen = TRUE;
			}

			if( win->LeftEdge < 0 )
			{
				tx = 0;
				was_offscreen = TRUE;
			}

			if( win->TopEdge + win->Height > scr->Height )
			{
				ty = scr->Height - win->Height;
				was_offscreen = TRUE;
			}

			if( win->LeftEdge + win->Width > scr->Width )
			{
				tx = scr->Width - win->Width;
				was_offscreen = TRUE;
			}

			/* XXX: we should check if a border is already set in the prefs! for example screenmode change.. the method might need a new parameter though */

			/*
			 * Find out to which border we
			 * have to attach.
			 */
			if( !was_offscreen )
			{
	            dir = BA_TOP;

				dist = win->TopEdge;
				distcomp = scr->Width - ( win->LeftEdge + win->Width );

				if( distcomp < dist )
				{
					dir = BA_RIGHT;
					dist = distcomp;
				}
				distcomp = scr->Height - ( win->TopEdge + win->Height );

				if( distcomp < dist )
				{
					dir = BA_BOTTOM;
					dist = distcomp;
				}
				distcomp = win->LeftEdge;

				if( distcomp < dist )
				{
					dir = BA_LEFT;
				}

				switch( dir )
				{
					case BA_LEFT:
						tx = 0;
						ty = win->TopEdge;
						break;

					case BA_RIGHT:
						tx = scr->Width - win->Width;
						ty = win->TopEdge;
						break;

					case BA_TOP:
						tx = win->LeftEdge;
						ty = 0;
						break;

					case BA_BOTTOM:
						tx = win->LeftEdge;
						ty = scr->Height - win->Height;
						break;
				}

				/*
				 * Find if we are near a corner, if so,
				 * attach to it.
				 */
				if( win->LeftEdge < scr->Width * CORNER_ATTRACTION / 100 )
				{
					tx = 0;
				}
				if( win->TopEdge < scr->Height * CORNER_ATTRACTION / 100 )
				{
					ty = 0;
				}
				if( scr->Width - ( win->LeftEdge + win->Width ) < scr->Width * CORNER_ATTRACTION / 100 )
				{
					tx = scr->Width - win->Width;
				}
				if( scr->Height - ( win->TopEdge + win->Height ) < scr->Height * CORNER_ATTRACTION / 100 )
				{
					ty = scr->Height - win->Height;
				}

				setprefslong_ctx( data->pctx, DSI_PANELGROUP_BORDERATTACH, dir );
			}

			do_action( obj, TA_Panels_Move,
											TT_Panels_Move_Window, win,
											TT_Panels_Move_X, tx,
											TT_Panels_Move_Y, ty,
											TAG_DONE );
		}
	}
	if( !data->position || msg->immediate_save )
	{
		/* save the new positions immediately */
		if( getprefslong( DSI_PANEL_AUTOSAVE_WINDOWPOS ) )
		{
			DoMethod( obj, MM_Panelwin_SaveConfig );
		}
	}
	return( 0 );
}
 
/************************************************************************/
 
DEFTMETHOD(Panelwin_SaveConfig)
{
	GETDATA;
	APTR bctxt;

	setprefslong_ctx( data->pctx, DSI_PANELGROUP_SIZE                , getv( data->tbar, MA_Panelgroup_Size ) );
	setprefslong_ctx( data->pctx, DSI_PANELGROUP_ZIPPING_ENABLED     , getv( data->tbar, MA_Panelgroup_Zipping ) );
	setprefslong_ctx( data->pctx, DSI_PANELGROUP_AUTOZIP             , getv( data->tbar, MA_Panelgroup_AutoZip ) );
	setprefslong_ctx( data->pctx, DSI_PANELGROUP_DEPTH               , data->depth);
	setprefslong_ctx( data->pctx, DSI_PANELGROUP_LOCKED              , getv( data->tbar, MA_Panelgroup_Locked ) );
	setprefslong_ctx( data->pctx, DSI_PANELGROUP_DRAGGADGET_PLACEMENT, getv( data->tbar, MA_Panelgroup_DragMode ) );
	setprefslong_ctx( data->pctx, DSI_PANELGROUP_HIDEDRAGBAR         , getv( data->tbar, MA_Panelgroup_HideDragBar ) );
	setprefslong_ctx( data->pctx, DSI_PANELGROUP_ISHORIZ             , getv( data->tbar, MA_Panelgroup_Horiz ) );
	setprefslong_ctx( data->pctx, DSI_PANELGROUP_POSMODE             , data->position );
	/*
	 * During window depth change, the window is closed
	 * during saving and would get 0/0 coordinates without
	 * the following check.
	 */
	if( getv( obj, MUIA_Window_Open ) )
	{
		setprefslong_ctx( data->pctx, DSI_PANELGROUP_X, getv( obj, MUIA_Window_LeftEdge ) );
		setprefslong_ctx( data->pctx, DSI_PANELGROUP_Y, getv( obj, MUIA_Window_TopEdge ) );
	} else {
		setprefslong_ctx( data->pctx, DSI_PANELGROUP_X, 0 );
		setprefslong_ctx( data->pctx, DSI_PANELGROUP_Y, 0 );
	}

	/*
	 * Now save the panel content.
	 */
	DoMethod( data->tbar, MM_Panelgroup_SaveConfig, data->pctx, 0 );

	if ( ( bctxt = prefspool_duplicate( data->pctx ) ) )
	{
		panelprefs_save( bctxt );
	}

	/* XXX */
	return( 0 );
}

/************************************************************************/

DEFTMETHOD(Panelwin_Close)
{
	GETDATA;
	ULONG deleting = FALSE;

	set( obj, MUIA_Window_Open, FALSE );
	set( obj, MA_Panelwin_Closed,TRUE ); /* trigger notify for perfeswin*/
	if( data->pctx )
	{
	//	  if (data->panelwin_delete)
		{
			/* XXX: there could be some other pending jobs.. how to abort them properly ? */
			deleting = do_action( obj, TA_Panels_Delete,
									TT_Panels_Delete_Ctx, data->pctx,
									TAG_DONE );
		}
	}

	if( !deleting )
	{
		/* we close the window in MM_Thread_Finished */
		threads_abort( obj, NULL );
	}
	return( 0 );
}

/************************************************************************/

DEFSMETHOD(Panelwin_Delete_Ok)
{
	switch( msg->butnum )
	{
		case 0: /* cancel */
			break;

		case 1: /* delete */
			{
				GETDATA;
				data->panelwin_delete = TRUE;
				DoMethod( app, MUIM_Application_PushMethod, obj, 1, MM_Panelwin_Close );
			}
			break;

		#ifdef DEBUG
		default:
			PDB(("no action\n"));
			break;
		#endif
	}
	return( 0 );
}

/************************************************************************/

DEFTMETHOD(Panelwin_FindRoot)
{
	return( (ULONG) obj );
}

/************************************************************************/

DEFMMETHOD(Window_Setup)
{
	GETDATA;
	/*
	 * Set proper window mode here.
	 */
	switch( data->depth )
	{
		case MV_Panelgroup_Depth_Normal:
			SetAttrs( obj, MUIA_Window_Backdrop, FALSE, MUIA_Window_Frontdrop, FALSE, TAG_DONE );
			break;

		case MV_Panelgroup_Depth_Back:
			SetAttrs( obj, MUIA_Window_Backdrop, TRUE, MUIA_Window_Frontdrop, FALSE, TAG_DONE );
			break;

		case MV_Panelgroup_Depth_Front:
			SetAttrs( obj, MUIA_Window_Backdrop, FALSE, MUIA_Window_Frontdrop, ( EnableExtensions ? TRUE : FALSE ), TAG_DONE );
			break;

		#ifdef DEBUG
		default:
			PDB(("wrong val\n"));
			break;
		#endif
	}

	/*
	 * If the user changed the screenmode this lets
	 * attached panels join their borders.
	 */
	if( data->position == MV_Panelwin_Position_Attached )
	{
		DoMethod( app, MUIM_Application_PushMethod, obj, 1, MM_Panelwin_AttachToBorder, FALSE );
	}
	return( DOSUPER );
}

/************************************************************************/

static ULONG sizeslidera_w[] = {
									MV_Panelgroup_Size_Micro,
									MV_Panelgroup_Size_Small,
									MV_Panelgroup_Size_Medium,
									MV_Panelgroup_Size_Large,
									MV_Panelgroup_Size_Huge,
									0
};

/************************************************************************/

static ULONG panel_size_to_slider( ULONG size )
{
	ULONG *p = sizeslidera_w;
	ULONG i = 0;

	while( *p )
	{
		if( *p == size )
		{
			break;
		}
		i++;
		p++;
	}
	return( i );
}

/************************************************************************/

static ULONG panel_slider_to_size( ULONG slider )
{
	return( sizeslidera_w[ slider ] );
}

/************************************************************************/


#if 0

/************************************************************************/

static ULONG panel_dragmode_to_cycle( ULONG dragmode )
{
	switch( dragmode )
	{
		case MV_Panelgroup_DragMode_None:
			return( 0 );

		case MV_Panelgroup_DragMode_LeftUp:
			return( 1 );

		case MV_Panelgroup_DragMode_RightBottom:
			return( 2 );

		case MV_Panelgroup_DragMode_Both:
			return( 3 );
	}
	PDB(("huho.. nodragmode %ld\n", dragmode));

	return( 0 );
}

/************************************************************************/

static ULONG panel_cycle_to_dragmode( ULONG cycle )
{
	switch( cycle )
	{
		case 0:
			return( MV_Panelgroup_DragMode_None );

		case 1:
			return( MV_Panelgroup_DragMode_LeftUp );

		case 2:
			return( MV_Panelgroup_DragMode_RightBottom );

		case 3:
			return( MV_Panelgroup_DragMode_Both );
	}
	PDB(("huho.. no cyclemode %ld\n", cycle));

	return( 0 );
}
#endif

/************************************************************************/

DEFSMETHOD(Panelwin_BackPen)
{
	GETDATA;
	ULONG pen,pen2;
	ULONG RGB[4];
	ULONG alpha;
	struct MUI_RenderInfo *mri;
	if(( data->settings_active ) && ( mri = muiRenderInfo( msg->poppen ) ))
	{
		pen = MUI_ObtainPen( mri, msg->penspec, 0 );
		pen2 = MUIPEN(pen);
		GetRGB32( mri->mri_Screen->ViewPort.ColorMap, pen2, 1, RGB );
		MUI_ReleasePen( mri, pen );

		get( data->tbar, MA_Panelgroup_BackColor, &alpha );
		alpha   = alpha    >> 24;
		RGB[ 0] = RGB[ 0 ] >> 24;
		RGB[ 1] = RGB[ 1 ] >> 24;
		RGB[ 2] = RGB[ 2 ] >> 24;
		set( data->tbar, MA_Panelgroup_BackColor, ( alpha << 24 ) + ( RGB[ 0 ] << 16 ) + ( RGB[ 1 ] << 8 ) + RGB[ 2 ] );
	}
	return( 0 );
}

/************************************************************************/

DEFSMETHOD(Panelwin_BackAlpha)
{
	GETDATA;
	ULONG color,alpha;

	get( data->tbar, MA_Panelgroup_BackColor, &color );
	alpha = msg->alpha << 24;
	color = ( color & 0x00FFFFFF ) + alpha;
	set( data->tbar, MA_Panelgroup_BackColor, color );

	return( 0 );
}

/************************************************************************/

DEFSMETHOD(Panelwin_Size)
{
	GETDATA;
	set( data->tbar, MA_Panelgroup_Size, panel_slider_to_size( msg->size ) );
	return( 0 );

}

/************************************************************************/


DEFSMETHOD(Panelwin_FindObject)
{
	GETDATA;
	BPTR searchlock,objlock;
	APTR /*tbar,*/child;
	APTR child_state;
	struct List *child_list;
	BOOL found = FALSE;
	if((searchlock = Lock(msg->pattern,SHARED_LOCK)))
	{
	//	PDB(("%x %x\n",searchlock,msg->object));
		if( ( GetAttr(MUIA_Group_ChildList,data->tbar,(ULONG*)&child_list ) ) && ( child_list ) )
		{					
			child_state = child_list->lh_Head;
			PDB(("%x\n",child_state));
			while( ( child = NextObject( &child_state ) ) && (found == FALSE))// *msg->object == 0 )  )
			{
				STRPTR uri;
				PDB(("%x\n",child));
				if(( GetAttr(MA_Panel_URI,child,(ULONG*)&uri ) ) && ( uri ) )
				{
					PDB(("%s %x %x!!\n",uri,child,*msg->object));
					if( ( objlock = Lock(uri,SHARED_LOCK) ) )
					{
						if( SameLock( searchlock,objlock ) == 	LOCK_SAME ) 
						{
							*msg->object = child;
							found = TRUE;
							PDB(("%x %x\n",*msg->object,child));
						}
						UnLock(objlock);
					}
				}
			}
			
		}
		UnLock(searchlock);
	}
	PDB(("%s %x\n",msg->pattern,msg->object));
	if (msg->object) return(TRUE);
	return ( FALSE );
}

DEFTMETHOD(Panel_Settings_Group)
{
	GETDATA;
	APTR name, reggroup;
	APTR backmode, alpha, backpen;
	APTR backasl,backasl_str;
	APTR cyc_ori, cyc_pos, sl_size, cyc_depth, cyc_placement;
	APTR bt_autozip, txt_autozip, hide_dragbar, bt_zip, txt_zip;
	ULONG backcolor,zip;
	ULONG background_mode;
	ULONG RGB[3];
	ULONG hasalpha;
	
	static STRPTR pages[ MSG_PANELWIN_BEHAVIOUR  - MSG_PANELWIN_NAME_LOOK + 2 ];



	static STRPTR cyc_modes     [ MSG_PANELWIN_MODES_FIXED    - MSG_PANELWIN_MODES_FLOATING  + 2 ];
	static STRPTR cyc_titles    [ MSG_PANELWIN_TITLES_HORIZONTAL - MSG_PANELWIN_TITLES_VERTICAL + 2 ];
	static STRPTR cyc_depths    [ MSG_PANELWIN_DEPTHS_FRONT      - MSG_PANELWIN_DEPTHS_NORMAL   + 2 ];
	static STRPTR cyc_placementh[ MSG_PANELWIN_PLACEMENTH_BOTH   - MSG_PANELWIN_PLACEMENTH_NONE + 2 ];
	static STRPTR cyc_backmodes [ MSG_PANELWIN_BACKMODE_IMAGE    - MSG_PANELWIN_BACKMODE_COLOR  + 2 ];
	
	backcolor = getv( data->tbar, MA_Panelgroup_BackColor );
	RGB[ 0 ] = ( backcolor >> 16 ) << 24;
	RGB[ 1 ] = ( backcolor >>  8 ) << 24;
	RGB[ 2 ] = ( backcolor << 24 );

	background_mode = getv( data->tbar, MA_Panelgroup_BackMode );

	MUIInitStringArray( (APTR) pages, MSG_PANELWIN_NAME_LOOK , MSG_PANELWIN_BEHAVIOUR  );
	
	reggroup = RegisterGroup( pages ),

		Child, ColGroup(2),
			Child, NSLabel1(MSG_PANELWIN_NAME),
			Child, name = MUICreateString( MSG_PANELWIN_NAME, PANELNAME_SIZEOF - 2, GSI(MSG_PANELWIN_NAME) ),
			Child, NSLabel1(MSG_PANELWIN_BACKGROUND),
			Child, HGroup,
				Child, backmode = MUICreateCycle( MSG_PANELWIN_BACKGROUND, cyc_backmodes, MSG_PANELWIN_BACKMODE_COLOR, MSG_PANELWIN_BACKMODE_IMAGE, "PANEBACK" ),
				Child, backpen = PoppenObject, End,
				Child, backasl = PopaslObject,
							MUIA_Popstring_String, backasl_str = KeyString( getv( data->tbar, MA_Panelgroup_Backdrop), 60, "n" ),
							MUIA_Popstring_Button, PopButton(MUII_PopUp),
							MUIA_ShowMe,FALSE,
							End,
				End,
				Child, NSLabel1(MSG_PANELWIN_ALPHA),
				Child, alpha = MUICreateSlider( MSG_PANELWIN_ALPHA, 0, 255, 255, "PANEALPH" ),
				Child, NSLabel(MSG_PANELWIN_ORIENTATION),
				Child, cyc_ori = MUICreateCycle( MSG_PANELWIN_ORIENTATION, cyc_titles, MSG_PANELWIN_TITLES_VERTICAL, MSG_PANELWIN_TITLES_HORIZONTAL, "PANEORIE" ),
				Child, NSLabel(MSG_PANELWIN_SIZE),
				Child, sl_size = NewObject( getpanelslidersizeclass(), NULL, MUIA_ControlChar, MUIGetUnderScore( MSG_PANELWIN_SIZE ), MUIA_ShortHelp, GSI(MSG_PANELWIN_SIZE_HELP), TAG_DONE ),
			End,
			Child, ColGroup(2),
				Child, NSLabel(MSG_PANELWIN_POSITIONING),
				Child, cyc_pos = MUICreateCycle( MSG_PANELWIN_POSITIONING, cyc_modes, MSG_PANELWIN_MODES_FLOATING, MSG_PANELWIN_MODES_FIXED, "PANEPOSI" ),
				Child, NSLabel(MSG_PANELWIN_DEPTH_ARRANGEMENT),
				Child, cyc_depth = MUICreateCycle( MSG_PANELWIN_DEPTH_ARRANGEMENT, cyc_depths, MSG_PANELWIN_DEPTHS_NORMAL, MSG_PANELWIN_DEPTHS_FRONT, "PANEDEPT" ),
				Child, NSLabel(MSG_PANELWIN_DRAG_GADGET),
				Child, cyc_placement = MUICreateCycle( MSG_PANELWIN_DRAG_GADGET, cyc_placementh, MSG_PANELWIN_PLACEMENTH_NONE, MSG_PANELWIN_PLACEMENTH_BOTH, "PANEDRAG" ),
				Child, HSpace(-1),
				Child, VGroup,
					Child, HGroup,
						Child, bt_zip  = MUICreateCheckbox( MSG_PANELWIN_ALLOW_ZIPPING, FALSE, "PANEZIPP" ),
						Child, txt_zip = NSLabel1(MSG_PANELWIN_ALLOW_ZIPPING),
						Child, HSpace(-1),
						Child, bt_autozip  = MUICreateCheckbox( MSG_PANELWIN_AUTO_ZIPPING, FALSE, "PANEAZIP" ),
						Child, txt_autozip = NSLabel1(MSG_PANELWIN_AUTO_ZIPPING),
						Child, HSpace(-1),
					End,
					Child, HGroup,
						Child,  hide_dragbar = MUICreateCheckbox(MSG_PANELWIN_HIDE_DRAGBAR, FALSE, NULL ),
						Child,  NSLabel1(MSG_PANELWIN_HIDE_DRAGBAR),
						Child, HSpace(-1),
					End,
				End,
			End,
		End;
	if(!reggroup) return 0;
	
	set( name, MUIA_String_Contents, data->name );
	backcolor = getv( data->tbar, MA_Panelgroup_BackColor );
	if( background_mode == MV_Panelgroup_BackMode_Color )
	{
		set( backpen, MUIA_ShowMe, TRUE  );
		set( backasl, MUIA_ShowMe, FALSE );
	}  	   
	else if( background_mode == MV_Panelgroup_BackMode_Picture )
	{
		set( backpen, MUIA_ShowMe, FALSE );
		set( backasl, MUIA_ShowMe, TRUE  );
	}
	DoMethod( backpen      , MUIM_Pendisplay_SetRGB, RGB[ 0 ], RGB[ 1 ], RGB[ 2 ] );
	SetAttrs( alpha        , MUIA_NoNotify, TRUE, MUIA_Slider_Level, backcolor >> 24, TAG_DONE );
	SetAttrs( backmode     , MUIA_NoNotify, TRUE, MUIA_Cycle_Active, getv( data->tbar, MA_Panelgroup_BackMode ), TAG_DONE );
	DoMethod( backasl_str  , MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, data->tbar, 3, MUIM_Set, MA_Panelgroup_Backdrop, MUIV_TriggerValue );
	SetAttrs( hide_dragbar , MUIA_NoNotify, TRUE, MUIA_Selected, getv( data->tbar, MA_Panelgroup_HideDragBar ), TAG_DONE );
	SetAttrs( sl_size      , MUIA_NoNotify, TRUE, MUIA_Slider_Level, panel_size_to_slider( getv( data->tbar, MA_Panelgroup_Size ) ),TAG_DONE );
	SetAttrs( cyc_ori      , MUIA_NoNotify, TRUE, MUIA_Cycle_Active, getv( data->tbar, MA_Panelgroup_Horiz ),TAG_DONE );
	SetAttrs( cyc_placement, MUIA_NoNotify, TRUE, MUIA_Cycle_Active, getv( data->tbar, MA_Panelgroup_DragMode ),TAG_DONE );
	SetAttrs( cyc_depth    , MUIA_NoNotify, TRUE, MUIA_Cycle_Active, getv( obj       , MA_Panelwin_Depth), TAG_DONE );
	SetAttrs( cyc_pos      , MUIA_NoNotify, TRUE, MUIA_Cycle_Active, getv( obj       , MA_Panelwin_Position), TAG_DONE);

	zip = getv( data->tbar, MA_Panelgroup_Zipping );
	SetAttrs(bt_zip    , MUIA_NoNotify, TRUE, MUIA_Selected, zip, TAG_DONE );
	SetAttrs(bt_autozip, MUIA_NoNotify, TRUE, MUIA_Selected, getv( data->tbar, MA_Panelgroup_AutoZip ), TAG_DONE );
	set( bt_autozip , MUIA_Disabled, !zip );
	set( txt_autozip, MUIA_Disabled, !zip );

	DoMethod( backmode     , MUIM_Notify, MUIA_Cycle_Active   , 0             , backpen   , 3, MUIM_Set, MUIA_ShowMe, TRUE );
	DoMethod( backmode     , MUIM_Notify, MUIA_Cycle_Active   , 1             , backpen   , 3, MUIM_Set, MUIA_ShowMe, FALSE );
	DoMethod( backmode     , MUIM_Notify, MUIA_Cycle_Active   , 1             , backasl   , 3, MUIM_Set, MUIA_ShowMe, TRUE );
	DoMethod( backmode     , MUIM_Notify, MUIA_Cycle_Active   , 0             , backasl   , 3, MUIM_Set, MUIA_ShowMe, FALSE );
	DoMethod( backmode     , MUIM_Notify, MUIA_Cycle_Active   , MUIV_EveryTime, data->tbar, 3, MUIM_Set, MA_Panelgroup_BackMode, MUIV_TriggerValue );
	DoMethod( backpen      , MUIM_Notify, MUIA_Pendisplay_Spec, MUIV_EveryTime, obj       , 3, MM_Panelwin_BackPen, MUIV_TriggerValue, backpen );
	DoMethod( alpha        , MUIM_Notify, MUIA_Slider_Level   , MUIV_EveryTime, obj       , 2, MM_Panelwin_BackAlpha, MUIV_TriggerValue);
	DoMethod( sl_size      , MUIM_Notify, MUIA_Numeric_Value  , MUIV_EveryTime, obj       , 2, MM_Panelwin_Size, MUIV_TriggerValue );
	DoMethod( cyc_ori      , MUIM_Notify, MUIA_Cycle_Active   , MUIV_EveryTime, data->tbar, 3, MUIM_Set, MA_Panelgroup_Horiz, MUIV_TriggerValue );
	DoMethod( cyc_pos      , MUIM_Notify, MUIA_Cycle_Active   , MUIV_EveryTime, obj       , 3, MUIM_Set, MA_Panelwin_Position, MUIV_TriggerValue );
	DoMethod( cyc_depth    , MUIM_Notify, MUIA_Cycle_Active   , MUIV_EveryTime, obj       , 3, MUIM_Set, MA_Panelwin_Depth, MUIV_TriggerValue );
	DoMethod( cyc_placement, MUIM_Notify, MUIA_Cycle_Active   , MUIV_EveryTime, data->tbar, 3, MUIM_Set, MA_Panelgroup_DragMode, MUIV_TriggerValue );
	DoMethod( hide_dragbar , MUIM_Notify, MUIA_Selected       , MUIV_EveryTime, data->tbar, 3, MUIM_Set, MA_Panelgroup_HideDragBar, MUIV_TriggerValue  );
	DoMethod( bt_zip       , MUIM_Notify, MUIA_Selected       , MUIV_EveryTime, data->tbar, 3, MUIM_Set, MA_Panelgroup_Zipping, MUIV_TriggerValue );
	DoMethod( bt_zip       , MUIM_Notify, MUIA_Selected       , MUIV_EveryTime, bt_autozip, 3, MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue );
	DoMethod( bt_autozip   , MUIM_Notify, MUIA_Selected       , MUIV_EveryTime, data->tbar, 3, MUIM_Set, MA_Panelgroup_AutoZip, MUIV_TriggerValue );
	DoMethod( name         , MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj       , 3, MUIM_Set, MA_Panelwin_Name,MUIV_TriggerValue );
	
	DoMethod( reggroup, MUIM_Notify,MUIM_Cleanup+2222,MUIV_EveryTime,obj,3,MUIM_Set,MUIM_Cleanup+2222,MUIV_TriggerValue);
	
	GetAttr( SA_OpacitySupport,(APTR)getv(obj,MUIA_Window_Screen), &hasalpha );
	if( ( hasalpha == SAOS_OpacitySupport_None ) || ( hasalpha == SAOS_OpacitySupport_OnOff ) )
	{
		SetAttrs( hide_dragbar, MUIA_Selected, FALSE, MUIA_Disabled, TRUE, TAG_DONE );
	}
	data->settings_active = TRUE;
	return( (ULONG) reggroup );
}

/************************************************************************/

BEGINMTABLE
DECNEW
DECGET
DECSET
DECTMETHOD(Panelwin_Close)
DECSMETHOD(Panelwin_Delete_Ok)
DECTMETHOD(Panelwin_SaveConfig)
DECTMETHOD(Panelwin_FindRoot)
DECSMETHOD(Panelwin_AttachToBorder)
DECMMETHOD(Window_Setup)
DECTMETHOD(Panel_Settings_Group)
DECSMETHOD(Panelwin_BackPen)
DECSMETHOD(Panelwin_BackAlpha)
DECSMETHOD(Panelwin_Size)
DECSMETHOD(Panelwin_FindObject)
ENDMTABLE

DECSUBCLASS_NC( MUIC_Window, panelwinclass )
#endif
