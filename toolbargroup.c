/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
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
 * $Id: toolbargroup.c,v 1.23 2025/07/23 23:54:26 geit Exp $
 */

#include "ambient.h"


/* public */
#include <cybergraphx/cybergraphics.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>


/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "toolbarclass.h"
#include "command.h"
#include "strings.h"
#include "name.h"
#include "rexx.h"
#include "prefs.h"
#include "legacy.h"
#include "viewapi.h"


struct Data {
	ULONG droppable;
	LONG droppos;
	LONG olddroppos;
	APTR predobj;
	STRPTR definition;
	ULONG definition_valid;
	ULONG source;
	APTR cm;
	ULONG aftersetup;
	APTR vo;
	APTR extraspacer;
};

enum {
	LABEL_OFF = 0,
	LABEL_LEFT,
	LABEL_RIGHT,
	LABEL_BOTTOM,
};

/*
 * Templates for buttons. Dunno if it's best place to keep then but
 * it's easiest one.
 */

typedef APTR ( *getclassfunc ) ( void );

struct buttonclass_definition
{
	ULONG	id;
	getclassfunc getclassfunc; /* Pointer to function to get class. XXX: It's safer than using names as will fail at compilation time  */
	ULONG	argnum;
	CONST_STRPTR	template;
};

enum {
	BTN_ACTION = 1,
	BTN_SPACER,
	BTN_HISTORY,
	BTN_VIEWSWITCH,
	BTN_BOOKMARKS,
};

static const struct buttonclass_definition avail_buttons[] = {
#warning the last argument, "VIEWS/A", causes a parsing error
	{ BTN_ACTION,     gettoolbutton_actionclass,     5, "TYPE/A,COMMAND/A,IMAGE/A,FLAGS/A,VIEWS/A" },
	{ BTN_SPACER,     gettoolbutton_spacerclass,     1, "TYPE/A" },
	{ BTN_HISTORY,    gettoolbutton_historyclass,    5, "TYPE/A,COMMAND/A,IMAGE/A,FLAGS/A,VIEWS/A" },
	{ BTN_VIEWSWITCH, gettoolbutton_viewswitchclass, 0, "" },
	{ BTN_BOOKMARKS,  gettoolbutton_bookmarksclass,  0, "" },
	{ 0, 0, 0, 0}
};


/*
 * All available buttons. not perfectly readable. do something with it!.
 */

struct button_definition
{
	CONST_STRPTR name;
	ULONG buttonclass_id;
	CONST_STRPTR args;
	STRPTR localized_name;
	STRPTR localized_help;
};

/*
 * FLAGS (4'th param) are defined in mimetype.h
 * VIEWS (5'th param) are defined in viewapi.h
 */
enum
{
	TOOLBAR_ENUM_DEVICE_LIST,
	TOOLBAR_ENUM_COPY,
	TOOLBAR_ENUM_COPY_AS,
	TOOLBAR_ENUM_MOVE,
	TOOLBAR_ENUM_MOVE_AS,
	TOOLBAR_ENUM_DELETE,
	TOOLBAR_ENUM_TRASH,
	TOOLBAR_ENUM_COPY_TO_CLIPBOARD,
	TOOLBAR_ENUM_MOVE_TO_CLIPBOARD,
	TOOLBAR_ENUM_PASTE_FROM_CLIPBOARD,
	TOOLBAR_ENUM_PASTE_FROM_CLIPBOARD_AS,
	TOOLBAR_ENUM_SELECT,
	TOOLBAR_ENUM_ICONINFO,
	TOOLBAR_ENUM_FORMAT,
	TOOLBAR_ENUM_MAKEDIR,
	TOOLBAR_ENUM_MAKELINK,
	TOOLBAR_ENUM_RENAME,
	TOOLBAR_ENUM_FIND,
	TOOLBAR_ENUM_PARENT,
	TOOLBAR_ENUM_BACK,
	TOOLBAR_ENUM_FORWARD,
	TOOLBAR_ENUM_SWITCH_VIEW,
	TOOLBAR_ENUM_BOOKMARKS,
	TOOLBAR_ENUM_SPACE,
	TOOLBAR_ENUM_FLEXIBLE_SPACE,
	TOOLBAR_ENUM_SEPARATOR
};

static struct button_definition definition_all[] = {
	{ "Devices list",            BTN_ACTION,    "Internal \"LoadURI devices:// VIEWID %Si\" toolbar/deviceslist.png 0 -1" , NULL, NULL },
	{ "Copy",                    BTN_ACTION,    "Internal \"Copy FROM %sp TO %D\" toolbar/filecopy.png 8 128"  , NULL, NULL},
	{ "Copy as",                 BTN_ACTION,    "Internal \"Copy FROM %sp TO %D AS\" toolbar/filecopyas.png 8 128"  , NULL, NULL},
	{ "Move",                    BTN_ACTION,    "Internal \"Move FROM %sp TO %D\" toolbar/filemove.png 8 128"  , NULL, NULL},
	{ "Move as",                 BTN_ACTION,    "Internal \"Move FROM %sp TO %D AS\" toolbar/filemoveas.png 8 128"  , NULL, NULL},
	{ "Delete",                  BTN_ACTION,    "Internal \"Delete %sp\" toolbar/delete.png 8 128"  , NULL, NULL},
	{ "Trash",                   BTN_ACTION,    "Internal \"Trash %sp\" toolbar/trash.png 8 128"  , NULL, NULL},
	{ "Copy to clipboard",       BTN_ACTION,    "Internal \"ClipboardCopy %sp\" toolbar/clipcopy.png 8 -1"  , NULL, NULL},
	{ "Move to clipboard",       BTN_ACTION,    "Internal \"ClipboardCut %sp\" toolbar/clipmove.png 8 -1"  , NULL, NULL},
	{ "Paste from clipboard",    BTN_ACTION,    "Internal \"ClipboardPaste VIEWID %Si\" toolbar/clippaste.png 0 128"  , NULL, NULL},
	{ "Paste from clipboard as", BTN_ACTION,    "Internal \"ClipboardPaste VIEWID %Si AS\" toolbar/clippasteas.png 0 128"  , NULL, NULL},
	{ "Select",                  BTN_ACTION,    "Internal \"Select VIEWID %Si PATTERN\" toolbar/select.png 0 128"  , NULL, NULL},
	{ "IconInfo",                BTN_ACTION,    "Internal \"Iconinfo %sp\" toolbar/iconinfo.png 0 384"  , NULL, NULL},
	{ "Format",                  BTN_ACTION,    "Internal \"Format %sp\" toolbar/format.png 0 256"  , NULL, NULL},
	{ "Makedir",                 BTN_ACTION,    "Internal \"Makedir %S\" toolbar/makedir.png 0 128"  , NULL, NULL},
	{ "Makelink",                BTN_ACTION,    "Internal \"Makelink %S\" toolbar/makelink.png 0 128"  , NULL, NULL},
	{ "Rename",                  BTN_ACTION,    "Internal \"Rename %sp\" toolbar/rename.png 8 384"  , NULL, NULL},
	{ "Find",                    BTN_ACTION,    "Internal \"Find %S\" toolbar/find.png 0 -1"  , NULL, NULL},
	{ "Parent",                  BTN_HISTORY,   "Internal \"Parent %Si\" toolbar/parent.png 0 -1"  , NULL, NULL},
	{ "Back",                    BTN_HISTORY,   "Internal \"HistoryPrev %Si\" toolbar/historyprev.png 0 -1"  , NULL, NULL},
	{ "Forward",                 BTN_HISTORY,   "Internal \"HistoryNext %Si\" toolbar/historynext.png 0 -1"  , NULL, NULL},
	{ "Switch view",             BTN_VIEWSWITCH,""  , NULL, NULL},
	{ "Bookmarks",               BTN_BOOKMARKS, ""  , NULL, NULL},
	{ "Space",                   BTN_SPACER,    "1"  , NULL, NULL},
	{ "Flexible space",          BTN_SPACER,    "0"  , NULL, NULL},
	{ "Separator",               BTN_SPACER,    "2"  , NULL, NULL},
	{ 0, 0, 0, 0, 0 }
};

static const struct button_definition definition_default[] = {
	{ 0, 0, 0, 0, 0 }
};

static STRPTR definition_serialize( struct button_definition *bd )
{
	String *def	= string_new_len( NULL, 256 );

	if ( bd && def )
	{
		while ( bd->name )
		{
			//DB(("Add:%s %d %s\n", bd->name, bd->buttonclass_id, bd->args ));
			string_append_printf( def, "%s\1%d\1%s", bd->name, bd->buttonclass_id, bd->args );
			bd++;
			if ( bd->name )
				string_append( def, "\1" );	/* there will be more */
		}

		//DB(("Definition string:(%d)<%s>\n",def->len, def->str));

		return string_free( def, TRUE );
	}

	return NULL;
}

static struct button_definition *definition_deserialize( CONST_STRPTR def )
{
	struct button_definition *bd;

	ULONG items = 0;
	ULONG i;

	/* count number of items */

	for(i=0; def[ i ]; i++)
	{
		if ( def[ i ] == 1 )
			items++;
	}

	items++;	/* last one is trailed by 0 */

	if ( items % 3 )
	{
		//DB(("Definition string damaged (substrings number (%d) not dividable by 3)\n", items ));
		return NULL;
	}

	items /= 3;

	bd = malloc( ( items + 1 ) * sizeof( struct button_definition ) );

	if ( bd )
	{
		ULONG n = 0;

		while (def && *def != 0 && n < items)
		{
			TEXT name_str[ 32 ];
			TEXT type_str[ 4 ];
			TEXT args[ 256 ];
			CONST_STRPTR eol, neol;

			/* we know there is right number of \1s so we don't have to check */

			eol = def;
			neol = strchr( eol, 1 );
			stccpy( name_str, eol, min(neol + 1 - eol, sizeof(name_str)) );
			eol = neol + 1;

			neol = strchr( eol, 1 );
			stccpy( type_str, eol, min(neol + 1 - eol, sizeof(type_str)) );
			eol = neol + 1;

			neol = strchr( eol, 1 );
			if ( !neol )
				neol = eol + strlen( eol );

			stccpy( args, eol, min(neol + 1 - eol, sizeof(args)) );

			{
				/* we have name, type and params strings */

				ULONG type = atoi( type_str );

				bd[ n ].name = name_build( name_str );
				bd[ n ].buttonclass_id = type;
				bd[ n ].args = name_build( args );

				n++;
			}

			def = neol + 1;

		}

		bd[ n ].name = NULL;
		bd[ n ].buttonclass_id = 0;
		bd[ n ].args = NULL;

	}

	return bd;
}

static void definition_delete( struct button_definition *bd )
{
	if ( bd )
	{
		ULONG i = 0;

		while( bd[ i ].name )
		{
			name_delete( (STRPTR)bd[ i ].name );
			name_delete( (STRPTR)bd[ i ].args );
			i++;
		}

		free( bd );
	}
}

static APTR create_button_final( APTR button, ULONG label_pos )
{
	STRPTR label     = (STRPTR)getv( button, MA_Toolbutton_Label );
	ULONG  inputmode = getv( button, MUIA_InputMode );
	APTR   text      = NULL;
	LONG   deffspec  = _conf(toolbar_framespec)[0] != '!' ? FALSE : TRUE;
	LONG   defispec  = _conf(toolbar_imagespec)[0] != '!' ? FALSE : TRUE;

	if ( (label == NULL) || (strlen(label) == 0) )
	{
		return button;
	}

	if ( label_pos != LABEL_OFF )
	{
		text = TextObject,
				InnerSpacing(2,0),
				MUIA_Text_Contents, label,
				MUIA_Font,          MUIV_Font_Tiny,
				MUIA_Text_PreParse, "\033c",
				End;
	}

	set(button, MUIA_InputMode,  MUIV_InputMode_None);
	set(button, MUIA_Frame,      MUIV_Frame_None);

	switch( label_pos )
	{
		case LABEL_OFF:
			return VGroup,
				MUIA_CycleChain, TRUE,
				MUIA_Frame,      deffspec ? MUIV_Frame_Button : (ULONG)_conf(toolbar_framespec),
				MUIA_Background, defispec ? MUII_ButtonBack   : (ULONG)_conf(toolbar_imagespec),
				MUIA_InputMode,  inputmode,
				Child, button,
				End;

		case LABEL_BOTTOM:
			return VGroup,
				MUIA_Group_Spacing, 0,
				MUIA_Frame,      deffspec ? MUIV_Frame_Button : (ULONG)_conf(toolbar_framespec),
				MUIA_Background, defispec ? MUII_ButtonBack   : (ULONG)_conf(toolbar_imagespec),
				MUIA_InputMode,  inputmode,
				MUIA_CycleChain, TRUE,
				Child, button,
				Child, text,
				End;

		#if 0 /* not used yet and have to be fixed */
		case LABEL_LEFT:
			return HGroup,
				MUIA_Frame, MUIV_Frame_Button,
				Child, text,
				Child, button,
				End;
		#endif

		#if 0 /* not used yet */
		case LABEL_RIGHT:
			return HGroup,
				MUIA_Frame, MUIV_Frame_Button,
				Child, button,
				Child, text,
				End;
		#endif
	};

	return button;
}

/*
 * Setup visibility for a button, based on the view it's attached to.
 */

static void button_set_visible( Object *obj, struct Data *data, APTR button )
{
	if ( data->vo )
	{
		LONG vind = getv( data->vo, MA_Viewgroup_ViewIndex );
		struct viewnode *vn = viewapi_findbyid( vind );
		
		if ( vn )
		{
			ULONG vflags = viewapi_getflags( vn );
			ULONG bflags;

			if (vflags & VF_TOOLBAR)
				if (_parent(data->extraspacer) != NULL)
					DoMethod(obj, MUIM_Group_Remove, data->extraspacer);
		
			if (!(vflags & VF_TOOLBAR))
				if (_parent(data->extraspacer) == NULL)
					DoMethod(obj, MUIM_Group_AddHead, data->extraspacer);			
			
			if ( get( button, MA_Toolbutton_Viewflags, &bflags ) )
			{
				ULONG isvisible = getv( button, MUIA_ShowMe );
				/*APTR button_class =*/ OCLASS( button );
				if (!(vflags & VF_TOOLBAR))
					set(button, MUIA_ShowMe, FALSE);
				else
				{
				
					if ( !isvisible && ( vflags & bflags ) )
					{
						set( button, MUIA_ShowMe, TRUE );
					}
					else if ( isvisible && !( vflags & bflags ) )
					{
						set( button, MUIA_ShowMe, FALSE );
					}
				}
			}
		}
	}
}

DEFNEW
{
	/*
	 * For source group we put 4 objects in a row (separated by 3 spacers).
	 */

	ULONG source    = GetTagData(MA_Toolbargroup_Source,    FALSE, INITTAGS);
	ULONG samplebar = GetTagData(MA_Toolbargroup_Draggable, FALSE, INITTAGS);

	obj = DoSuperNew(cl, obj,
		MUIA_Group_Horiz, TRUE,
		MUIA_InnerBottom, 1,               /* these 4 tags get overidden for Views */
		MUIA_InnerLeft,   source ? 1 : 2,
		MUIA_InnerTop,    1,
		MUIA_InnerRight,  1,
		source ? TAG_IGNORE : MUIA_Group_HorizSpacing, samplebar ? 1 : 0,  /* mui default spacing in storage, 1 in sample bar, 0 in view */
		source ? MUIA_Group_Columns : TAG_IGNORE, 4 + 3,
		TAG_MORE, INITTAGS
	);

	if (obj)
	{
		struct Data *data;
		STRPTR barcode;

		data = INST_DATA(cl, obj);

		definition_all[TOOLBAR_ENUM_DEVICE_LIST].localized_name 				= GSI(MSG_PREFSWIN_TOOLBAR_DEVICE_LIST);
		definition_all[TOOLBAR_ENUM_DEVICE_LIST].localized_help					= GSI(MSG_PREFSWIN_TOOLBAR_DEVICE_LIST_HELP);
		
		definition_all[TOOLBAR_ENUM_COPY].localized_name 						= GSI(MSG_PREFSWIN_TOOLBAR_COPY);
		definition_all[TOOLBAR_ENUM_COPY].localized_help 						= GSI(MSG_PREFSWIN_TOOLBAR_COPY_HELP);
		
		definition_all[TOOLBAR_ENUM_COPY_AS].localized_name 					= GSI(MSG_PREFSWIN_TOOLBAR_COPY_AS);
		definition_all[TOOLBAR_ENUM_COPY_AS].localized_help						= GSI(MSG_PREFSWIN_TOOLBAR_COPY_AS_HELP);

		definition_all[TOOLBAR_ENUM_MOVE].localized_name 						= GSI(MSG_PREFSWIN_TOOLBAR_MOVE);
		definition_all[TOOLBAR_ENUM_MOVE].localized_help 						= GSI(MSG_PREFSWIN_TOOLBAR_MOVE_HELP);

		definition_all[TOOLBAR_ENUM_MOVE_AS].localized_name 					= GSI(MSG_PREFSWIN_TOOLBAR_MOVE_AS);
		definition_all[TOOLBAR_ENUM_MOVE_AS].localized_help 					= GSI(MSG_PREFSWIN_TOOLBAR_MOVE_AS_HELP);

		definition_all[TOOLBAR_ENUM_DELETE].localized_name 						= GSI(MSG_PREFSWIN_TOOLBAR_DELETE);
		definition_all[TOOLBAR_ENUM_DELETE].localized_help 						= GSI(MSG_PREFSWIN_TOOLBAR_DELETE_HELP);
	
		definition_all[TOOLBAR_ENUM_TRASH].localized_name 						= GSI(MSG_PREFSWIN_TOOLBAR_TRASH);
		definition_all[TOOLBAR_ENUM_TRASH].localized_help 						= GSI(MSG_PREFSWIN_TOOLBAR_TRASH_HELP);

		definition_all[TOOLBAR_ENUM_COPY_TO_CLIPBOARD].localized_name 			= GSI(MSG_PREFSWIN_TOOLBAR_COPY_TO_CLIPBOARD);
		definition_all[TOOLBAR_ENUM_COPY_TO_CLIPBOARD].localized_help			= GSI(MSG_PREFSWIN_TOOLBAR_COPY_TO_CLIPBOARD_HELP);

		definition_all[TOOLBAR_ENUM_MOVE_TO_CLIPBOARD].localized_name 			= GSI(MSG_PREFSWIN_TOOLBAR_MOVE_TO_CLIPBOARD);
		definition_all[TOOLBAR_ENUM_MOVE_TO_CLIPBOARD].localized_help 			= GSI(MSG_PREFSWIN_TOOLBAR_MOVE_TO_CLIPBOARD_HELP);

		definition_all[TOOLBAR_ENUM_PASTE_FROM_CLIPBOARD].localized_name 		= GSI(MSG_PREFSWIN_TOOLBAR_PASTE_FROM_CLIPBOARD);
		definition_all[TOOLBAR_ENUM_PASTE_FROM_CLIPBOARD].localized_help 		= GSI(MSG_PREFSWIN_TOOLBAR_PASTE_FROM_CLIPBOARD_HELP);

		definition_all[TOOLBAR_ENUM_PASTE_FROM_CLIPBOARD_AS].localized_name 	= GSI(MSG_PREFSWIN_TOOLBAR_PASTE_FROM_CLIPBOARD_AS);
		definition_all[TOOLBAR_ENUM_PASTE_FROM_CLIPBOARD_AS].localized_help 	= GSI(MSG_PREFSWIN_TOOLBAR_PASTE_FROM_CLIPBOARD_AS_HELP);

		definition_all[TOOLBAR_ENUM_SELECT].localized_name 						= GSI(MSG_PREFSWIN_TOOLBAR_SELECT);
		definition_all[TOOLBAR_ENUM_SELECT].localized_help						= GSI(MSG_PREFSWIN_TOOLBAR_SELECT_HELP);

		definition_all[TOOLBAR_ENUM_ICONINFO].localized_name 					= GSI(MSG_PREFSWIN_TOOLBAR_ICONINFO);
		definition_all[TOOLBAR_ENUM_ICONINFO].localized_help 					= GSI(MSG_PREFSWIN_TOOLBAR_ICONINFO_HELP);

		definition_all[TOOLBAR_ENUM_FORMAT].localized_name 						= GSI(MSG_PREFSWIN_TOOLBAR_FORMAT);
		definition_all[TOOLBAR_ENUM_FORMAT].localized_help 						= GSI(MSG_PREFSWIN_TOOLBAR_FORMAT_HELP);

		definition_all[TOOLBAR_ENUM_MAKEDIR].localized_name 					= GSI(MSG_PREFSWIN_TOOLBAR_MAKEDIR);
		definition_all[TOOLBAR_ENUM_MAKEDIR].localized_help 					= GSI(MSG_PREFSWIN_TOOLBAR_MAKEDIR_HELP);

		definition_all[TOOLBAR_ENUM_MAKELINK].localized_name 					= GSI(MSG_PREFSWIN_TOOLBAR_MAKELINK);
		definition_all[TOOLBAR_ENUM_MAKELINK].localized_help 					= GSI(MSG_PREFSWIN_TOOLBAR_MAKELINK_HELP);

		definition_all[TOOLBAR_ENUM_RENAME].localized_name 						= GSI(MSG_PREFSWIN_TOOLBAR_RENAME);
		definition_all[TOOLBAR_ENUM_RENAME].localized_help						= GSI(MSG_PREFSWIN_TOOLBAR_RENAME_HELP);

		definition_all[TOOLBAR_ENUM_FIND].localized_name 						= GSI(MSG_PREFSWIN_TOOLBAR_FIND);
		definition_all[TOOLBAR_ENUM_FIND].localized_help						= GSI(MSG_PREFSWIN_TOOLBAR_FIND_HELP);

		definition_all[TOOLBAR_ENUM_PARENT].localized_name 						= GSI(MSG_PREFSWIN_TOOLBAR_PARENT);
		definition_all[TOOLBAR_ENUM_PARENT].localized_help 						= GSI(MSG_PREFSWIN_TOOLBAR_PARENT_HELP);

		definition_all[TOOLBAR_ENUM_BACK].localized_name 						= GSI(MSG_PREFSWIN_TOOLBAR_BACK);
		definition_all[TOOLBAR_ENUM_BACK].localized_help 						= GSI(MSG_PREFSWIN_TOOLBAR_BACK_HELP);

		definition_all[TOOLBAR_ENUM_FORWARD].localized_name 					= GSI(MSG_PREFSWIN_TOOLBAR_FORWARD);
		definition_all[TOOLBAR_ENUM_FORWARD].localized_help 					= GSI(MSG_PREFSWIN_TOOLBAR_FORWARD_HELP);

		definition_all[TOOLBAR_ENUM_SWITCH_VIEW].localized_name 				= GSI(MSG_PREFSWIN_TOOLBAR_SWITCH_VIEW);
		definition_all[TOOLBAR_ENUM_SWITCH_VIEW].localized_help 				= GSI(MSG_PREFSWIN_TOOLBAR_SWITCH_VIEW_HELP);

		definition_all[TOOLBAR_ENUM_BOOKMARKS].localized_name 					= GSI(MSG_PREFSWIN_TOOLBAR_BOOKMARKS);
		definition_all[TOOLBAR_ENUM_BOOKMARKS].localized_help 					= GSI(MSG_PREFSWIN_TOOLBAR_BOOKMARKS_HELP);

		definition_all[TOOLBAR_ENUM_SPACE].localized_name 						= GSI(MSG_PREFSWIN_TOOLBAR_SPACE);
		definition_all[TOOLBAR_ENUM_SPACE].localized_help 						= GSI(MSG_PREFSWIN_TOOLBAR_SPACE_HELP);

		definition_all[TOOLBAR_ENUM_FLEXIBLE_SPACE].localized_name 				= GSI(MSG_PREFSWIN_TOOLBAR_FLEXIBLE_SPACE);
		definition_all[TOOLBAR_ENUM_FLEXIBLE_SPACE].localized_help 				= GSI(MSG_PREFSWIN_TOOLBAR_FLEXIBLE_SPACE_HELP);
	
		definition_all[TOOLBAR_ENUM_SEPARATOR].localized_name 					= GSI(MSG_PREFSWIN_TOOLBAR_SEPARTOR);
		definition_all[TOOLBAR_ENUM_SEPARATOR].localized_help 					= GSI(MSG_PREFSWIN_TOOLBAR_SEPARTOR_HELP);


		barcode	= (STRPTR)GetTagData(MA_Toolbargroup_Definition, MV_Toolbargroup_Definition_Default, INITTAGS);
		data->droppable = samplebar;
		data->source    = source;

		set( obj, MA_Toolbargroup_Definition, barcode );

		data->droppos = -1;
		data->olddroppos = -1;
		data->definition = NULL;
		data->definition_valid = FALSE;
		data->cm = NULL;
		data->aftersetup = 0;
		data->vo = NULL;
		data->extraspacer = HSpace(0);
	}
	return ((ULONG)obj);
}


DEFMMETHOD(DragDrop)
{
	GETDATA;

	{
		APTR owner;

		owner = _parent(msg->obj);

		/* check if we dropped it on source panel from other panel */

		if ( data->source && owner != obj )
		{
			/* delete object from panel (do NOT dispose here. postpone!) */

			DoMethod(owner, MUIM_Group_InitChange);
			DoMethod(owner, OM_REMMEMBER, msg->obj);
			DoMethod(app, MUIM_Application_PushMethod, app, 2, MM_Application_DisposeObject, msg->obj);
			DoMethod(owner, MUIM_Group_ExitChange);

			set(owner, MA_Toolbargroup_Definition, MV_Toolbargroup_Definition_Updated);
			return 0;

		}


		if ( owner != obj )
		{
			/*
			* We just make a copy, not move object 'physicly'.
			*/

			STRPTR name = (STRPTR)getv( msg->obj, MA_Toolbutton_Name );
			STRPTR args	= (STRPTR)getv( msg->obj, MA_Toolbutton_Args );
			APTR button_class = OCLASS( msg->obj );

			APTR new = NewObject( button_class, NULL,
								MA_Toolbutton_Name,     name,
								MA_Toolbutton_Args,     args,
								MUIA_Draggable,         TRUE,
								MA_Toolbutton_Location, data->droppable ? MV_Toolbutton_Location_Samplebar : MV_Toolbutton_Location_Toolbar, /* we can't be Storage here */
								MUIA_Frame,             MUIV_Frame_Button,
								MUIA_Background,        MUII_ButtonBack,
								TAG_DONE );

			if ( new )
			{
				DoMethod(obj, MUIM_Group_InitChange);

				if ( data->predobj )
				{
					DoMethod(obj, MUIM_Family_Insert, new, data->predobj );
				}
				else
				{
					DoMethod(obj, MUIM_Family_AddHead, new);
				}

				DoMethod(obj, MUIM_Group_ExitChange);

				set(obj, MA_Toolbargroup_Definition, MV_Toolbargroup_Definition_Updated);
			}
		}
		else if ( owner == obj && msg->obj !=data->predobj )
		{
			/*
			 * Move button inside the group.
			 */

			DoMethod(obj, MUIM_Group_InitChange);
			DoMethod(obj, OM_REMMEMBER, msg->obj );

			if ( data->predobj )
			{
				DoMethod(obj, MUIM_Family_Insert, msg->obj, data->predobj );
			}
			else
			{
				DoMethod(obj, MUIM_Family_AddHead, msg->obj);
			}

			DoMethod(obj, MUIM_Group_ExitChange);

			set(obj, MA_Toolbargroup_Definition, MV_Toolbargroup_Definition_Updated);
		}
	}
	return (0);
}

DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_Toolbargroup_Source:
			*msg->opg_Storage = data->source;
			return TRUE;
			break;

		case MA_Toolbargroup_Draggable:
			*msg->opg_Storage = data->droppable;
			return TRUE;
			break;

		case MA_Toolbargroup_Definition:
			{
				if ( !data->definition_valid )
				{
					/* rebuld definition string. */

					struct button_definition *bd;
					ULONG n = 0;

					FORCHILD( obj, MUIA_Group_ChildList )
					{
						n++;
					}
					NEXTCHILD;

					bd = malloc( ( n + 1 ) * sizeof( struct button_definition ) );

					if ( bd )
					{
						n = 0;

						FORCHILD( obj, MUIA_Group_ChildList )
						{
							const struct buttonclass_definition *bcd;

							/* lookup class id */

							for(bcd = avail_buttons; bcd->id && bcd->getclassfunc() != OCLASS( child ); bcd++);

							if ( bcd->id )
							{

								bd[ n ].name = name_build( (STRPTR)getv( child, MA_Toolbutton_Name ) );
								bd[ n ].buttonclass_id = bcd->id;

								/* merge all args from array */

								{
									ULONG *array = (ULONG*)getv( child, MA_Toolbutton_Args );
									String *args = string_new_len( NULL, 128 );

									if ( array && args )
									{
										while( *array )
										{
											string_append_printf( args, "\"%s\"", (STRPTR)(*array) );
											array++;
											if ( *array )
											{
												string_append( args, " " );
											}
										}
									}

									bd[ n ].args = string_free( args, TRUE );
								}

								n++;
							}
						}
						NEXTCHILD;

						bd[ n ].name = NULL;

						data->definition = definition_serialize( bd );
						definition_delete( bd );
						data->definition_valid = TRUE;
					}
				}

				*msg->opg_Storage = (ULONG)( data->definition_valid ? data->definition : NULL );
			}

			return TRUE;
	}

	return (DOSUPER);
}

DEFSET
{
	GETDATA;

	FORTAG(INITTAGS)
	{
		case MA_Toolbargroup_Definition:
			{
				struct button_definition *bd = NULL;
				ULONG nbuttons = 0;
				ULONG nbuttons_created = 0;
				CONST_STRPTR barcode = (STRPTR)tag->ti_Data;
				ULONG release_definition = FALSE;
				BOOL hasflexiblespace = FALSE;

				if ( (ULONG)barcode == MV_Toolbargroup_Definition_All )
				{
					bd = (struct button_definition *)definition_all;
				}
				else if ( (ULONG)barcode == MV_Toolbargroup_Definition_Default )
				{
					bd = (struct button_definition *)definition_default;
				}
				else if ( (ULONG)barcode == MV_Toolbargroup_Definition_Updated )
				{
					data->definition_valid = FALSE;
					break;
				}
				else
				{
					bd = definition_deserialize( barcode );
					release_definition = TRUE;
				}

				DoMethod( obj, MUIM_Group_InitChange );

				/* remove existing objects*/

				FORCHILD(obj, MUIA_Group_ChildList)
				{
					if (child != data->extraspacer)
					{
						DoMethod(obj, OM_REMMEMBER, child);
						MUI_DisposeObject(child);
					}
				}
				NEXTCHILD

				/* add new objects */

				while ( bd && bd[ nbuttons ].name )
				{
					APTR bo = NULL;

					if ( bd[ nbuttons ].buttonclass_id > 0 )
					{
						const struct buttonclass_definition *bcd;

						for (bcd = avail_buttons; bcd->id && bcd->id != bd[ nbuttons ].buttonclass_id; bcd++);

						if ( bcd->id > 0 )
						{
							struct RDArgs *rda = NULL;
							ULONG array[ 32 ]; /* XXX: Should be enough(tm) */

							//DB(("Name:%s, Type:%d, Template:%s, Args: %s\n", bd[ nbuttons ].name, bcd->id, bcd->template, bd[ nbuttons ].args ));

							/* read args */

							memset(array, 0, sizeof( array ) );

							if ( bcd->argnum )
							{
								rda = readargsstring(bd[ nbuttons ].args, bcd->template, array);
							}

							if ( bcd->argnum == 0 || rda )
							{
								/* got all info, build button */

								LONG i = 0;
								STRPTR localized_name = (STRPTR)bd[ nbuttons ].name;
								STRPTR localized_help = (STRPTR)bd[ nbuttons ].name;
								while(definition_all[i].name)
								{
									if(!stricmp(definition_all[i].name, bd[ nbuttons ].name))
									{
										localized_name = definition_all[i].localized_name;
										localized_help = definition_all[i].localized_help;
										break;
									}
									i++;
								}

								if ( data->source )
								{
									/*
									 * Build group with label underneath.
									 */

									bo = VGroup,
											Child, NewObject( bcd->getclassfunc(), NULL,
													MA_Toolbutton_Name,     bd[ nbuttons ].name,
													MA_Toolbutton_Help, 	localized_help,
													MA_Toolbutton_Args,     array,
													MA_Toolbutton_Location, MV_Toolbutton_Location_Storage,
													MUIA_Draggable,         data->droppable,
													TAG_DONE ),
											Child, TextObject,
													MUIA_Text_Contents,     localized_name,
													MUIA_Text_PreParse,     "\33c",
													End,
										End;
								}
								else
								{
									bo = NewObject( bcd->getclassfunc(), NULL,
											MA_Toolbutton_Name,     bd[ nbuttons ].name,
											MA_Toolbutton_Help, 	localized_help,
											MA_Toolbutton_Args,     array,
											MA_Toolbutton_Location, data->droppable ? MV_Toolbutton_Location_Samplebar : MV_Toolbutton_Location_Toolbar,
											MUIA_Draggable,         data->droppable,
											!data->droppable ? TAG_IGNORE: MUIA_Frame,      MUIV_Frame_Button,
											!data->droppable ? TAG_IGNORE: MUIA_Background, MUII_ButtonBack,
											TAG_DONE );

									if ( !data->droppable )
									{
										bo = create_button_final( bo, _conf( toolbar_displaymode ) == 0 ? LABEL_BOTTOM : LABEL_OFF );
									}
								}

								/*  in case the user has configured no Flexible Space at all we need to add one later to avoid
								 *  layout problems with MUI. 
								 */
								if ((bd[nbuttons].buttonclass_id == BTN_SPACER) && (bcd->argnum == 1) && (*(STRPTR)(array[0]) == '0')) /* flexible space */
								{
									hasflexiblespace = TRUE;
								}

								if ( rda )
									freeargsstring(rda);
							}
						}
						else
						{
							//DB(("Button class not found (id=%d)\n", bd[ nbuttons ].buttonclass_id ));
						}
					}

					if ( bo )
					{
						button_set_visible( obj, data, bo );
						DoMethod(obj, OM_ADDMEMBER, bo);

						if ( data->source && ( nbuttons_created % 4 ) != 3 )
						{
							DoMethod( obj, OM_ADDMEMBER, HSpace(0) );
						}

						nbuttons_created++;

					}

					nbuttons++;
				}

				if ( data->vo == NULL && ((!data->source && (!hasflexiblespace))
                     || data->droppable) /* XXX: sucks, but if we remove all flexible spaces per drag'n'drop in prefs it confuses. Needs some more work. */
                   )
				{
					APTR so = RectangleObject,
								MUIA_FixHeight, data->droppable ? 24+2 : 1, /* XXX: moo? :) (attention: 24 == minheight in the subclasses) */
								/*   
								 *   XXX: this causes a weird MUI bug when the parent group is resized to a 
								 *        width > 1280 (with weight of 1), we need to slap stuntzi until 
								 *        it's fixed or always dynamically show/hide the RectangeleObject
								 *        (w/o MUIA_Weight set aka default) depending if we have 'Flexible Space' 
								 *        in the toolbar or not (needs to be synced after DnD ops too)  
								 *        -- tokai
								 *
								 *   use a weight of 3 for now (works at least up to 1680 ;)
								 */ 
								data->droppable ? MUIA_Weight : TAG_IGNORE, 3,   
								End;
					if ( so )
						DoMethod( obj, OM_ADDMEMBER, so );
				}

				DoMethod( obj, MUIM_Group_ExitChange );

				/*   hide the toolbar from View windows when it's empty (avoids ugly extra space on top of 
				 *   clickpath). This also automatically unhides it, if required.
				 */
				if (!(data->droppable || data->source))
				{
					set(obj, MUIA_ShowMe, nbuttons_created ? TRUE : FALSE);
				}

				if ( release_definition )
					definition_delete( bd );

			}
			break;
	}
	NEXTTAG;

	return (DOSUPER);
}


DEFMMETHOD(DragQuery)
{
	GETDATA;
	APTR owner;
	ULONG dummy;

	if ( data->source )
	{
		/* owner is actualy parent of parent */
		owner = _parent( _parent(msg->obj) );
	}
	else
	{
		owner = _parent(msg->obj);
	}

	if(!GetAttr(MA_Toolbutton_Label, msg->obj, &dummy))
	{
		return (MUIV_DragQuery_Refuse);
	}

	if ( !data->source && data->droppable )
	{
		return (MUIV_DragQuery_Accept);
	}
	else if ( data->source && data->droppable && owner != obj )
	{
		return (MUIV_DragQuery_Accept);
	}

	return (MUIV_DragQuery_Refuse);
}


DEFMMETHOD(Draw)
{
	ULONG rc;
	GETDATA;

	rc = DOSUPER;

	//if ( data->droppos != data->olddroppos )
	{
		if ( data->olddroppos != -1 )
		{
			DoMethod( obj, MUIM_DrawBackground, data->olddroppos + 1, _mtop(obj), 1, _mheight(obj), 0, 0, 0 );
		}

		if ( data->droppos != -1 )
		{
			/*   draw a faked dropmark ;)
			 */
			ULONG y;
			UBYTE moo3 = 0;
			BOOL  cs = FALSE;
	
			for (y = _mtop(obj); y < _mbottom(obj); y++, moo3++)
			{
				if (moo3 > 3) 
				{
					moo3 = 0;
					cs = !cs;
				}
		
				WriteRGBPixel(_rp(obj), (ULONG)data->droppos + 1, y, (cs ? 0xffffffff : 0xff000000));
			} 
		}
	}

	data->olddroppos = data->droppos;

	return rc;
}

DEFMMETHOD(DragBegin)
{

	GETDATA;

	data->droppos = -1;
	data->predobj = NULL;

	return DOSUPER;
}

DEFMMETHOD(DragFinish)
{

	GETDATA;

	data->droppos = -1;

	return DOSUPER;
}

DEFMMETHOD(DragReport)
{
	GETDATA;

	LONG pos = getv( obj, MUIA_LeftEdge );
	APTR predobj = NULL;

	if ( data->source )
		return MUIV_DragReport_Continue;

	if (msg->update)
	{
		MUI_Redraw(obj, MADF_DRAWOBJECT);
		return (MUIV_DragReport_Continue);
	}

	if ( !_isinobject(msg->x, msg->y))
	{
		return (MUIV_DragReport_Continue);
	}

	/*
	 * Check where to drop the new button.
	 */

	FORCHILD(obj, MUIA_Group_ChildList)
	{
		STRPTR name;

		/* Check if it's a button actualy */

		if ( get( child, MA_Toolbutton_Name, &name ) )
		{
			if ( msg->x > getv( child, MUIA_RightEdge ) )
			{
				if ( getv( child, MUIA_RightEdge ) > pos )
				{
					pos = getv( child, MUIA_RightEdge );
					predobj = child;
				}
			}
		}
	}
	NEXTCHILD

	if ( pos != data->droppos )
	{
		data->droppos = pos;
		return MUIV_DragReport_Refresh;

	}

	data->predobj = predobj;
	return MUIV_DragReport_Lock;

}

DEFMMETHOD(AskMinMax)
{
	GETDATA;

	DOSUPER;

	if ( data->source )
	{
		msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
		msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;
	}

	return (0);
}

DEFMMETHOD(Setup)
{
	/* lookup view object and install notify */

	ULONG rc;

	if ( (rc = DOSUPER) )
	{
		GETDATA;

		if ( !data->aftersetup )
		{
			APTR vo = _view( obj );

			data->aftersetup = TRUE;

			if ( vo )
			{
				data->vo = vo;

				/* setup notify while we are at it.. */

				DoMethod( vo, MUIM_Notify, MA_Viewgroup_ViewChanged, TRUE,
					obj, 1, MM_Toolbargroup_UpdateVisible );

				/* ...and do initial setup */

				DoMethod( app, MUIM_Application_PushMethod, obj, 1, MM_Toolbargroup_UpdateVisible );

			}
		}
	}

	return rc;
}

DEFTMETHOD(Toolbargroup_UpdateVisible)
{
	GETDATA;

	if ( data->vo )
	{
		/* iterate buttons and setup their visibility */

		DoMethod(obj, MUIM_Group_InitChange);

		FORCHILD(obj, MUIA_Group_ChildList)
		{
			button_set_visible( obj, data, child );
		}
		NEXTCHILD
		
		DoMethod(obj, MUIM_Group_ExitChange );
	}

	return 0;
}


BEGINMTABLE
DECNEW
DECGET
DECSET
DECMMETHOD(DragDrop)
DECMMETHOD(Setup)
DECMMETHOD(DragQuery)
DECMMETHOD(DragReport)
DECMMETHOD(DragBegin)
DECMMETHOD(DragFinish)
DECMMETHOD(Draw)
DECMMETHOD(AskMinMax)
DECTMETHOD(Toolbargroup_UpdateVisible)

ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, toolbargroupclass)
