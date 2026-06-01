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
 * $Id: executewinclass.c,v 1.15 2025/09/16 13:52:45 jacadcaps Exp $
 */

#include "ambient.h"

/* public */
#include <libraries/asl.h>
#include <proto/openurl.h>
#include <dos/dostags.h>
#include <proto/dos.h>

/* private */
#include "mui_func.h"
#include "ambient_cat.h"
#include "file_func.h"
#include "prefs.h"
#include "smartreq.h"
#include "screen.h"
#include "rexx.h"
#include "command.h"
#include "storage.h"

#define MAXENTRIES 10
#define AMBIENT_EXEC_HISTORY "ENVARC:Sys/AmbientExecHistory"

APTR executewin; /* XXX: hell, remove that.. put in appclass.c */

struct Data {
	APTR pop_command;
	APTR str_command;
	APTR pop_list;
	APTR menu;
	APTR bt_ok;
	APTR bt_cancel;
};

DEFNEW
{
	struct Data *data;
	APTR bt_ok, bt_cancel, pop_command, menClearAct, menClearAll;

	obj = DoSuperNew(cl, obj,
		MUIA_Window_ScreenTitle, screentitle,
		MUIA_Window_ID, MAKE_ID('E','X','E','C'),
		MUIA_Window_Title, GSI(MSG_EXEC_TITLE),
		MUIA_Window_ShowPrefs, FALSE,
		MUIA_Window_ShowJump, FALSE,
		MUIA_Window_ShowAbout, FALSE,
		MUIA_Window_ShowIconify, FALSE,
		WindowContents, VGroup,
			Child, text(MSG_EXEC_INFO),

			Child, pop_command = NewObject(getnavigationclass(), NULL,
									MA_Navigation_MaxHistoryItems, MAXENTRIES,
									MA_Navigation_HistoryPath, AMBIENT_EXEC_HISTORY,
									MA_Navigation_StorageID, STORAGE_EXEC_HISTORY,
									MA_Navigation_DiskStorage, TRUE,
									MUIA_ShortHelp, GSI(MSG_EXEC_COMMAND_HELP),
									TAG_DONE),

			Child, HGroup,
				MUIA_Group_SameWidth, TRUE,
				Child, bt_ok = MUICreateButton(MSG_EXEC_EXECUTE, "EXECUTE_EXECUTE"),
				Child, HSpace(0),
				Child, bt_cancel = MUICreateButton(MSG_EXEC_CANCEL, "EXECUTE_CANCEL"),
			End,
		End,
	End;

	if (!obj)
	{
		return (0);
	}

	data = INST_DATA(cl, obj);

	data->pop_command = pop_command;
	data->str_command = (APTR)getv(pop_command, MA_Navigation_StringObject);

	data->bt_ok       = bt_ok;
	data->bt_cancel   = bt_cancel;

	data->menu = MenustripObject,
		Child, MenuObject,
			MUIA_Menu_Title, GSI(MSG_EXEC_CMENU_HISTORY_TITLE),
			Child, menClearAct = MenuitemObject,
				MUIA_Menuitem_Title, GSI(MSG_EXEC_CMENU_HISTORY_CLEARACTIVE),
			End,
			Child, menClearAll = MenuitemObject,
				MUIA_Menuitem_Title, GSI(MSG_EXEC_CMENU_HISTORY_CLEARALL),
			End,
		End,
	End;

	set(data->pop_command, MUIA_ContextMenu, data->menu);
	data->pop_list = (APTR)getv(data->pop_command, MA_Navigation_HistoryList);
	DoMethod(data->pop_command, MUIM_Notify, MUIA_ContextMenuTrigger, menClearAll, data->pop_list, 1, MUIM_List_Clear);
	DoMethod(data->pop_command, MUIM_Notify, MUIA_ContextMenuTrigger, menClearAll, data->pop_command, 1, MM_Navigation_SaveHistory);

	DoMethod(data->pop_command, MUIM_Notify, MUIA_ContextMenuTrigger, menClearAct, data->pop_list, 2, MUIM_List_Remove, MUIV_List_Remove_Active);
	DoMethod(data->pop_command, MUIM_Notify, MUIA_ContextMenuTrigger, menClearAct, data->pop_command, 1, MM_Navigation_SaveHistory);

	DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
		app, 5, MUIM_Application_PushMethod, obj, 2, MM_Execute_Close, MV_Execute_Close_Cancel
	);

	DoMethod(data->bt_cancel, MUIM_Notify, MUIA_Pressed, FALSE,
		app, 5, MUIM_Application_PushMethod, obj, 2, MM_Execute_Close, MV_Execute_Close_Cancel
	);

	DoMethod(data->bt_ok, MUIM_Notify, MUIA_Pressed, FALSE,
		app, 5, MUIM_Application_PushMethod, obj, 2, MM_Execute_Close, MV_Execute_Close_Ok
	);

	DoMethod(data->str_command, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
		app, 5, MUIM_Application_PushMethod, obj, 2, MM_Execute_Close, MV_Execute_Close_Ok
	);

	DoMethod(data->str_command, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
		obj, 1, MM_Execute_CheckContent
	);

	set(obj, MUIA_Window_ActiveObject, data->str_command);
	DoMethod(obj, MM_Execute_CheckContent);

	// We dont need asyncio here

	DoMethod( data->pop_command, MM_Navigation_LoadHistory );

	return ((ULONG)obj);
}


DEFGET
{
	switch (msg->opg_AttrID)
	{
		case MA_Window_ID:
			*msg->opg_Storage = 0;
			return (TRUE);

		case MA_Window_Path:
			*msg->opg_Storage = MV_Window_Path_None;
			return (TRUE);

		case MA_Window_Type:
			*msg->opg_Storage = MV_Window_Type_Execute;
			return (TRUE);
	}
	return (DOSUPER);
}


DEFSMETHOD(Execute_Close)
{
	GETDATA;
	BOOL add=FALSE; // add to navigation history or not

	set(obj, MUIA_Window_Open, FALSE);

	if (msg->val == MV_Execute_Close_Ok)
	{
		STRPTR s;

		s = (STRPTR)getv(data->str_command, MUIA_String_Contents);

		if (s && *s)
		{
			if (*s == '\'')
			{
				/*
				 * Internal command.
				 */
				execute_command(NULL, AC_INTERNAL, s+1, NULL); /* XXX: hm, couldn't there be something useful for the 'obj' ? */
				add = TRUE;
			}
			else if (!strnicmp(s, "http://", 7) ||
				!strnicmp(s, "ftp://", 6) ||
				!strnicmp(s, "gopher://", 9) ||
				!strnicmp(s, "https://", 8) ||
				!strnicmp(s, "telnet://", 9) ||
				!strnicmp(s, "irc://", 6) ||
				!strnicmp(s, "news://", 7) ||
				!strnicmp(s, "mailto:", 7) ||
				!strnicmp(s, "ftp.", 4) ||
				!strnicmp(s, "www.", 4)) 
			{
				/*
				 * Is it an URL ? This could be done in a more sophisticated way but who cares..
				 */
				if (OpenURLBase)
				{
					URL_OpenA(s, NULL);
					add = TRUE;
				}
			}
			else
			{
				#if 0
				STRPTR uri;
				ULONG path_built = FALSE;
				APTR ctx;

				/*
				 * XXX: we need to find the path and build a full URI,
				 * eg. dir -> file:///MOSSYS:C/dir or so
				 * so basically.. mimeuri_hasscheme() or so.. then path_build/path_free, etc..
				 * XXX: no no.. path_build cannot be used here.. mimeuri_hasscheme() is futile too..
				 * just add a new tag to TT_URI_Load..
				 */
				if (mimeuri_hasscheme(s))
				{
					uri = s;
				}
				else
				{
					uri = path_build(s);
					path_built = TRUE;
				}

				if (uri && (ctx = mimeuri_create()))
				{

					if (do_action(app, TA_URI_Load,
						TT_URI_Load_URI, uri,
						TT_URI_Load_Ctx, ctx,
						TT_URI_Load_Newwin, TRUE,
					TAG_DONE))
					{
						DoMethod(data->pop_command, MM_Navigation_InsertHistory, s);
					}
					else
					{
						mimeuri_delete(ctx);
					}

				}

				if (uri && path_built)
				{
					path_free(uri);
				}

				#else
				/* XXX: all that stuff should use wbstart.. */
				BPTR output;

				if (!_conf(cli_device) || !_conf(cli_device)[0] || !(output = Open(_conf(cli_device), MODE_NEWFILE)))
				{
					output = Open("NIL:", MODE_NEWFILE); /* XXX: erk, close input and output if we fail later.. */
				}

				if (output)
				{
					BPTR input;
					struct MsgPort *old;

					old = SetConsoleTask(((struct FileHandle *)BADDR(output))->fh_Type);
					input = Open("*", MODE_OLDFILE); /* XXX: check retcode.. */
					SetConsoleTask(old);

					if (input)
					{

						/* XXX: hm, perhaps we should just use execute_async().. */
						systemtags(s,
							SYS_Asynch, TRUE,
							SYS_Input, input,
							SYS_Output, output,
							NP_StackSize, _conf(cli_stack),
							NP_PPCStackSize, _conf(cli_stack) * 2,
							NP_Priority, 0, /* XXX: make that configurable ? */
							TAG_DONE
						);
						add = TRUE;
					}
					else
					{
						smartreq_info( GSI(MSG_EXEC_ERROR_TITLE), MV_Notification_Error, GSI(MSG_EXEC_ERROR_OPENINPUTSTREAMFAILED), NULL);
					}
				}
				else
				{
					smartreq_info( GSI(MSG_EXEC_ERROR_TITLE), MV_Notification_Error, GSI(MSG_EXEC_ERROR_OPENOUTPUTSTREAMFAILED), NULL);
				}
				#endif
			}
		}
		if (add)
		{
			/* Add entry to navigation history list. */
			DoMethod( data->pop_command, MM_Navigation_InsertHistory, s );
			DoMethod( data->pop_command, MM_Navigation_SaveHistory );
		}
	}
	else
	{
		set(data->str_command, MUIA_String_Contents, "");
	}

	set(obj, MUIA_Window_ActiveObject, data->str_command);

	/*
	 * We don't destroy it for now. Might change in the future,
	 * I don't know..
	 */

	return (0);
}


DEFTMETHOD(Execute_CheckContent)
{
	GETDATA;
	STRPTR s;

	s = (STRPTR)getv(data->str_command, MUIA_String_Contents);

	set(data->bt_ok, MUIA_Disabled, s && *s ? FALSE : TRUE);
	return (0);
}

BEGINMTABLE
DECNEW
DECGET
DECSMETHOD(Execute_Close)
DECTMETHOD(Execute_CheckContent)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, executeclass)
