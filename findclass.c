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
 * $Id: findclass.c,v 1.24 2025/09/15 12:40:36 bitrocky Exp $
 */


#include "ambient.h"

/* public */
#include <proto/dos.h>
#include <libraries/asl.h>
#include <mui/Listtree_mcc.h>


/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "screen.h"
#include "threads.h"
#include "methodstack.h"
#include "mimetype.h"
#include "findclass.h"
#include "recurse.h"
#include "name.h"
#include "file_io.h"
#include "storage.h"
#include "doslistcache.h"
#include "mimegroupclass.h"

struct Data {
	APTR pop_name;
	APTR pop_text;
	APTR pop_type;
	APTR pop_comment;
	
	APTR str_type;
	APTR bt_start;
	APTR bt_stop;
	APTR lst_results;
	APTR lst_targets;
	APTR txt_current;
	APTR str_target;
	APTR pop_target;
	APTR lst_types;

	APTR thread;
	STRPTR path;
	ULONG accept;

	ULONG dispose;
	ULONG restart;

	struct MUI_InputHandlerNode ihnode;

	/* cached filtering values */

	STRPTR find_name;
	STRPTR find_text;
	STRPTR find_comment;
	APTR find_type;
	STRPTR find_mime;
	TEXT current_path[ 512 ];   /* ok, this sucks, but allocating and releasing mem for each path would suck even more.. */
};

struct targetnode
{
	struct MinNode n;
	TEXT target[0];
};


DEFNEW
{
	struct Data *data;
	APTR mi_quit;
	APTR bt_start, bt_stop, pop_type, lst_types, gp_typesearch, str_type, bt_type, lst_targets, lst_results, txt_current;
	APTR str_target, pop_target, bt_target_add, bt_target_remove;
	APTR pop_name, pop_text, pop_comment;

	obj = DoSuperNew(cl, obj,
		MUIA_Window_Screen,       get_screen(),
		MUIA_Window_ScreenTitle,  screentitle,
		MUIA_Window_Title,        GSI(MSG_FINDCLASS_TITLE),
		MUIA_Window_ID,           MAKE_ID('F','I','N','D'),
		MUIA_Window_Width,        400,
		MUIA_Window_Height,       550,   // MUIV_Window_Width_MinMax(10),
		MUIA_Window_ShowIconify,  FALSE,
		MUIA_Window_ShowPrefs,    FALSE,
		MUIA_Window_ShowJump,     FALSE,
		MUIA_Window_ShowSnapshot, TRUE,
		MUIA_Window_ShowPopup,    FALSE,
		MUIA_Window_ShowAbout,    FALSE,
		MUIA_Window_Menustrip, MenustripObject,
			Child, MenuObject,
				MUIA_Menu_Title, GSI( MSG_FINDCLASS_MENU_PROJECT ),
				Child, mi_quit = MenuitemObject,
					MUIA_Menuitem_Title,    GSI( MSG_FINDCLASS_MENU_QUIT ),
					MUIA_Menuitem_Shortcut, "Q",
				End,
			End,
		End,

		WindowContents, VGroup,
			Child, ColGroup(2),
				Child, MUICreateLabel( MSG_FINDCLASS_NAME, MUIO_Label_LeftAligned|MUIO_Label_SingleFrame),
				Child, pop_name = NewObject(getnavigationclass(), NULL,
									MA_Navigation_MaxHistoryItems, 10,
									MA_Navigation_PathPopup,       FALSE,
									MA_Navigation_KeepActive,      FALSE,
									MA_Navigation_StorageID,       STORAGE_FIND_NAME_HISTORY,
									MA_Navigation_DiskStorage,     FALSE,
									MUIA_ControlChar,              MUIGetUnderScore( MSG_FINDCLASS_NAME ),
									MUIA_ShortHelp,                GSI( MSG_FINDCLASS_NAME_HELP),

									TAG_DONE),

				Child, MUICreateLabel( MSG_FINDCLASS_TEXT, MUIO_Label_LeftAligned|MUIO_Label_SingleFrame),
				Child, pop_text = NewObject(getnavigationclass(), NULL,
									MA_Navigation_MaxHistoryItems, 10,
									MA_Navigation_PathPopup,       FALSE,
									MA_Navigation_KeepActive,      FALSE,
									MA_Navigation_StorageID,       STORAGE_FIND_TEXT_HISTORY,
									MA_Navigation_DiskStorage,     FALSE,
									MUIA_ControlChar,              MUIGetUnderScore( MSG_FINDCLASS_TEXT ),
									MUIA_ShortHelp,                GSI( MSG_FINDCLASS_TEXT_HELP),
									TAG_DONE),

				Child, MUICreateLabel( MSG_FINDCLASS_TYPE, MUIO_Label_LeftAligned|MUIO_Label_SingleFrame),
				Child, pop_type = PopobjectObject,
									MUIA_ControlChar, MUIGetUnderScore( MSG_FINDCLASS_TYPE ),
									MUIA_ShortHelp, GSI( MSG_FINDCLASS_TYPE_HELP),
									MUIA_CycleChain, 1,
									MUIA_Popstring_Toggle, TRUE,
									MUIA_Popstring_Button, bt_type  = MUICreatePopButton( MSG_FINDCLASS_TYPE, MUII_PopUp, "FINDTYPE"),
									MUIA_Popstring_String, str_type = StringObject,
										MUIA_Frame, MUIV_Frame_String,
										End,
									MUIA_Popobject_Object, VGroup,
										Child, lst_types = NewObject(getmimelisttreeclass(), NULL,
											MA_Mimelisttree_MarkDefined, FALSE,
											MA_Mimelisttree_ShowInternal, FALSE,
											TAG_DONE),
										Child, gp_typesearch = NewObject(getsearchbarclass(), NULL,
											MA_Searchbar_Target, lst_types,
											MA_Searchbar_Flags, MV_Searchbar_Flags_ShowNext,
											MA_Searchbar_AutoHide, FALSE,
											TAG_DONE),
										End,
									End,

				Child, MUICreateLabel( MSG_FINDCLASS_COMMENT, MUIO_Label_LeftAligned|MUIO_Label_SingleFrame),
				Child, pop_comment = NewObject(getnavigationclass(), NULL,
									MA_Navigation_MaxHistoryItems, 10,
									MA_Navigation_PathPopup,       FALSE,
									MA_Navigation_KeepActive,      FALSE,
									MA_Navigation_StorageID,       STORAGE_FIND_COMMENT_HISTORY,
									MA_Navigation_DiskStorage,     FALSE,
									MUIA_ControlChar,              MUIGetUnderScore( MSG_FINDCLASS_COMMENT ),
									MUIA_ShortHelp,                GSI( MSG_FINDCLASS_COMMENT_HELP),
									TAG_DONE),
				End,

				/* search targets/locations */

			Child, VGroup, GroupFrameT(GSI(MSG_FINDCLASS_LOCATIONS)),
				MUIA_Weight, 10,

				Child, lst_targets = ListObject,
					InputListFrame,
					MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
					MUIA_List_DestructHook , MUIV_List_DestructHook_String,
					MUIA_List_MultiSelect  , MUIV_List_MultiSelect_Shifted, // bitRocky
					MUIA_ShortHelp         , GSI( MSG_FINDCLASS_LOCATIONS_HELP),
					End,

				Child, HGroup,
					Child, pop_target = PopaslObject,
						MUIA_Weight, 1000,
						MUIA_ShortHelp, GSI( MSG_FINDCLASS_ADDSTRING_HELP),
						MUIA_Popasl_Type, ASL_FileRequest,
						MUIA_Popstring_String, str_target = StringObject,
							MUIA_Frame, MUIV_Frame_String,
							MUIA_CycleChain, 1,
							End,
						MUIA_Popstring_Button, MUICreatePopButton( (MSG_FINDCLASS_ADDSTRING_HELP-1), MUII_PopFile, "FINDADST"),
						End,

					Child, bt_target_add = MUICreateButton( MSG_FINDCLASS_ADD, "FINDADDD" ),
					Child, bt_target_remove = MUICreateButton( MSG_FINDCLASS_REMOVE, "FINDREMO" ),
					End,
				End,

			Child, txt_current = TextObject,
				MUIA_Text_Shorten, MUIV_Text_Shorten_Cutoff,
				End,

			Child, VGroup, GroupFrameT(GSI(MSG_FINDCLASS_RESULTS)),
				Child, lst_results = NewObject(getfindresultlistclass(), NULL,
					MUIA_ControlChar , MUIGetUnderScore( MSG_FINDCLASS_RESULTS ),
					MUIA_ShortHelp   , GSI( MSG_FINDCLASS_RESULTS_HELP),
					MUIA_CycleChain  , 0,
					TAG_DONE),
				End,

			Child, HGroup,
				Child, bt_start = MUICreateButton( MSG_FINDCLASS_SEARCH, "FINDSEAR" ),
				Child, bt_stop = MUICreateButton( MSG_FINDCLASS_STOP, "FINDSTOP" ),
			End,
		End,
	End;

	if (obj == NULL)
		return ((ULONG)NULL);

	data = INST_DATA(cl, obj);

	data->thread      = NULL;
	data->accept      = FALSE;
	data->pop_text    = pop_text;
	data->pop_name    = pop_name;
	data->pop_type    = pop_type;
	data->str_type    = str_type;
	data->pop_comment = pop_comment;
	data->txt_current = txt_current;
	data->lst_results = lst_results;
	data->lst_targets = lst_targets;
	data->str_target  = str_target;
	data->bt_start    = bt_start;
	data->bt_stop     = bt_stop;
	data->lst_types   = lst_types;

	DoMethod(lst_targets, MUIM_Notify, MUIA_List_DoubleClick, TRUE,
		obj, 1, MM_Find_UpdateTarget
	);

	DoMethod(bt_start, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 1, MM_Find_Start
	);
	DoMethod(bt_stop, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_Find_Stop, FALSE
	);

	DoMethod(bt_target_add, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_Find_AddLocation, NULL
	);
	DoMethod(str_target, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
		obj, 2, MM_Find_AddLocation, NULL
	);

	DoMethod(bt_target_remove, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_Find_RemLocation, NULL
	);

	DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
		app, 5, MUIM_Application_PushMethod, obj, 2, MM_Window_Close, TRUE
	);

	DoMethod(pop_text, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
		obj, 2, MM_Find_Stop, TRUE
	);

	DoMethod(pop_name, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
		obj, 2, MM_Find_Stop, TRUE
	);

	DoMethod(pop_comment, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
		obj, 2, MM_Find_Stop, TRUE
	);

	DoMethod(pop_type, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
		obj, 2, MM_Find_Stop, TRUE
	);

	DoMethod(data->lst_types, MUIM_Notify, MUIA_Listtree_DoubleClick, MUIV_EveryTime,
		obj, 1, MM_Find_TypeSelected
	);

	set(bt_stop, MUIA_Disabled, TRUE );
	set(obj, MUIA_Window_ActiveObject, getv(pop_name, MA_Navigation_StringObject));

	DoMethod(pop_name, MM_Navigation_LoadHistory);
	DoMethod(pop_text, MM_Navigation_LoadHistory);
	DoMethod(pop_comment, MM_Navigation_LoadHistory);

/* menu item notifies */

	DoMethod(mi_quit, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, obj, 1, MM_Window_Close );

	return ((ULONG)obj);
}

DEFDISP
{
	GETDATA;

	if ( data->find_name )
		name_delete( data->find_name );
	if ( data->find_text )
		name_delete( data->find_text );
	if ( data->find_mime )
		name_delete( data->find_mime );
	if ( data->find_comment )
		name_delete( data->find_comment);

	return (DOSUPER);
}


DEFGET
{
	switch (msg->opg_AttrID)
	{
		case MA_Window_ID:
			*msg->opg_Storage = 0;
			return (TRUE);

		case MA_Window_Path:
			*msg->opg_Storage = (ULONG)NULL;
			return (TRUE);

		case MA_Window_Type:
			*msg->opg_Storage = MV_Window_Type_Find;
			return (TRUE);
	}
	return (DOSUPER);
}

DEFSET
{
	GETDATA;

	FORTAG(INITTAGS)
	{
		case MA_Find_Pattern:
			if ( tag->ti_Data && ((STRPTR)tag->ti_Data)[0] )
				set( data->pop_name, MUIA_String_Contents, tag->ti_Data );
			break;

		case MA_Find_Text:
			if ( tag->ti_Data && ((STRPTR)tag->ti_Data)[0] )
				set( data->pop_text, MUIA_String_Contents, tag->ti_Data );
			break;

	}
	NEXTTAG

	return (DOSUPER);
}

DEFSMETHOD(Find_Start)
{
	GETDATA;
	STRPTR name, text, type, comment;

	name = (STRPTR)getv( data->pop_name, MUIA_String_Contents );
	text = (STRPTR)getv( data->pop_text, MUIA_String_Contents );
	type = (STRPTR)getv( data->str_type, MUIA_String_Contents );
	comment = (STRPTR)getv( data->pop_comment, MUIA_String_Contents );

	if ( (name[0] == 0) && (text[0] == 0) && (type[0] == 0) && (comment[0] == 0) )
	{
		set( data->txt_current, MUIA_Text_Contents, GSI(MSG_FINDCLASS_FINISHED) );
		return FALSE; 
	}
	
    if ( data->find_name )
		name_delete( data->find_name );
	if ( data->find_text )
		name_delete( data->find_text );
	if ( data->find_mime )
		name_delete( data->find_mime );
	if ( data->find_comment )
		name_delete( data->find_comment );

	data->restart = FALSE;
	data->find_name = name_build( name );
	data->find_text = name_build( text );
	data->find_mime = name_build( type );
	data->find_comment = name_build( comment );

	if ( data->find_mime != NULL && strlen( data->find_mime ) > 0 )
		data->find_type = (APTR)mimetype_find_by_description( data->find_mime );
	else
		data->find_type = NULL;

	DoMethod(data->pop_name, MM_Navigation_LoadHistory);
	DoMethod(data->pop_text, MM_Navigation_LoadHistory);
	DoMethod(data->pop_comment, MM_Navigation_LoadHistory);

	DoMethod(data->pop_name, MM_Navigation_InsertHistory, data->find_name );
	DoMethod(data->pop_text, MM_Navigation_InsertHistory, data->find_text );
	DoMethod(data->pop_comment, MM_Navigation_InsertHistory, data->find_comment );

	DoMethod(data->pop_name, MM_Navigation_SaveHistory);
	DoMethod(data->pop_text, MM_Navigation_SaveHistory);
	DoMethod(data->pop_comment, MM_Navigation_SaveHistory);

	DoMethod( data->lst_results, MUIM_List_Clear);

	strcpy( data->current_path, "" );
	data->ihnode.ihn_Object = obj;
	data->ihnode.ihn_Flags  = MUIIHNF_TIMER;
	data->ihnode.ihn_Millis = 100;
	data->ihnode.ihn_Method = MM_Find_Update;

	if ( do_action(obj, TA_Find,
			TT_Find_Window, obj,
		TAG_DONE) )
	{
		set( data->bt_start, MUIA_Disabled, TRUE );
		set( data->bt_stop, MUIA_Disabled, FALSE );

		DoMethod(app, MUIM_Application_AddInputHandler, &data->ihnode);
	}


	return TRUE;
}

DEFSMETHOD(Find_Stop)
{
	GETDATA;
	set( data->bt_stop, MUIA_Disabled, TRUE );
	data->restart = msg->restart;
	threads_abort( obj, TA_Find, NULL );
	return 0;
}

DEFSMETHOD(Find_AddResult)
{
	GETDATA;

	if ( msg->result )
	{
		struct result_item item;
		item.filepart = msg->result;
		item.comment = msg->comment;
		DoMethod( data->lst_results, MUIM_List_InsertSingle, &item, MUIV_List_Insert_Sorted );
	}

	return TRUE;
}

DEFSMETHOD(Find_AddLocation)
{
	GETDATA;
	STRPTR loc = msg->location;

	if ( !loc )
	{
		loc = (STRPTR)getv( data->str_target, MUIA_String_Contents );
	}

	if ( loc && loc[0] )
	{
		if (strcmp(loc, "[VOLUMES]") == 0)
		{
			struct dlcnode *dlcn;

			doslistcache_lock();

			ITERATEDLC(dlcn)
			{
				if ( dlcn->type == DLT_VOLUME && (dlcn->state == DLC_NEW || dlcn->state == DLC_ADDED))
				{
					TEXT t[VOLUME_SIZE+1];

					strcpy(t, dlcn->name);
					strcat(t, ":");
					DoMethod( data->lst_targets, MUIM_List_InsertSingle, t, MUIV_List_Insert_Bottom );
				}
			}

			doslistcache_unlock();
			DoMethod(data->lst_targets, MUIM_List_Sort);
		}
		else
		{
			DoMethod( data->lst_targets, MUIM_List_InsertSingle, loc, MUIV_List_Insert_Bottom );
		}
	}

	return loc ? TRUE : FALSE;
}

DEFSMETHOD(Find_RemLocation)
{
	GETDATA;
	
	DoMethod( data->lst_targets, MUIM_List_Remove, MUIV_List_Remove_Selected );

	return TRUE;
}

DEFTMETHOD(Find_Update)
{
	GETDATA;

	set( data->txt_current, MUIA_Text_Contents, data->current_path );

	return 0;
}

DEFTMETHOD(Find_TypeSelected)
{
	GETDATA;
	TEXT mimetype[ 256 ] = "";
	struct MUIS_Listtree_TreeNode *active = NULL, *parent = NULL;

	active = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lst_types, MUIM_Listtree_GetEntry, NULL, MUIV_Listtree_GetEntry_Position_Active, 0);

	if(active != NULL)
	{
		if(((struct treedata *)active->tn_User)->flags & NODEFLAG_MEDIATYPE) /* it's a mediatype family */
		{
			snprintf(mimetype, sizeof(mimetype), "%s/*", active->tn_Name);
		}
		else /* it's the mimetype */
		{
			parent = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lst_types, MUIM_Listtree_GetEntry, active, MUIV_Listtree_GetEntry_Position_Parent, 0);

			if(parent)
			{
				snprintf(mimetype, sizeof(mimetype), "%s", ((struct treedata *)active->tn_User)->longname);
			}
		}
	}

	set(data->str_type, MUIA_String_Contents, mimetype);
	DoMethod(data->pop_type, MUIM_Popstring_Close, FALSE);

	return 0;
}

DEFTMETHOD(Find_UpdateTarget)
{
	GETDATA;
	STRPTR target;

	DoMethod(data->lst_targets, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &target);
	if (target)
	{
		set(data->str_target, MUIA_String_Contents, target);
	}

	return 0;
}


DEFSMETHOD(Window_Close)
{
	GETDATA;

	data->dispose = TRUE;
	DoMethod( obj, MM_Find_Stop, FALSE );
	set( obj, MUIA_Window_Open, FALSE );

	return 0;
}

DEFSMETHOD(Thread_Finished)
{
	GETDATA;

	if (msg->action == TA_Find )
	{
		set( data->bt_start, MUIA_Disabled, FALSE );
		set( data->bt_stop , MUIA_Disabled, TRUE );

		if ( data->ihnode.ihn_Object )
			DoMethod(app, MUIM_Application_RemInputHandler, &data->ihnode);
		data->ihnode.ihn_Object = NULL;

		if ( data->dispose )
		{
			DoMethod( app, MUIM_Application_PushMethod, app, 2, MM_Application_DisposeWindow , obj );
		}
		else
		{
			set( data->txt_current, MUIA_Text_Contents, GSI(MSG_FINDCLASS_FINISHED) );

			if ( data->restart )
			{
				DoMethod(obj, MM_Find_Start);
			}
		}

	}

	return 0;
}

/*
 * QuickSearch algoritm (Modified Bayer-Moore).
 * http://www-igm.univ-mlv.fr/~lecroq/string/index.html
 */

#define ASIZE 256   /* alphabet size */

static void preQsBc(UBYTE *x, LONG m, LONG qsBc[])
{
	LONG i;

	for (i = 0; i < ASIZE; ++i)
		qsBc[i] = m + 1;
	for (i = 0; i < m; ++i)
		qsBc[x[i]] = m - i;
}


static LONG QuickSearch(UBYTE *x, LONG m, UBYTE *y, LONG n)
{
	LONG j, qsBc[ASIZE];

	/* Preprocessing */
	preQsBc(x, m, qsBc);

	/* Searching */
	j = 0;
	while (j <= n - m)
	{
		if (memcmp(x, y + j, m) == 0)
			return j;
		j += qsBc[y[j + m]];               /* shift */
	}

	return -1;
}

#define BUFFERSIZE (4096*10) /* XXX: perhaps should be 8192 ? test.. */
#define IOBUFFERSIZE (10*8192 * 2)

static ULONG file_findtext(CONST_STRPTR path, STRPTR text)
{
	APTR af;
	STRPTR buf;
	ULONG rc = FALSE;
	ULONG offset = 0;
	ULONG readcnt = 0;

	THREAD;
	ASSERT(path);
	ASSERT(text);

	/*
	 * Method s simple:
	 * read into buffer + size of matched text
	 * try to match
	 * copy 'size of matched text' bytes from end to begining. go to 1.
	 */

	if ( (buf = malloc(BUFFERSIZE+1)) )
	{
		ULONG textlen;
		ULONG i;

		textlen = strlen(text);

	/*
	 * IMO this is very bad design. I'd rather see the caller
	 * lowercase the string. - piru
	 */
		for(i = 0; i < textlen; i++)
			text[i] = tolower(text[i]);

		if ( (af = file_open(path, MODE_OLDFILE)) )
		{
			LONG len;

			while ( (len = file_readmost(af, buf + offset, BUFFERSIZE - offset )) > 0 )
			{
				readcnt += len;

				if (threads_check_abort())
				{
					rc = ABORTED;
					goto done;
				}

				buf[ len + offset ] = 0;

				for(i=0; i<len + offset; i++)
				{
					if ( isalpha( buf[ i ] ) )
						buf[ i ] = tolower( buf[ i ] );
				}

				if ( QuickSearch(text, textlen, buf, len + offset) != -1 )
				{
					rc = TRUE;
					goto done;
				}

				/*
				 * Copy end of buffer to the begining.
				 */

				if ( len == BUFFERSIZE - offset )
				{
					memcpy(buf, buf + BUFFERSIZE - textlen, textlen);
					offset = textlen;
				}
				else
				{
					goto done;
				}
			}

			done:

			file_close(af);
		}
		free(buf);
	}

	return rc;
}

static ULONG checkfile(APTR obj UNUSED, CONST_STRPTR path, LONG type UNUSED, ULONG prot UNUSED, UQUAD size UNUSED, APTR userdata, CONST_STRPTR comment)
{
	APTR wo = userdata;
	struct IClass *cl = OCLASS( wo );
	struct Data *data = INST_DATA( cl, wo );
	LONG match = TRUE;
	ULONG pathlen = strlen(path);

	if (pathlen > sizeof( data->current_path ))
		strcpy( data->current_path, path + pathlen - sizeof( data->current_path ) - 1 );
	else
		strcpy( data->current_path, path );

	if ( data->find_name && *data->find_name && match == TRUE )
		match = name_match(FilePart(path), data->find_name);

	if ( data->find_comment && *data->find_comment && match == TRUE )
		match = name_match(comment, data->find_comment);

	if ( match == TRUE )
	{
		if ( data->find_type )
			match = mimetype_checkpath( data->find_type, path );
		else if ( data->find_mime && *data->find_mime )
		{
			/* lookup type and do namematching */

			match = (LONG)mimetype_find_pattern( "file://", path, MTF_FILEIO, data->find_mime ) ? TRUE : FALSE;
		}
	}

	if ( data->find_text && *data->find_text && match == TRUE )
		match = file_findtext(path, data->find_text);

	if ( match == TRUE )
		methodstack_push_sync( wo, 3, MM_Find_AddResult, path, comment);

	if ( match == ABORTED || threads_check_abort() )
	{
		return ABORTED;
	}

	return TRUE;
}

ULONG tr_find(APTR obj, APTR wo)
{
	ULONG rc = TRUE;
	struct IClass *cl = OCLASS( wo );
	struct Data *data = INST_DATA( cl, wo );
	LONG n = 0;

	/* for each location */

	while(1)
	{
		STRPTR target;

		DoMethod( data->lst_targets, MUIM_List_GetEntry, n++, &target );

		if ( target == NULL )
		{
			break;
		}

		rc = recurse(obj, target, "#?", NULL, NULL, checkfile, wo);

		if ( rc == ABORTED )
			break;
	}

	return rc;
}


BEGINMTABLE
DECNEW
DECDISP
DECGET
DECSET
DECSMETHOD(Find_AddResult)
DECTMETHOD(Find_AddLocation)
DECTMETHOD(Find_RemLocation)
DECTMETHOD(Find_Start)
DECTMETHOD(Find_Stop)
DECTMETHOD(Find_Update)
DECTMETHOD(Find_TypeSelected)
DECTMETHOD(Find_UpdateTarget)
DECTMETHOD(Window_Close)
DECSMETHOD(Thread_Finished)

ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, findwinclass)
