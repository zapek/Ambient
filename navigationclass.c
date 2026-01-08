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
 * $Id: navigationclass.c,v 1.8 2013/10/28 20:02:39 geit Exp $
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
#include "name.h"
#include "file_func.h"
#include "storage.h"

#ifndef MUIA_Textinput_RemainActive
#define MUIA_Textinput_RemainActive 0x851b0790
#endif

struct Data {
	APTR str_path;
	APTR lv_entries;
	APTR pop_path;
	ULONG max_items;
	STRPTR history_path;
	ULONG storageid;
	ULONG disk_storage;

	struct Hook StartHook;
	struct Hook StopHook;
	STRPTR tempstr;

	struct MUI_EventHandlerNode ehnode;
};


static ULONG ASL_StartFunc(void);
static VOID ASL_StopFunc(void);

static const struct EmulLibEntry StartGate = { TRAP_LIB, 0, (APTR)&ASL_StartFunc };
static const struct EmulLibEntry StopGate = { TRAP_LIBNR, 0, ASL_StopFunc };

static ULONG ASL_StartFunc(void)
{
	struct TagItem *tags = (struct TagItem *)REG_A1;
	struct Data *data = ((struct Hook *)REG_A0)->h_Data;
	STRPTR path, file, new;
	ULONG pathlen;

	path = (STRPTR)getv(data->str_path, MUIA_String_Contents);

	file = FilePart(path);

	pathlen = (ULONG)file - (ULONG)path;

	data->tempstr = new = AllocVec(pathlen + 1, MEMF_ANY);

	if (new)
	{
		static const struct TagItem MyTags[] =
		{
			{ ASLFR_DoPatterns    , TRUE        },
			{ ASLFR_RejectIcons   , TRUE        },
			{ TAG_DONE            , NULL        }
		};

		stccpy(new, path, pathlen);

		new[pathlen] = '\0';

		while (tags->ti_Tag != TAG_DONE)
		{
			tags++;
		}

		tags[0].ti_Tag  = ASLFR_InitialFile;
		tags[0].ti_Data = (ULONG)file;
		tags[1].ti_Tag  = ASLFR_InitialDrawer;
		tags[1].ti_Data = (ULONG)new;
		tags[2].ti_Tag  = ASLFR_TitleText;
		tags[2].ti_Data = (ULONG)GSI(MSG_EXEC_ASL_TITLE);
		tags[3].ti_Tag  = TAG_MORE;
		tags[3].ti_Data = (ULONG)MyTags;
	}

	return TRUE;
}


static VOID ASL_StopFunc(void)
{
	struct FileRequester *req = (struct FileRequester *)REG_A1;
	struct Data *data = ((struct Hook *)REG_A0)->h_Data;
	STRPTR p;
	ULONG len;

	FreeVec(data->tempstr);

	len = strlen(req->fr_File) + strlen(req->fr_Drawer) + 8;

	p = AllocTaskPooled(len);

	if (p != NULL)
	{
		strcpy(p, req->fr_Drawer);
		AddPart(p, req->fr_File, len);
		set(data->str_path, MUIA_String_Contents, p);
		FreeTaskPooled(p, len);
	}
}


MUI_HOOK(popclose, APTR list, APTR str)
{
	STRPTR s;

	DoMethod(list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &s);
	set(str, MUIA_String_Contents, s);

	return (0);
}

MUI_HOOK(popopen, APTR pop, APTR win)
{
	APTR list = (APTR)getv( pop , MUIA_Listview_List );

	SetAttrs(win, MUIA_Window_DefaultObject, list, MUIA_Window_ActiveObject, list, TAG_DONE);
	set( list, MUIA_List_Active, 0 );

	return (TRUE);
}



static void doset(APTR obj UNUSED, struct Data *data, struct TagItem *tags)
{

	FORTAG(tags)
	{
		case MA_Navigation_MaxHistoryItems:
			data->max_items = tag->ti_Data;
			break;
	}
	NEXTTAG
}


DEFNEW
{
	struct Data *data;
	APTR str_path, lv_entries, pop_path, bt_pop;
	APTR bt_asl;
	APTR bt_popfile;
	ULONG show_asl;
	ULONG keep_active;

	show_asl = GetTagData( MA_Navigation_PathPopup, TRUE, INITTAGS );
	keep_active	= GetTagData( MA_Navigation_KeepActive, TRUE, INITTAGS );
	
	obj = DoSuperNew(cl, obj,
		MUIA_Group_Horiz, TRUE,
		Child, pop_path = PopobjectObject,
			MUIA_Popstring_String, str_path = StringObject,
				StringFrame,
				MUIA_String_MaxLen, PATH_SIZE,
				MUIA_Textinput_RemainActive, keep_active, /* pff, Textinput sucks */
				MUIA_CycleChain, 1,
				End,
			MUIA_Popstring_Button, bt_pop = PopButton(MUII_PopUp),
			MUIA_Popobject_Object, lv_entries = ListviewObject,
				MUIA_Listview_List, ListObject,
					InputListFrame,
					MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
					MUIA_List_DestructHook, MUIV_List_DestructHook_String,
					End,
				End,
			MUIA_Popobject_ObjStrHook, &popclose_hook,
			MUIA_Popobject_WindowHook, &popopen_hook,
		End,

		Child, bt_asl = PopaslObject,
			MUIA_Popstring_Button, bt_popfile = PopButton(MUII_PopFile),
			MUIA_Popasl_Type, ASL_FileRequest,
			MUIA_ShowMe, show_asl,
		End,

		TAG_MORE, INITTAGS
	);

	if (!obj)
	{
		return (0);
	}

	data = INST_DATA(cl, obj);
	data->str_path = str_path;
	data->pop_path = pop_path;
	data->lv_entries = lv_entries;
	data->history_path = NULL;
	data->storageid = NULL;
	data->disk_storage = FALSE;

	data->StartHook.h_Entry = (HOOKFUNC)&StartGate;
	data->StartHook.h_Data = data;
	data->StopHook.h_Entry = (HOOKFUNC)&StopGate;
	data->StopHook.h_Data = data;

	FORTAG(INITTAGS)
	{
		case MA_Navigation_MaxHistoryItems:
			data->max_items	= tag->ti_Data;
			break;

		case MA_Navigation_HistoryPath:
			data->history_path = name_build( (STRPTR)tag->ti_Data );
			break;
			
		case MA_Navigation_StorageID:
			data->storageid = tag->ti_Data;
			break;

		case MA_Navigation_DiskStorage:
			data->disk_storage = tag->ti_Data;
			break;
	}
	NEXTTAG

	SetAttrs(bt_asl,
		MUIA_Popasl_StartHook, &data->StartHook,
		MUIA_Popasl_StopHook, &data->StopHook,
		TAG_DONE
	);

	set(bt_popfile, MUIA_CycleChain, 1);
	set(bt_pop,     MUIA_CycleChain, 1);

	DoMethod(str_path, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
		app, 5, MUIM_Application_PushMethod, obj, 2, MM_Execute_Close, MV_Execute_Close_Ok
	);

	DoMethod(str_path, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
		obj, 1, MM_Execute_CheckContent
	);

	DoMethod(data->lv_entries, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE,
		pop_path, 2, MUIM_Popstring_Close, TRUE
	);

	return ((ULONG)obj);
}


DEFDISP
{
	GETDATA;

	if ( data->history_path )
		name_delete( data->history_path );

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
		case MA_Navigation_StringObject:
			*msg->opg_Storage = (ULONG)data->str_path;
			return (TRUE);

		case MA_Navigation_HistoryList:
			*msg->opg_Storage = (ULONG)data->lv_entries;
			return (TRUE);

		case MUIA_String_Contents:
			return get( data->str_path, MUIA_String_Contents, msg->opg_Storage );
	}
	return (DOSUPER);
}


DEFSMETHOD(Navigation_InsertHistory)
{
	STRPTR s, x;
	ULONG i;
	GETDATA;

	s = msg->txt;

	if ( s == NULL || *s == 0 )
		return 0;

	/*
	 * Find if the current entry is already there and remove it
	 * if so.
	 */

	for (i = 0; ; i++)
	{
		DoMethod(data->lv_entries, MUIM_List_GetEntry, i, &x);
						
		if (!x)
			break;

		if (!(stricmp(s, x))) /* XXX: we should strip spaces too.. */
		{
			DoMethod(data->lv_entries, MUIM_List_Remove, i);
			break;
		}
	}

	while (getv(data->lv_entries, MUIA_List_Entries) >= data->max_items)
	{
		DoMethod(data->lv_entries, MUIM_List_Remove, MUIV_List_Remove_Last);
	}
	DoMethod(data->lv_entries, MUIM_List_InsertSingle, s, MUIV_List_Insert_Top);

	return (0);
}

DEFMMETHOD(Setup)
{
	ULONG rc;
	GETDATA;

	if ((rc = DOSUPER))
	{
		data->ehnode.ehn_Object   = obj;
		data->ehnode.ehn_Class    = cl;
		data->ehnode.ehn_Events   = IDCMP_RAWKEY;
		data->ehnode.ehn_Priority = 1;
		data->ehnode.ehn_Flags    = MUI_EHF_GUIMODE;

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
			if ( msg->imsg->Code == 205 )
			{
				/* handle arrow down key */

				APTR active = (APTR)getv( _win(obj) , MUIA_Window_ActiveObject );

				if	( active == data->str_path )
				{
					if ( getv( data->lv_entries, MUIA_List_Entries ) )
					{
						DoMethod( data->pop_path, MUIM_Popstring_Open);
						return (MUI_EventHandlerRC_Eat);
					}
				}
			}
		}
	}

	return (0);

}

DEFTMETHOD(Navigation_LoadHistory)
{
	char **entries;
	GETDATA;

	if (data->storageid)
	{
		if (data->disk_storage)
			storage_get(data->storageid, STORAGE_STRARRAY, (APTR *)&entries);
		else
			storage_get(data->storageid, STORAGE_MSTRARRAY, (APTR *)&entries);

		if (entries)
		{
			int i;

			for (i = 0; entries[ i ] != NULL; i++)
			{
				DoMethod( obj, MM_Navigation_InsertHistory, entries[i]);
			}

			free(entries);
		}

	}

	return (TRUE);
}

DEFTMETHOD(Navigation_SaveHistory)
{
	GETDATA;

	ULONG i, ecount, x;

	get(data->lv_entries, MUIA_List_Entries, &ecount);

	if (ecount)
	{
		char *entries[ecount + 1];

		for (i = 0; i < ecount; i++)
		{
			DoMethod( data->lv_entries, MUIM_List_GetEntry, i, &x);

			entries[i] = (char *)x;
		}
	
		entries[ecount] = NULL;

		if (data->disk_storage)
		{
			storage_set(data->storageid, STORAGE_STRARRAY, entries);
		}
		else
		{
			storage_set(data->storageid, STORAGE_MSTRARRAY, entries);
		}
	}
	return (TRUE);
}

BEGINMTABLE
DECNEW
DECDISP
DECGET
DECSET
DECSMETHOD(Navigation_InsertHistory)
DECTMETHOD(Navigation_LoadHistory)
DECTMETHOD(Navigation_SaveHistory)
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECMMETHOD(HandleEvent)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, navigationclass)
