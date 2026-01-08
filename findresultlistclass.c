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
 * $Id: findresultlistclass.c,v 1.14 2021/12/31 17:52:18 piru Exp $
 */

#include "ambient.h"

/* public */
#include <dos/dosextens.h>
#include <proto/dos.h>

/* private */
#include "mui_func.h"
#include "command.h"
#include "rexx.h"
#include "contextmenu.h"
#include "mimetype.h"
#include "actiondispatcherclass.h"
#include "action.h"
#include "typescanner.h"
#include "file_func.h"
#include "methodstack.h"
#include "threads.h"
#include "mimeuri.h"
#include "ambient_cat.h"
#include "findclass.h" // for struct item

#define BUF_TITLE_SIZE 0x30

enum { COL_NAME=0, COL_PATH, COL_COMMENT };

struct Data {
	ULONG sort_col;
	LONG sort_direction;
	APTR mimetype;
	APTR cmenu;
	TEXT buf_title[BUF_TITLE_SIZE];  /* used for variable name + escaping   */
};

/* moved to findclass.h and renamed to "result_item"
struct item {
	STRPTR filepart;
	STRPTR comment;
	TEXT text[ 0 ];
};
*/


static void doset(APTR obj, struct Data *data, struct TagItem *tags)
{
	FORTAG(tags)
	{
		case MUIA_List_TitleClick:
		{
			if(data->sort_col == tag->ti_Data)
				data->sort_direction *= -1;
			else
				data->sort_col = tag->ti_Data;

			DoMethod(obj, MUIM_List_Sort);
			break;
		}

		case MA_Icon_MimeType:
		{
			/* ok, cheating here. will fix laters. */

			data->mimetype = (APTR)tag->ti_Data;
			break;
		}

		case MUIA_List_DoubleClick:
		{
			struct result_item *item;
			DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &item);

			if (item)
			{
				APTR dispatcher = (APTR)DoMethod(app, MM_Application_CreateActionDispatcher);

				if (dispatcher)
				{
					TEXT path[PATH_SIZE];

					stccpy(path, item->text, sizeof(path));
					AddPart(path, item->filepart, sizeof(path));

					SetAttrs(dispatcher,
						MA_ActionDispatcher_Event, ACTION_EVENT_DOUBLECLICK,
						MA_ActionDispatcher_SrcID, getv(_win( obj ), MA_Window_ID),
						MA_ActionDispatcher_RefWin, _win(obj),
						TAG_DONE
					);

					DoMethod(dispatcher, MM_ActionDispatcher_AddURI, path, TRUE);
					DoMethod(dispatcher, MM_ActionDispatcher_Execute);
				}
			}

			break;
		}
	}
	NEXTTAG
}


DEFNEW
{
	obj = DoSuperNew(cl, obj,
		MUIA_List_Format, "C=0 MIW=-1 BAR,C=1 MIW=-1 BAR, C=2 MIW=-1",
		MUIA_List_Title, TRUE,
		InputListFrame,
		MUIA_CycleChain, TRUE,
		MUIA_ContextMenu, 1,
		TAG_MORE, INITTAGS
	);

	if (obj)
	{
		GETDATA;

		memset(data, 0, sizeof(struct Data));

		data->sort_direction = 1;
	}

	return (ULONG)obj;
}

DEFMMETHOD(List_Construct)
{
	struct result_item *entry = msg->entry;
	STRPTR path = entry->filepart;
	struct result_item *item = AllocVecPooled(msg->pool, sizeof( struct result_item ) + strlen( path ) + 2);

	if (item)
	{
		if (entry->comment && entry->comment[0])
		{
			if ( (item->comment = AllocVecPooled(msg->pool, strlen(entry->comment)+1)) ) strcpy(item->comment, entry->comment);
			// TODO?: else item->comment = "Not enough memory!"; ???
		}else item->comment = NULL;
	
		strcpy(item->text, path);
		item->filepart = FilePart(item->text);

		if (item->filepart)
		{
			memmove(item->filepart + 1, item->filepart, strlen( item->filepart ) + 1);
			item->text[ (ULONG)item->filepart - (ULONG)item->text ] = 0; // bitRocky: GCC5 complained, so I added "(ULONG)"s
			item->filepart++;
		}
		else
		{
			item->filepart = item->text;
		}
	}

	return (ULONG)item;
}

DEFMMETHOD(List_Destruct)
{
	struct result_item *item = msg->entry;
	if (item){
		FreeVecPooled(msg->pool, item->comment);
		FreeVecPooled(msg->pool, item);
	}

	return 0;
}

static LONG compareEntries( struct Data * data UNUSED, struct result_item *e1, struct result_item *e2, LONG col )
{
	LONG rc = 0;

	if (col == COL_NAME)
	{
		rc = stricmp(e1->filepart, e2->filepart);
	}
	else if (col == COL_PATH)
	{
		rc = stricmp(e1->text, e2->text);
		if (rc == 0)
			rc = stricmp(e1->filepart, e2->filepart);
	}
	else if (col == COL_COMMENT)
	{
		rc = stricmp(e1->comment, e2->comment);
	}

	return rc;
}

DEFMMETHOD(List_Compare)
{
	GETDATA;

	struct result_item *e1 = (struct result_item *)msg->entry1;
	struct result_item *e2 = (struct result_item *)msg->entry2;

	/* get clicked col and reverse state */
	LONG col = data->sort_col;
	LONG rev = data->sort_direction;

	LONG result;

	/* compare for each column */

	result = compareEntries(data, e1 , e2, col);
	result *= rev;

	return result;
}

DEFMMETHOD(List_Display)
{
	GETDATA;

	struct result_item *item = (struct result_item*)msg->entry;

	if (!item)
	{
		snprintf( data->buf_title, BUF_TITLE_SIZE, "\033I[6:%s] %s", (data->sort_direction > 0) ? "38" : "39", GSI( MSG_FINDRESULTLISTCLASS_NAME + data->sort_col ) );

		msg->array[ COL_NAME ] = (data->sort_col==COL_NAME) ? (STRPTR)data->buf_title : GSI( MSG_FINDRESULTLISTCLASS_NAME );
		msg->array[ COL_PATH ] = (data->sort_col==COL_PATH) ? (STRPTR)data->buf_title : GSI( MSG_FINDRESULTLISTCLASS_CONTAINER );
		msg->array[ COL_COMMENT ] = (data->sort_col==COL_COMMENT) ? (STRPTR)data->buf_title : GSI( MSG_FINDRESULTLISTCLASS_COMMENT );
	}
	else
	{
		msg->array[ COL_NAME ] = item->filepart;
		msg->array[ COL_PATH ] = item->text;
		msg->array[ COL_COMMENT ] = item->comment;

		if ((ULONG)msg->array[ -1 ] % 2)
			msg->array[ -9 ] = (STRPTR)10;
	}

	return 0;
}

DEFSET
{
	GETDATA;

	doset(obj, data, INITTAGS);
	return (DOSUPER);
}

DEFMMETHOD(ContextMenuBuild)
{
	GETDATA;

	struct MUI_List_TestPos_Result res;

	DoMethod(obj, MUIM_List_TestPos, msg->mx, msg->my, &res);

	if (data->cmenu)
	{
		MUI_DisposeObject(data->cmenu);
		data->cmenu = NULL;
	}

	if(res.entry == -1)
		return (ULONG)NULL;

	{
		struct result_item *item;

		DoMethod(obj, MUIM_List_GetEntry, res.entry, (ULONG *)&item);

		set(obj, MUIA_List_Active, res.entry);

		if(item)
		{
			if ((data->cmenu = contextmenu_build(CM_LIST, 0)))
			{
				TEXT path[PATH_SIZE];
				APTR mimetype = NULL;

				/*
				 * Add type dependant actions to context menu.
				 */

				stccpy(path, item->text, sizeof(path));
				if (res.column == 0)
					AddPart(path, item->filepart, sizeof(path));

				if (mimetype == NULL)
				{
					/*
					 * Needs to be found. This is a bit problematic as it should be done on
					 * a thread but for now we do this workaround.
					 */

					LONG cnt = 0;
					Object *mimeTypeObject = DoMethod(NewObject(getmimetypeclass(), NULL, TAG_DONE), OM_RETAIN);
					
					if (do_action(obj, TA_MimeType_Scan,
									TT_MimeType_Scan_Path, path,
									TT_MimeType_Scan_MimetypeObject, mimeTypeObject,
								TAG_DONE))
					{
						/*
						 * We give it max 1s to find the type.
						 */

						while( cnt < 25 && !xget(mimeTypeObject, MA_Mimetype_TypeResolved) )
						{
							Delay(2);
							cnt++;
							methodstack_check(FALSE);
						}
					}

					mimetype = xget(mimeTypeObject, MA_Mimetype_Type);
					DoMethod(mimeTypeObject, OM_RELEASE);
				}

				if (mimetype)
				{
					TEXT buf[ 1024 ];
					STRPTR mode = "List";

					{
						UBYTE encpath[mimeuri_encodepath(NULL, 0, path)];

						mimeuri_encodepath(encpath, sizeof(encpath), path);

						/* XXX: if it's a directory, read icon window coords and pass them, or use some listview preferences coords ? */
						snprintf(buf, sizeof(buf), "file:///%s?view=%s&mode=all",
							encpath,
							mode );
					}

					contextmenu_add_mime(data->cmenu, buf, mimetype);
				}

				return ((ULONG)data->cmenu);
			}
		}
	}

	return (ULONG)NULL;
}

DEFMMETHOD(ContextMenuChoice)
{
	struct command_menu *cm;

	if ((cm = (struct command_menu *)getv(msg->item, MA_Menuitem_Command)))
	{
		if (cm->name && *cm->name)
		{
			execute_command(obj, cm->type, cm->args, NULL); /* XXX: I think.. well no.. we need the object */
		}

		if (cm->actionnode)
		{
			/*
			 * This is action we selected. Needs to be parsed.
			 */

			APTR dispatcher = (APTR)DoMethod( app, MM_Application_CreateActionDispatcher );

			if ( dispatcher )
			{
				SetAttrs( dispatcher,
					MA_ActionDispatcher_SrcID, getv(_win(obj), MA_Window_ID),
					MA_ActionDispatcher_RefWin, _win(obj),
					MA_ActionDispatcher_Action, cm->actionnode,
					TAG_DONE
				);

				DoMethod(dispatcher, MM_ActionDispatcher_AddURI, cm->args, TRUE);
				DoMethod(dispatcher, MM_ActionDispatcher_Execute);
			}
		}
	}
	return (0);
}


BEGINMTABLE
DECNEW
DECSET
DECMMETHOD(List_Compare)
DECMMETHOD(List_Display)
DECMMETHOD(List_Construct)
DECMMETHOD(List_Destruct)
DECMMETHOD(ContextMenuBuild)
DECMMETHOD(ContextMenuChoice)
ENDMTABLE

DECSUBCLASS_NC(MUIC_List, findresultlistclass)
