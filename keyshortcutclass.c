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
 * $Id: keyshortcutclass.c,v 1.9 2025/02/28 10:29:05 bitrocky Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "mui_func.h"
#include "ambient_cat.h"
#include "keyshortcuts.h"
#include "action.h"
#include "methodstack.h"

/************************************************************************/

struct window_entry
{
	struct MinNode n;
	APTR obj;
};


struct Data {
	struct key_shortcut_t shortcut;
	ULONG edit_action;
	APTR action;

	APTR str_name;
	APTR ka_definition;
	APTR bt_action;
	APTR bt_remove;

	APTR prefsobj;

	struct MinList windows_list;
};

/************************************************************************/

static void doset( APTR obj UNUSED, struct Data *data, struct TagItem *tags )
{
	FORTAG( tags )
	{
		case MA_KeyShortcut_Shortcut:
			keyshortcut_copy( &data->shortcut, (struct key_shortcut_t *) tag->ti_Data );
			break;

		case MA_KeyShortcut_EditAction:
			data->edit_action = (ULONG) tag->ti_Data;
			break;

		case MA_Prefswin_Object:
			data->prefsobj = (APTR) tag->ti_Data;
			break;
	}
	NEXTTAG
}

/************************************************************************/

DEFNEW
{
	struct Data *data;
	APTR str_name, ka_definition, bt_action, bt_remove;
	TEXT buffer[128];

	if( ( obj = DoSuperNew( cl, obj,
		MUIA_Group_Horiz, TRUE,
		TAG_MORE, INITTAGS ) ) ) {

		data = INST_DATA( cl, obj );

		doset( obj, data, INITTAGS );


		/* XXX is the Init/Exit Change mess realy required? Why not
		** init change here and exit at the end?
		*/
		DoMethod( obj, MUIM_Group_InitChange );
		if( data->shortcut.flags & SHORTCUT_FLAG_BUILTIN )
		{
			str_name = MUI_NewObject( MUIC_Text,
									  MUIA_HorizWeight, 30, // bitRocky: looks better this way
									  MUIA_Text_PreParse, "\033r", 
									  TAG_DONE );
		} else {
			str_name = MUI_NewObject( MUIC_String,
									  MUIA_Frame        , MUIV_Frame_String,
									  MUIA_CycleChain   , 1,
									  MUIA_String_MaxLen, 128,
									  TAG_DONE );
		}

		if( str_name )
		{
			//DoMethod( obj, MUIM_Group_InitChange );
			DoMethod( obj, OM_ADDMEMBER, str_name );
			//DoMethod( obj, MUIM_Group_ExitChange );
		}

		if( ( ka_definition =  MUI_NewObject( "Keyadjust.mui",
									    MUIA_HorizWeight, 70, // bitRocky: looks better this way
										MUIA_Keyadjust_AllowMouseEvents, TRUE,
									    MUIA_Keyadjust_AllowMultipleKeys, FALSE,
	                                    MUIA_CycleChain, 1,
										TAG_DONE) ) )
		{
			//DoMethod( obj, MUIM_Group_InitChange );
			DoMethod( obj, OM_ADDMEMBER, ka_definition );
			//DoMethod( obj, MUIM_Group_ExitChange );
		}

		if( ( bt_action = MUICreateButton(MSG_KEYSHORTCUTCLASS_ACTION, NULL ) ) )
		{
			//DoMethod( obj, MUIM_Group_InitChange );
			DoMethod( obj, OM_ADDMEMBER, bt_action );
			//DoMethod( obj, MUIM_Group_ExitChange );
		}

		if( ( bt_remove = MUICreateButton(MSG_KEYSHORTCUTCLASS_REMOVE, NULL ) ) )
		{
			//DoMethod( obj, MUIM_Group_InitChange );
			DoMethod( obj, OM_ADDMEMBER, bt_remove );
			//DoMethod( obj, MUIM_Group_ExitChange );
		}
		DoMethod( obj, MUIM_Group_ExitChange );

		data->str_name = str_name;
		data->ka_definition = ka_definition;
		data->bt_action = bt_action;
		data->action = NULL;
		data->bt_remove = bt_remove;
		NEWLIST( &data->windows_list );

		if( data->shortcut.flags & SHORTCUT_FLAG_BUILTIN )
		{
			snprintf( buffer, sizeof( buffer ), GSI(MSG_KEYSHORTCUTCLASS_LABEL_FORMAT), ( data->shortcut.msgid ? GSI(data->shortcut.msgid) : data->shortcut.name ) );

			set( data->str_name, MUIA_Text_Contents, buffer );
		} else {
			set( data->str_name, MUIA_String_Contents, data->shortcut.name );
		}

		keyshortcut_sequence_to_string( &data->shortcut.sequence, buffer, sizeof( buffer ) );

		set( data->ka_definition, MUIA_String_Contents, buffer );

		set( data->bt_action, MUIA_ShowMe, data->edit_action );
		set( data->bt_remove, MUIA_ShowMe, data->edit_action );
		set( data->bt_action, MUIA_Weight, 10 );
		set( data->bt_remove, MUIA_Weight, 10 );

		DoMethod(data->bt_action, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_KeyShortcut_EditAction );

		DoMethod(data->bt_remove, MUIM_Notify, MUIA_Pressed, FALSE, obj, 2, MM_Prefswin_Keyboard_RemoveShortcut, obj );
	}
	return( (ULONG) obj );
}

/************************************************************************/

DEFDISP
{
	GETDATA;
	struct window_entry *n, *nextn;

	ITERATELISTSAFE( n, nextn, &data->windows_list )
	{
		if( n->obj )
		{
			DoMethod( n->obj, MUIM_KillNotify, MUIA_Window_Open );
			DoMethod( n->obj, MM_ActioneditorWin_Close );
		}
		FreeVecTaskPooled( n );
	}

	if( data->shortcut.name )
	{
		FreeVecTaskPooled( data->shortcut.name );
		data->shortcut.name = NULL;
	}

	if( data->shortcut.action.command )
	{
		FreeVecTaskPooled( data->shortcut.action.command );
		data->shortcut.action.command = NULL;
	}

	if( data->action )
	{
		actionnode_delete( data->action );
	}

	return( DOSUPER );
}

/************************************************************************/

DEFGET
{
	GETDATA;

	switch( msg->opg_AttrID )
	{
		case MA_KeyShortcut_Shortcut:
			DoMethod( obj, MM_KeyShortcut_Update );
			*msg->opg_Storage = (ULONG) &data->shortcut;
			return( TRUE );
	}
	return( DOSUPER );
}

/************************************************************************/

DEFSET
{
	GETDATA;

	doset( obj, data, INITTAGS );

	return( DOSUPER );
}

/************************************************************************/

/* retrieve current shortcut configuration */
DEFTMETHOD(KeyShortcut_Update)
{
	GETDATA;
	STRPTR definition;
	STRPTR name;
	ULONG len;

	if( data->shortcut.flags & SHORTCUT_FLAG_BUILTIN )
	{
		/* no change */
	} else {
		name = (STRPTR) getv( data->str_name, MUIA_String_Contents );

		if( name )
		{
			if( data->shortcut.name )
			{
				FreeVecTaskPooled( data->shortcut.name );
			}

			len = strlen( name ) + 1;
			data->shortcut.name = (STRPTR) AllocVecTaskPooled( len );

			if( data->shortcut.name )
			{
				strcpy( data->shortcut.name, name );
			}
		}
	}

	definition = (STRPTR) getv( data->ka_definition, MUIA_Keyadjust_Key );

	if( definition )
	{
		keyshortcut_string_to_sequence( definition, &data->shortcut.sequence );
	}
	return( 0 );
}

/************************************************************************/

DEFTMETHOD(KeyShortcut_EditAction)
{
	GETDATA;
	APTR o;
	struct window_entry * n;

	n = AllocVecTaskPooled( sizeof( *n ) );

	if( n )
	{
		n->obj = NULL;

		data->action = actionnode_create();

		if ( data->action )
		{
			actionnode_setattrs( data->action,
								 ACTIONNODETAG_NAME, (STRPTR) getv(data->str_name, MUIA_String_Contents ), /* useless anyway */
								 ACTIONNODETAG_FLAGS, data->shortcut.action.flags,
			                     TAG_DONE );

			if( data->shortcut.action.command )
			{
				actionnode_addcommand( data->action, data->shortcut.action.type, data->shortcut.action.command );
			}

			if( ( o = NewObject(getactioneditorwinclass(), NULL,
						MA_ActioneditorWin_ActionNode, data->action,
						MA_Actioneditor_MimeAction, FALSE,
						TAG_DONE ) ) ) {
				DoMethod( o, MUIM_Notify, MUIA_Window_Open, FALSE, obj, 2, MM_KeyShortcut_EditAction_Ack, o );
				DoMethod( o, MM_ActioneditorWin_Open );

                n->obj = o;
			}
		}
		ADDTAIL( &data->windows_list, n );
	}
	return( 0 );
}

/************************************************************************/

DEFSMETHOD(KeyShortcut_EditAction_Ack)
{
	GETDATA;
	APTR action;
	struct window_entry * n;

	action = (APTR) getv( msg->actionobj, MA_ActioneditorWin_ActionNode );

	if( action )
	{
		APTR commandlist;

		if ( ( commandlist = actionnode_getattr( action, ACTIONNODETAG_COMMAND_LIST ) ) )
		{
			APTR cn;
			ITERATELIST( cn, commandlist ) /* we get first command and ignore following ones */
			{
				STRPTR command = (STRPTR) commandnode_getattr( cn, COMMANDNODETAG_COMMAND );

				if( ( data->shortcut.action.command = AllocVecTaskPooled( strlen( command ) + 1 ) ) )
				{
					strcpy( data->shortcut.action.command, command );
				}

				data->shortcut.action.type = (ULONG) commandnode_getattr( cn, COMMANDNODETAG_TYPE );

				break;
			}
		}
		data->shortcut.action.flags = (ULONG) actionnode_getattr( action, ACTIONNODETAG_FLAGS );
	}

	/* remove opened window */
	ITERATELIST( n, &data->windows_list )
	{
		if( n->obj == msg->actionobj )
		{
			REMOVE( n );
			FreeVecTaskPooled( n );
			break;
		}
	}

	DoMethod( msg->actionobj, MM_ActioneditorWin_Close );

	if( data->action )
	{
		actionnode_delete( data->action );
		data->action = NULL;
	}
	return( 0 );
}

/************************************************************************/

DEFSMETHOD(Prefswin_Keyboard_RemoveShortcut)
{
	GETDATA;

	DoMethod( data->bt_remove, MUIM_KillNotify, MUIA_Pressed );
	DoMethod( data->prefsobj, MM_Prefswin_Keyboard_RemoveShortcut, msg->object );

	return( 0 );
}

/************************************************************************/

BEGINMTABLE
DECNEW
DECDISP
DECGET
DECSET
DECTMETHOD(KeyShortcut_Update)
DECTMETHOD(KeyShortcut_EditAction)
DECSMETHOD(KeyShortcut_EditAction_Ack)
DECSMETHOD(Prefswin_Keyboard_RemoveShortcut)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, keyshortcutclass)
