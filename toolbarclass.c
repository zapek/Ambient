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
 * $Id: toolbarclass.c,v 1.19 2017/08/28 14:51:22 cyfm Exp $
 */

#include "ambient.h"

/* public */
#include <proto/dos.h>
#include <devices/rawkeycodes.h>

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "history.h"
#include "keymap.h"
#include "prefs.h"
#include "toolbarclass.h"
#include "viewapi.h"
#include "rexx.h"
#include "command.h"
#include "tags.h"
#include "mimeuri.h"
#include "file_func.h"
#include "strings.h"
#include "storage.h"
#include "legacy.h"


struct Data {
	APTR clickpath;
	APTR location;
	APTR str_location;

	STRPTR path;

	APTR history;
	APTR pages;

	APTR grp_viewchange;
	APTR but_view_icons; // bitRocky
	APTR but_view_list; // bitRocky
	APTR grp_viewbuts; // bitRocky: grp with the two view buttons
	APTR cyc_viewmodes;

	struct MUI_EventHandlerNode ehnode;

	STRPTR viewmodes[ 32 ];	   /* much more than enough */
	STRPTR views[ 32 ];	       /* much more than enough */
	STRPTR viewlabels[ 32 ];   /* much more than enough */
	ULONG viewsmap[ 32 ];      /* allows us to map view id into cycle position */

};

static void doset( APTR obj UNUSED, struct Data *data, struct TagItem *tags )
{
	FORTAG(tags)
	{
		case MA_Toolbar_Path:
		{
			CONST_STRPTR copyme;
			STRPTR p;
			TEXT buff[ 512 ]; /* should be enough(tm) */
			STRPTR path;
			
			path = (STRPTR)tag->ti_Data;
			copyme = buff;

			if (path && *path)
			{
				/* try to expand assigns */

				if ( path_expand( path, buff, sizeof( buff ) ) )
				{
					p = malloc(strlen( buff ) + 1);
				}
				else
				{
					copyme = path;
					p = malloc(strlen( path ) + 1 );
				}
			}
			else
			{
				copyme = "devices://";
				p = malloc(sizeof("devices://") - 1 + 1);
			}

			if ( p )
			{
				strcpy(p, copyme);

				if ( data->path && 0 == strcmp( data->path, p ) )
				{
					free( p );
					break;
				}
				else
				{
					if (data->path)
					{
						free(data->path);
					}
				
					data->path = p;
				}

				set(data->location, MUIA_String_Contents, p);
				set(data->clickpath, MA_Clickpath_Path,
					strcmp("devices://", p) == 0 ? (STRPTR) "" : p
				);
				
			}
			break;
		}
	}
	NEXTTAG
}


/*
 *
 */
APTR toolbar_build_tray( CONST UBYTE *barcode, LONG draggable, LONG viewall UNUSED )
{
	APTR obj;

	obj =	NewObject( gettoolbargroupclass(), NULL,
												MA_Toolbargroup_Definition, barcode,
												MA_Toolbargroup_Draggable , draggable,
												MUIA_Group_HorizSpacing   , 0,
												MUIA_Frame                , MUIV_Frame_None,
												InnerSpacing( 0, 0 ),
												TAG_DONE );

	return( obj );
}


DEFNEW
{
	APTR location, clickpath;
	APTR pages, toolbar;
	APTR grp_viewchange;

	obj = DoSuperNew(cl, obj,
		VirtualFrame,
		MUIA_Background, MUII_GroupBack,
		MUIA_FrameVisible, FALSE,
		Child, VGroup, /*NewObject(getvirtgroupclass(), NULL,*/
			Child, toolbar = toolbar_build_tray(_conf(toolbar_definition), FALSE, FALSE),

			Child, pages = PageGroup,
				Child, HGroup,
					Child, clickpath = NewObject(getclickpathclass(), NULL, TAG_DONE),
					Child, RectangleObject, MUIA_Rectangle_VBar, TRUE, MUIA_Weight, 0, End,
					Child, grp_viewchange = HGroup, /*MUIA_Group_HorizSpacing, 0,*/ MUIA_Weight, -1, MUIA_Font, MUIV_Font_Tiny, End,
				End,
				Child, HGroup,
					Child, location = NewObject(getnavigationclass(), NULL, MA_Navigation_MaxHistoryItems, 32,
					                            MA_Navigation_StorageID, STORAGE_CHANGE_LOCATION_HISTORY,
					                            MA_Navigation_DiskStorage, FALSE,
				    	                        TAG_DONE),
				End,
			End,
		End,

		TAG_MORE, INITTAGS
	);

	if (obj)
	{
		struct Data *data;
		APTR str_location = (APTR)getv( location, MA_Navigation_StringObject );

		data = INST_DATA(cl, obj);

		data->clickpath = clickpath;
		data->location = location;
		data->pages = pages;
		data->grp_viewchange = grp_viewchange;
		data->cyc_viewmodes = NULL;
		data->grp_viewbuts = data->but_view_icons = data->but_view_list = NULL; // bitRocky
		data->str_location = str_location;

		/* setup notifications for navigation */

		DoMethod(clickpath, MUIM_Notify, MA_Clickpath_Position, MUIV_EveryTime, obj, 1, MM_Toolbar_NewPath);

		DoMethod(str_location, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
			obj, 1, MM_Toolbar_LocationAcknowledge
		);

		/* initialize history context */

		data->history = history_create( (APTR)GetTagData( MA_Toolbar_Viewobj, (ULONG)NULL, INITTAGS ) );

		DoMethod( (APTR)GetTagData( MA_Toolbar_Viewobj, (ULONG)NULL, INITTAGS ), MUIM_Notify, MA_Viewgroup_ViewChanged, TRUE,
			obj, 1, MM_Toolbar_ViewChanged );

		/* Initialize cycle for view and viewmode */

		DoMethod( obj, MM_Toolbar_ViewChanged );

		DoMethod(location, MM_Navigation_LoadHistory);
	}

	return ((ULONG)obj);
}


DEFDISP
{
	GETDATA;

	if (data->path)
		free(data->path);

	if (data->history)
		history_delete(data->history);

	return (DOSUPER);
}


DEFSET
{
	GETDATA;

	doset(obj, data, INITTAGS);

	return (DOSUPER);
}

DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_Toolbar_History:
			*msg->opg_Storage = (ULONG)data->history;
			return (TRUE);

	}
	return (DOSUPER);
}


DEFSMETHOD(Toolbar_HistoryAddURI)
{
	GETDATA;

	if	( !history_getattr( data->history, HISTORYTAG_LOCKED ) )
	{
		/* create new node and add it to history */

		APTR hn = historynode_create( msg->path, msg->mode );
		history_addnode( data->history, hn );

		/* setup string. */

		set( obj, MA_Toolbar_Path, msg->path );

	}

	/* unlock */

	history_setattrs( data->history,
		HISTORYTAG_LOCKED, FALSE,
	TAG_DONE);

	return 0;
}


DEFSMETHOD(Toolbar_HistoryUpdateURI)
{
	GETDATA;

	/* update current history node */

	APTR hn = history_getattr( data->history, HISTORYTAG_NODE );

	if ( hn )
	{
		ULONG changed = FALSE;

		if ( msg->path )
		{
			changed |= historynode_setattrs( hn,
				HISTORYNODETAG_PATH, msg->path,
			TAG_DONE);
		}

		if ( msg->mode )
		{
			historynode_setattrs( hn,
				HISTORYNODETAG_MODENAME, msg->mode,
			TAG_DONE);
		}

		/* setup string. */

		if ( changed )
		{	 
			set( obj, MA_Toolbar_Path, historynode_getattr( hn, HISTORYNODETAG_PATH ) );
		}

		DoMethod( obj, MM_Toolbar_ViewChanged );
	}

	return (0);
}

DEFTMETHOD(Toolbar_HistoryNext)
{
	return DoMethod( obj, MM_Toolbar_HistoryMove, 1 );
}


DEFTMETHOD(Toolbar_HistoryPrev)
{
	return DoMethod( obj, MM_Toolbar_HistoryMove, -1 );
}

DEFSMETHOD(Toolbar_HistoryMove)
{
	GETDATA;

	LONG steps = msg->steps;
	LONG pos = (ULONG)history_getattr( data->history, HISTORYTAG_POSITION );
	LONG entries = (ULONG)history_getattr( data->history, HISTORYTAG_SIZE );
	APTR hn;

	/* check if we have more entries in history */

	if ( steps < 0 )	/* move backwards */
	{
		if ( pos == 0 )
			return 0;

		pos = max( 0, pos + steps );
	}
	else if ( steps > 0 )	 /* move forwards */
	{
		if ( pos == entries - 1 )
			return 0;

		pos = min( entries - 1, pos + steps );

	}
	else
		return 0;

	/* update current history node */

	DoMethod( _win(obj), MM_Window_UpdateURI );

	/* get new entry entry */

	history_setattrs( data->history,
		HISTORYTAG_POSITION, pos,
		TAG_DONE);

	hn = history_getattr( data->history, HISTORYTAG_NODE );

	if ( hn )
	{
		/* lock history for the time of loading new URI */

		history_setattrs( data->history,
			HISTORYTAG_LOCKED, TRUE,
		TAG_DONE );

		/* let windowclass execute it */

		DoMethod(_win(obj), MM_Window_LoadURI, historynode_getattr( hn, HISTORYNODETAG_PATH ), historynode_getattr( hn, HISTORYNODETAG_MODENAME ), NULL );

	}

	return (0);
}


DEFMMETHOD(Setup)
{
	ULONG rc;
	GETDATA;

	if ( (rc = DOSUPER) )
	{
		data->ehnode.ehn_Object = obj;
		data->ehnode.ehn_Class  = cl;
		data->ehnode.ehn_Events = IDCMP_RAWKEY;
		data->ehnode.ehn_Priority = 1;
		data->ehnode.ehn_Flags = MUI_EHF_GUIMODE;

		DoMethod( _win(obj), MUIM_Window_AddEventHandler, &data->ehnode);
	}

	return (rc);
}


DEFMMETHOD(Cleanup)
{
	GETDATA;
	DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehnode );

	return (DOSUPER);
}


DEFMMETHOD(HandleEvent)
{
	if ( msg->imsg )
	{
		GETDATA;

		if ( msg->imsg->Class == IDCMP_RAWKEY )
		{
			ULONG up=FALSE;
			switch ( msg->imsg->Code )
			{
				case	RAWKEY_TAB:
					{
						/* check if we are in string mode */

						if ( getv( data->pages, MUIA_Group_ActivePage ) != 0 )
						{
							set( data->pages, MUIA_Group_ActivePage, 0 );
							return (MUI_EventHandlerRC_Eat);
						}
					}
					break;
				case RAWKEY_NM_WHEEL_UP:
					up = TRUE;
				case RAWKEY_NM_WHEEL_DOWN:
					{
						if (_isinobject2(data->but_view_list, msg->imsg->MouseX, msg->imsg->MouseY))// && getv(data->but_view_icons, MUIA_ShowMe))
						{
							APTR viewobj = history_getattr(data->history, HISTORYTAG_VIEWOBJ);
							APTR currview = (APTR)getv(viewobj, MA_Viewgroup_CurrentView);
							LONG isa = getv(currview, MA_Icon_SizeAdjustment);

							if ( (up && (isa < 128)) || (!up && (isa - 10 > -(LONG)_conf(icon_minsize))) )
							{
								SetAttrs(currview,
									MUIA_Group_Forward, FALSE,
									MA_Icon_SizeAdjustment, up ? (isa + 10) : (isa - 10),
								TAG_DONE);
								return (MUI_EventHandlerRC_Eat);
							}
						}
					}
					break;
				default: /* VANILLAKEY */
					switch (keymap_vanilla(msg->imsg))
					{
						case	'/':
						{
							/* check if we are not in string mode */
					
							if ( getv( data->pages, MUIA_Group_ActivePage ) != 1 )
							{
								APTR string = (APTR)getv( data->location, MA_Navigation_StringObject );

								set( data->pages, MUIA_Group_ActivePage, 1 );
								set( _win( obj ), MUIA_Window_ActiveObject, string );
								return (MUI_EventHandlerRC_Eat);
							}
						}
						break;
					}
			}
		}
	}

	return (0);
}

/*
 * Method called when some element on toolbar changes path.
 */

DEFTMETHOD(Toolbar_NewPath)
{
	GETDATA;

	STRPTR p = (STRPTR)getv( data->clickpath, MA_Clickpath_Path );

	if ( p )
	{
		/*
		 * Update current history node with new path.
		 * Maybe we should instead add new path after
		 * Current history node?
		 */

		DoMethod( _win(obj), MM_Window_UpdateURI );

		/*
		 * Pass new URI to window for loading.
		 */

		{
			APTR hn;
			hn = history_getattr( data->history, HISTORYTAG_NODE );

			if ( hn )
			{
				historynode_setattrs( hn, HISTORYNODETAG_PATH, p, TAG_DONE);

				/* lock history for the time of loading new URI to not create new node */

				history_setattrs( data->history,
					HISTORYTAG_LOCKED, TRUE,
				TAG_DONE );

				DoMethod(_win(obj), MM_Window_LoadURI, historynode_getattr( hn, HISTORYNODETAG_PATH ) , historynode_getattr( hn, HISTORYNODETAG_MODENAME ), NULL );
			}
		}
	}

	return(0);
}


DEFTMETHOD(Toolbar_LocationAcknowledge)
{
	ULONG x;
	GETDATA;

	x = getv(data->str_location, MUIA_String_Contents);

	DoMethod(data->location, MM_Navigation_LoadHistory);
	DoMethod(data->location, MM_Navigation_InsertHistory, x);
	DoMethod(data->location, MM_Navigation_SaveHistory);
	
	DoMethod(_win(obj), MM_Window_LoadURI, x, NULL, NULL);
	
	set(data->pages, MUIA_Group_ActivePage, 0);

	return(0);
}


DEFTMETHOD(Toolbar_ChangeView)
{
	/* We will simply use internal command to change viewmode */

	TEXT cmd[64];
	GETDATA;
	struct viewnode *vn;
	ULONG viewmode = 0;
	ULONG view;
	ULONG vm_icons = FALSE, vm_list = FALSE;

	if ( data->but_view_icons ) vm_icons = (getv( data->grp_viewbuts, MUIA_Group_ActivePage ) == 0); //getv( data->but_view_list, MUIA_ShowMe );
	if ( data->but_view_list ) vm_list = (getv( data->grp_viewbuts, MUIA_Group_ActivePage ) == 1); //getv( data->but_view_icons, MUIA_ShowMe );
	PDB(("vm_icons = %ld, vm_list = %ld\n", vm_icons, vm_list));

	view = vm_icons ? 0 : (vm_list ? 1 : 0);
	vn = viewapi_findbyname( data->views[ view ] );

	/* see if we have viewmode cycle gadget */
	if ( data->cyc_viewmodes )
		viewmode = getv( data->cyc_viewmodes, MUIA_Cycle_Active );

	sprintf(cmd, "Viewmode \"%s %s\"", data->views[ view ] , (STRPTR)tags_nth_tagdata(AVIEW_Query_Viewmode_RexxName, (ULONG)"" ,vn->querytagarray, viewmode + 1 )  );
	PDB(("cmd = '%s'\n", cmd));
	
	execute_command( (APTR)history_getattr( data->history, HISTORYTAG_VIEWOBJ ), AC_INTERNAL, cmd, NULL);
	return 0;
}

/*
 * View or viewmode were changed. Have to update cycles
 */

DEFTMETHOD(Toolbar_ViewChanged)
{
	GETDATA;

	APTR viewobj = history_getattr( data->history, HISTORYTAG_VIEWOBJ );
	struct viewnode *vn = viewapi_findbyid( getv( viewobj, MA_Viewgroup_ViewIndex ) );

	ULONG i, num;

	/* cleanup old */

	if ( DoMethod( data->grp_viewchange, MUIM_Group_InitChange ) ) // bitRocky: check return value of MUIM_Group_InitChange!
	{
		if ( DoMethod( data->pages, MUIM_Group_InitChange ) ) // bitRocky: check return value of MUIM_Group_InitChange!
		{

			if ( data->grp_viewbuts )
			{
				DoMethod( viewobj, MUIM_KillNotifyObj, MA_Viewgroup_ViewChanged, data->but_view_icons );
				DoMethod( viewobj, MUIM_KillNotifyObj, MA_Viewgroup_ViewChanged, data->but_view_list );
				DoMethod( data->grp_viewchange, OM_REMMEMBER, data->grp_viewbuts );
				MUI_DisposeObject( data->grp_viewbuts );
				data->grp_viewbuts = data->but_view_icons = data->but_view_list = NULL;
			}
/*
			if ( data->but_view_icons ) // bitRocky
			{
				DoMethod( viewobj, MUIM_KillNotifyObj, MA_Viewgroup_ViewChanged, data->but_view_icons );

				DoMethod( data->grp_viewchange, OM_REMMEMBER, data->but_view_icons );
				MUI_DisposeObject( data->but_view_icons );
				data->but_view_icons = NULL;
			}

			if ( data->but_view_list ) // bitRocky
			{
				DoMethod( viewobj, MUIM_KillNotifyObj, MA_Viewgroup_ViewChanged, data->but_view_list );

				DoMethod( data->grp_viewchange, OM_REMMEMBER, data->but_view_list );
				MUI_DisposeObject( data->but_view_list );
				data->but_view_list = NULL;
			}
*/
			if ( data->cyc_viewmodes )
			{
				DoMethod( viewobj, MUIM_KillNotifyObj, MA_Viewgroup_ViewChanged, data->cyc_viewmodes );

				DoMethod( data->grp_viewchange, OM_REMMEMBER, data->cyc_viewmodes );
				MUI_DisposeObject( data->cyc_viewmodes );
				data->cyc_viewmodes = NULL;
			}

			/* build array of available views */

			{
				STRPTR s = (STRPTR)getv( viewobj, MA_View_MIME );
				struct viewnode *vn;

				i = num = 0;
				while ( ( vn = viewapi_findbyid( i + 1 ) ) )
				{
					/* view exists, check it */

					if ( s && viewapi_checkmime( i + 1, s ) )
					{
						data->viewsmap[ vn->id ] = num;
						data->views[ num ] = vn->name;
						data->viewlabels[ num++ ] = vn->label;
					}
					else
					{
						data->viewsmap[ vn->id ] = -1;
					}
					i++;
				}
			}

			data->views[ num ] = NULL;
			data->viewlabels[ num ] = NULL;

			if ( num > 1 )
			{
				// bitRocky: start
				data->grp_viewbuts = HGroup,
					MUIA_Group_PageMode, TRUE,
					MUIA_Group_ActivePage, data->viewsmap[ vn->id ], // 0=List, 1=Icon
				//data->but_view_list = MUICreateToggleImageButton( "4:PROGDIR:images/toolbar/viewmode_list.mbr",
				//data->but_view_list = MUICreateToggleButton( "\33I[4:PROGDIR:images/toolbar/viewmode_list.mbr]",
					//(data->viewsmap[ vn->id ]==1), MSG_TOOLBAR_CLASS_VIEWMODE_LIST_HELP );
					Child, data->but_view_list = TinyButton( "\33I[4:PROGDIR:images/toolbar/viewmode_list.mbr]" ),

				//data->but_view_icons = MUICreateToggleImageButton( "4:PROGDIR:images/toolbar/viewmode_icons.mbr",
				//data->but_view_icons = MUICreateToggleButton( "\33I[4:PROGDIR:images/toolbar/viewmode_icons.mbr]",
					//(data->viewsmap[ vn->id ]==0), MSG_TOOLBAR_CLASS_VIEWMODE_ICONS_HELP );
					Child, data->but_view_icons = TinyButton( "\33I[4:PROGDIR:images/toolbar/viewmode_icons.mbr]" ),

				End;

				if ( data->but_view_icons && data->but_view_list )
				{
					//set(data->but_view_icons, MUIA_ShowMe, (data->viewsmap[ vn->id ]==1));
					SetAttrs(data->but_view_icons, MUIA_ShortHelp, GSI(MSG_TOOLBAR_CLASS_VIEWMODE_ICONS_HELP), MUIA_CycleChain, 1, TAG_DONE );
					//set(data->but_view_list, MUIA_ShowMe, (data->viewsmap[ vn->id ]==0));
					SetAttrs(data->but_view_list, MUIA_ShortHelp, GSI(MSG_TOOLBAR_CLASS_VIEWMODE_LIST_HELP), MUIA_CycleChain, 1, TAG_DONE );

					DoMethod( data->grp_viewchange, OM_ADDMEMBER, data->grp_viewbuts );
					/*
					DoMethod( data->grp_viewchange, OM_ADDMEMBER, data->but_view_icons );
					DoMethod( data->grp_viewchange, OM_ADDMEMBER, data->but_view_list );
					*/
					
					/*
					DoMethod( data->but_view_icons, MUIM_Notify, MUIA_Selected, TRUE, data->but_view_list, 3, MUIM_NoNotifySet, MUIA_Selected, FALSE);
					DoMethod( data->but_view_icons, MUIM_Notify, MUIA_Selected, FALSE, data->but_view_list, 3, MUIM_Set, MUIA_Selected, TRUE);
					DoMethod( data->but_view_icons, MUIM_Notify, MUIA_Selected, TRUE, app, 4, MUIM_Application_PushMethod, obj, 1, MM_Toolbar_ChangeView );

					DoMethod( data->but_view_list, MUIM_Notify, MUIA_Selected, TRUE, data->but_view_icons, 3, MUIM_NoNotifySet, MUIA_Selected, FALSE);
					DoMethod( data->but_view_list, MUIM_Notify, MUIA_Selected, FALSE, data->but_view_icons, 3, MUIM_Set, MUIA_Selected, TRUE);
					DoMethod( data->but_view_list, MUIM_Notify, MUIA_Selected, TRUE, app, 4, MUIM_Application_PushMethod, obj, 1, MM_Toolbar_ChangeView );
					*/
					DoMethod( data->but_view_icons, MUIM_Notify, MUIA_Pressed, FALSE, data->grp_viewbuts, 3, MUIM_Set, MUIA_Group_ActivePage, 0 );
					DoMethod( data->but_view_icons, MUIM_Notify, MUIA_Pressed, FALSE, app, 4, MUIM_Application_PushMethod, obj, 1, MM_Toolbar_ChangeView );
					//DoMethod( data->but_view_icons, MUIM_Notify, MUIA_Pressed, FALSE, data->but_view_icons, 3, MUIM_Set, MUIA_ShowMe, FALSE);
					//DoMethod( data->but_view_icons, MUIM_Notify, MUIA_Pressed, FALSE, data->but_view_list, 3, MUIM_Set, MUIA_ShowMe, TRUE);

					DoMethod( data->but_view_list, MUIM_Notify, MUIA_Pressed, FALSE, data->grp_viewbuts, 3, MUIM_Set, MUIA_Group_ActivePage, 1 );
					DoMethod( data->but_view_list, MUIM_Notify, MUIA_Pressed, FALSE, app, 4, MUIM_Application_PushMethod, obj, 1, MM_Toolbar_ChangeView );
					//DoMethod( data->but_view_list, MUIM_Notify, MUIA_Pressed, FALSE, data->but_view_list, 3, MUIM_Set, MUIA_ShowMe, FALSE);
					//DoMethod( data->but_view_list, MUIM_Notify, MUIA_Pressed, FALSE, data->but_view_icons, 3, MUIM_Set, MUIA_ShowMe, TRUE);
				}
				// bitRocky: end
			}

			/* build array of available viewmodes */

			i = num = 0;
			FORTAG( vn->querytagarray )
			{
				case AVIEW_Query_Viewmode_Name:
					data->viewmodes[ num++ ] = (STRPTR)tag->ti_Data;
					PDB(("viewmodes[%ld] = '%s'\n", num, (STRPTR)tag->ti_Data));
			}
			NEXTTAG
			data->viewmodes[ num ] = NULL;

			if ( num > 1 ) /* no need for cycle if only one mode available */
			{
				data->cyc_viewmodes = CycleObject,
					MUIA_Font,          MUIV_Font_Tiny,
					MUIA_CycleChain,    TRUE,
					MUIA_Cycle_Active,  getv( viewobj, MA_Viewgroup_ViewModeIndex ),
					MUIA_Cycle_Entries, data->viewmodes,
				End;

				DoMethod( data->grp_viewchange, OM_ADDMEMBER, data->cyc_viewmodes );

				DoMethod( data->cyc_viewmodes, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
					app, 4, MUIM_Application_PushMethod, obj, 1, MM_Toolbar_ChangeView );
			}

			#if 0
			vn = viewapi_findbyid( getv( viewobj, MA_Viewgroup_ViewIndex ) );

			if(stricmp((STRPTR)getv(viewobj, MA_View_Scheme), "devices"))
			{
				set( data->pages, MUIA_ShowMe, TRUE);
			}
			else
			{
				set( data->pages, MUIA_ShowMe, FALSE);
			}
			#endif
			DoMethod( data->pages, MUIM_Group_ExitChange );
		}
		DoMethod( data->grp_viewchange, MUIM_Group_ExitChange );
	}

	return 0;
}

DEFTMETHOD(Clickpath_Parent)
{
	GETDATA;
	return DoMethod(data->clickpath, MM_Clickpath_Parent);
}

BEGINMTABLE
DECNEW
DECDISP
DECSET
DECGET
DECSMETHOD(Toolbar_HistoryAddURI)
DECSMETHOD(Toolbar_HistoryUpdateURI)
DECTMETHOD(Toolbar_HistoryNext)
DECTMETHOD(Toolbar_HistoryPrev)
DECSMETHOD(Toolbar_HistoryMove)
DECTMETHOD(Toolbar_ChangeView)
DECTMETHOD(Toolbar_ViewChanged)
DECTMETHOD(Toolbar_NewPath)
DECTMETHOD(Toolbar_LocationAcknowledge)
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECMMETHOD(HandleEvent)
DECTMETHOD(Clickpath_Parent)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, toolbarclass)
