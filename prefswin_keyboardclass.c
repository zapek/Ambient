/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber
 * Copyright 2005-2015 Ambient Open Source Team
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
 * $Id: prefswin_keyboardclass.c,v 1.7 2016/01/29 19:05:20 itix Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "prefswin.h"
#include "prefsclone.h"
#include "time_func.h"
#include "keyshortcuts.h"
#include "command.h"

/************************************************************************/

struct Data {
	APTR group_builtin_shortcuts;
	APTR group_custom_shortcuts;
	APTR spacer; /* Spacer in custom shortcuts */
};

/************************************************************************/

static void AddKeyShortcuts(APTR obj, APTR built_in_keys, APTR custom_keys)
{
	struct key_shortcut_node *n = keyshortcuts_next(NULL);
	APTR cl = getkeyshortcutclass();

	while (n)
	{
		if (n->shortcut.flags & SHORTCUT_FLAG_ENABLED)
		{
			size_t built_in = n->shortcut.flags & SHORTCUT_FLAG_BUILTIN;

			APTR o = NewObject(cl, NULL,
							MA_KeyShortcut_Shortcut, &n->shortcut,
							MA_KeyShortcut_EditAction, built_in ? FALSE : TRUE,
							MA_Prefswin_Object, obj,
							TAG_DONE);

			if (o)
			{
				DoMethod(built_in ? built_in_keys : custom_keys, OM_ADDMEMBER, (size_t)o);
			}
		}

		n = keyshortcuts_next(n);
	}
}

DEFNEW
{
	APTR group_builtin_shortcuts;
	APTR group_custom_shortcuts;
	APTR o;
	APTR bt_addshortcut;
	
	if( ( obj = DoSuperNew(cl, obj,
									Child,
									ScrollgroupObject,
										GroupFrameT( GSI(MSG_PREFSWIN_KEYBOARDCLASS_BUILTINGROUP) ),
										//MUIA_Weight, 60,
										MUIA_Scrollgroup_FreeVert, TRUE,
										MUIA_Scrollgroup_AutoBars, TRUE,
										MUIA_Scrollgroup_Contents,
										group_builtin_shortcuts	=  VirtgroupObject, End,
									End,

									Child,
									ScrollgroupObject,
										GroupFrameT( GSI(MSG_PREFSWIN_KEYBOARDCLASS_CUSTOMGROUP) ),
										//MUIA_Weight, 40,
										MUIA_Scrollgroup_FreeVert, TRUE,
										MUIA_Scrollgroup_AutoBars, TRUE,
										MUIA_Scrollgroup_Contents,
										group_custom_shortcuts =  VirtgroupObject, End,
									End,

									Child, HGroup,
										Child, HSpace(0),
										Child, bt_addshortcut = MUICreateButton( MSG_PREFSWIN_KEYBOARDCLASS_ADDAHOTKEY, NULL ),
										Child, HSpace(0),
										End,
								TAG_DONE ) ) )
	{
		struct Data *data = INST_DATA( cl, obj );

		data->group_builtin_shortcuts = group_builtin_shortcuts;
		data->group_custom_shortcuts  = group_custom_shortcuts;

		/* Add shortcuts */
		DoMethod( obj, MUIM_Group_InitChange );

		AddKeyShortcuts(obj, group_builtin_shortcuts, group_custom_shortcuts);

		if( ( o = RectangleObject, End ) )
		{
			data->spacer = o;
			DoMethod( group_custom_shortcuts, OM_ADDMEMBER, o );
		}

		DoMethod( obj, MUIM_Group_ExitChange );

		set( bt_addshortcut, MUIA_Weight, 10 );

		DoMethod( bt_addshortcut, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_Prefswin_Keyboard_AddShortcut );

	}
	return ((ULONG)obj);
}

/************************************************************************/

DEFTMETHOD(Prefswin_Store)
{
	GETDATA;
	struct key_shortcut_t * shortcut;
	TEXT buffer[128];
	APTR pl, pi;
	ULONG i;

	prefspool_item_remove( cloneprefspool, NULL, DSI_LISTPOOL_KEYSHORTCUT );

	if( !( pl = prefspool_item_get( cloneprefspool, NULL, DSI_LISTPOOL_KEYSHORTCUT, NULL, NULL ) ) ) {
		pl = prefspool_item_add( cloneprefspool, NULL, DSI_LISTPOOL_KEYSHORTCUT, NULL, NULL );
	}

	if( pl )
	{
		i = 0;
		/*  builtin commands */
		FORCHILD( data->group_builtin_shortcuts, MUIA_Group_ChildList )
		{
			APTR ptr = (APTR) getv( child, MA_KeyShortcut_Shortcut );

			shortcut = (struct key_shortcut_t *) ptr;

			if( shortcut )
			{

				if( shortcut->name && shortcut->action.command )
				{
					keyshortcut_sequence_to_string( &shortcut->sequence, buffer, sizeof( buffer ) );

					if ( !( pi = prefspool_item_get( cloneprefspool, pl, i | DSF_LISTPOOL, NULL, NULL ) ) ) {
						pi = prefspool_item_add( cloneprefspool, pl, i | DSF_LISTPOOL, NULL, NULL );
					}

					if( pi )
					{
						setprefsstr_lp(  cloneprefspool, pi, DSI_LISTPOOL_KEYSHORTCUT_NAME      , shortcut->name );
						setprefslong_lp( cloneprefspool, pi, DSI_LISTPOOL_KEYSHORTCUT_MSGID     , shortcut->msgid );
						setprefsstr_lp(  cloneprefspool, pi, DSI_LISTPOOL_KEYSHORTCUT_DEFINITION, buffer );
						setprefslong_lp( cloneprefspool, pi, DSI_LISTPOOL_KEYSHORTCUT_ID        , shortcut->id );
						setprefslong_lp( cloneprefspool, pi, DSI_LISTPOOL_KEYSHORTCUT_FLAGS     , shortcut->flags );
						setprefsstr_lp(  cloneprefspool, pi, DSI_LISTPOOL_KEYSHORTCUT_CMDSTRING , shortcut->action.command );
						setprefslong_lp( cloneprefspool, pi, DSI_LISTPOOL_KEYSHORTCUT_CMDTYPE   , shortcut->action.type );
						setprefslong_lp( cloneprefspool, pi, DSI_LISTPOOL_KEYSHORTCUT_CMDFLAGS  , shortcut->action.flags );
					}
					i++;
				}
			}
		}
		NEXTCHILD

		/* custom commands */

		FORCHILD( data->group_custom_shortcuts, MUIA_Group_ChildList )
		{
			APTR ptr = (APTR) getv( child, MA_KeyShortcut_Shortcut );

			shortcut = (struct key_shortcut_t *) ptr;

			if( shortcut && stricmp( OCLASS(child)->cl_ID, "Rectangle.mui" ) )  /* why does rectangle also return a pointer ? */
			{
				if( shortcut->name && shortcut->action.command )
				{
					keyshortcut_sequence_to_string( &shortcut->sequence, buffer, sizeof( buffer ) );

					if( !( pi = prefspool_item_get( cloneprefspool, pl, i | DSF_LISTPOOL, NULL, NULL ) ) ) {
						pi = prefspool_item_add( cloneprefspool, pl, i | DSF_LISTPOOL, NULL, NULL );
					}

					if( pi )
					{
						setprefsstr_lp(  cloneprefspool, pi, DSI_LISTPOOL_KEYSHORTCUT_NAME      , shortcut->name );
						setprefslong_lp( cloneprefspool, pi, DSI_LISTPOOL_KEYSHORTCUT_MSGID     , shortcut->msgid );
						setprefsstr_lp(  cloneprefspool, pi, DSI_LISTPOOL_KEYSHORTCUT_DEFINITION, buffer );
						setprefslong_lp( cloneprefspool, pi, DSI_LISTPOOL_KEYSHORTCUT_ID        , shortcut->id );
						setprefslong_lp( cloneprefspool, pi, DSI_LISTPOOL_KEYSHORTCUT_FLAGS     , shortcut->flags );
						setprefsstr_lp(  cloneprefspool, pi, DSI_LISTPOOL_KEYSHORTCUT_CMDSTRING , shortcut->action.command );
						setprefslong_lp( cloneprefspool, pi, DSI_LISTPOOL_KEYSHORTCUT_CMDTYPE   , shortcut->action.type );
						setprefslong_lp( cloneprefspool, pi, DSI_LISTPOOL_KEYSHORTCUT_CMDFLAGS  , shortcut->action.flags );
					}
					i++;
				}
			}
		}
		NEXTCHILD
	}

	setprefslong( DSI_LISTPOOL_KEYSHORTCUT_CHANGED, timed() );

	return( 0 );
}

/************************************************************************/

DEFDISPOSE
{
	DoMethod( obj, MM_Prefswin_Store );

	return( DOSUPER );
}

/************************************************************************/

DEFTMETHOD(Prefswin_Keyboard_AddShortcut)
{
	GETDATA;
	struct key_shortcut_t * shortcut;
	APTR o;

	shortcut = keyshortcut_create( GSI(MSG_PREFSWIN_KEYBOARDCLASS_NEWSHORTCUT) );

	if( shortcut )
	{
		DoMethod( data->group_custom_shortcuts, MUIM_Group_InitChange );
		{
			DoMethod( data->group_custom_shortcuts, OM_REMMEMBER, data->spacer );

			if( ( o = NewObject( getkeyshortcutclass(), NULL,
							MA_KeyShortcut_Shortcut, shortcut,
							MA_KeyShortcut_EditAction, TRUE,
							MA_Prefswin_Object, obj,
							TAG_DONE ) ) ) {

				DoMethod( data->group_custom_shortcuts, OM_ADDMEMBER, o );
			}

			DoMethod( data->group_custom_shortcuts, OM_ADDMEMBER, data->spacer );
			DoMethod( data->group_custom_shortcuts, MUIM_Group_ExitChange );
		}

		DoMethod( _parent( data->group_custom_shortcuts ), MUIM_Group_InitChange );
		DoMethod( _parent( data->group_custom_shortcuts ), MUIM_Group_ExitChange );

		keyshortcut_delete( shortcut );
	}
	return( 0 );
}

/************************************************************************/

DEFSMETHOD(Prefswin_Keyboard_RemoveShortcut)
{
	GETDATA;

	if( DoMethod( data->group_custom_shortcuts, MUIM_Group_InitChange ) )
	{
		DoMethod( data->group_custom_shortcuts, OM_REMMEMBER, msg->object );
		DoMethod( _app(obj), MUIM_Application_PushMethod, app, 2, MM_Application_DisposeObject, msg->object );
		DoMethod( data->group_custom_shortcuts, MUIM_Group_ExitChange );
	}

	DoMethod( _parent( data->group_custom_shortcuts ), MUIM_Group_InitChange );
	DoMethod( _parent( data->group_custom_shortcuts ), MUIM_Group_ExitChange );

	return( 0 );
}

/************************************************************************/

BEGINMTABLE
DECNEW
DECTMETHOD(Prefswin_Store)
DECTMETHOD(Prefswin_Keyboard_AddShortcut)
DECSMETHOD(Prefswin_Keyboard_RemoveShortcut)
DECDISPOSE
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, prefswin_keyboardclass)
