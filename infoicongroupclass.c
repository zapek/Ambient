/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2007 Ambient Open Source Team
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
 * $Id: infoicongroupclass.c,v 1.11 2016/08/11 11:36:54 itix Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "deficonpool.h"
#include "mui_func.h"
#include "infoicongroup.h"
#include "methodstack.h"
#include "iconio.h"
#include "threads.h"


struct Data {
	APTR  iconobj;
	APTR  iconobj_dragged;
	APTR  iconobj_dispose;
	ULONG changed;

	APTR  icongrp;
	APTR  txt_name;
};


DEFNEW
{
	APTR name, grp, iconobj = NULL;
	ULONG editable = FALSE;

	FORTAG(INITTAGS)
	{
		case MA_Infoicongroup_Child:
			iconobj = (APTR)tag->ti_Data;
			break;

		case MA_Infoicongroup_Editable:
			editable = tag->ti_Data;
			break;
	}
	NEXTTAG

	ASSERT(iconobj);

	if (editable)
	{
		ULONG imagetype = getv(iconobj, MA_Icon_ImageType);

		if (imagetype != MV_Icon_ImageType_PNGicon && imagetype != MV_Icon_ImageType_DTicon)
		{
			editable = FALSE;
		}
	}

	obj = DoSuperNew(cl, obj,
		InnerSpacing(0,0),
		//MUIA_Group_Horiz, TRUE,
		//MUIA_Background,  MUII_BACKGROUND,
		ImageButtonFrame,
		MUIA_Popstring_Button, grp = VGroup, InnerSpacing(0,0), Child, iconobj, End,
		MUIA_Popstring_String, name = StringObject, MUIA_String_MaxLen, PATH_SIZE, MUIA_String_Contents, getv(iconobj, MA_Icon_PathInfo), MUIA_ShowMe, FALSE, End,
		editable ? MUIA_InputMode : TAG_DONE, MUIV_InputMode_RelVerify,
		TAG_DONE
	);

	if (obj)
	{
		GETDATA;

		data->iconobj = iconobj;
		data->icongrp = grp;
		data->txt_name = name;

		if (editable)
		{
			DoMethod(iconobj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, obj, 3, MUIM_Set, MUIA_Selected, MUIV_TriggerValue);
			DoMethod(obj, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_Popstring_Open);
			DoMethod(name, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 3, MM_Infoicongroup_SetIcon, MUIV_TriggerValue, FALSE);
		}
		else
		{
			set(iconobj, MUIA_InputMode, MUIV_InputMode_None);
		}
	}

	return ((ULONG)obj);
}


DEFGET
{
	GETDATA; 

	switch (msg->opg_AttrID)
	{
		case MA_Infoicongroup_Child:
			{
				if (data->iconobj_dragged)
				{
					*msg->opg_Storage = (ULONG)data->iconobj_dragged;
				}
				else
				{
					*msg->opg_Storage = (ULONG)data->iconobj;
				}
			}
			return (TRUE);

		case MA_Infoicongroup_Changed:
			{
				*msg->opg_Storage = data->changed;
			}
			return (TRUE);
	}

	return (DOSUPER);
}


DEFSET
{
	GETDATA;

	FORTAG(INITTAGS)
	{
		case MA_Infoicongroup_Changed:
			{
				data->changed = tag->ti_Data;
			}
			break;
	}
	NEXTTAG

	return (DOSUPER);
}


DEFSMETHOD(Infoicongroup_AddDragIcon)
{
	GETDATA;
	STRPTR pathinfo;

	ASSERT(msg->o);
	ASSERT(data->iconobj);

	DoMethod(msg->o, MUIM_Notify, MUIA_Pressed, MUIV_EveryTime, obj, 3, MUIM_Set, MUIA_Pressed, MUIV_TriggerValue);
	DoMethod(msg->o, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, obj, 3, MUIM_Set, MUIA_Selected, MUIV_TriggerValue);

	DoMethod(obj, MUIM_Group_InitChange);

	if (data->iconobj_dragged)
	{
		DoMethod(data->icongrp, OM_REMMEMBER, data->iconobj_dragged);
		MUI_DisposeObject(data->iconobj_dragged);
	}
	else
	{
		DoMethod(data->icongrp, OM_REMMEMBER, data->iconobj);
	}

	pathinfo = (STRPTR)getv(data->iconobj, MA_Icon_PathInfo);

	set(msg->o, MA_Icon_PathInfo, pathinfo);
	DoMethod(data->txt_name, MUIM_NoNotifySet, MUIA_String_Contents, pathinfo);

	if (!data->iconobj_dragged)
	{
		data->iconobj_dispose = data->iconobj; /* put it aside */
	}

	DoMethod(data->icongrp, OM_ADDMEMBER, msg->o);
	DoMethod(obj, MUIM_Group_ExitChange);

	data->iconobj_dragged = msg->o;

	set(obj, MA_Infoicongroup_Changed, TRUE);

	return (0);
}


DEFMMETHOD(DragQuery)
{
	if (msg->obj != obj)
	{
		switch (getv(msg->obj, MA_DragDrop_Type))
		{
			case MV_DragDrop_Type_Icon:
				{
					LONG ft = getv(msg->obj, MA_Icon_FileType);

					switch (ft)
					{
						case MV_Icon_FileType_File:
						case MV_Icon_FileType_Directory:
						case MV_Icon_FileType_Device:
							{
								return (MUIV_DragQuery_Accept);
							}
							break;
					}
				}
				break;
		}
	}

	return (MUIV_DragQuery_Refuse);
}

DEFMMETHOD(DragDrop)
{
	if (msg->obj != obj)
	{
		do_action(obj, TA_Infoicon_Load,
			TT_Infoicon_Load_Path, getv(msg->obj, MA_Icon_PathInfo),
		TAG_DONE);
	}

	return (0);
}


DEFDISP
{
	GETDATA;

	if (data->iconobj_dispose)
	{
		MUI_DisposeObject(data->iconobj_dispose);
	}

	return (DOSUPER);
}


DEFSMETHOD(Infoicongroup_SetIcon)
{
	return (do_action(obj, TA_Infoicon_Load,
		TT_Infoicon_Load_Path, msg->path,
		TT_Infoicon_Load_DefIcon, msg->deficon,
	TAG_DONE));
}


BEGINMTABLE
DECNEW
DECGET
DECSET
DECDISP
DECSMETHOD(Infoicongroup_AddDragIcon)
DECSMETHOD(Infoicongroup_SetIcon)
DECMMETHOD(DragQuery)
DECMMETHOD(DragDrop)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Popasl, infoicongroupclass)


/* XXX: sigh.. no clue where to abort/wait for that thread for now */
ULONG tr_infoicon_load(APTR obj, STRPTR path, ULONG deficon)
{
	APTR o;

	ASSERT(obj);
	ASSERT(path);
	THREAD;

	if ((o = (APTR)methodstack_push_sync(app, 3, MM_Application_CreateIconinfo, FALSE, MV_ViewID_Info)))
	{
		ULONG success;
		D(ICONIO,bug("Load icon data from %s\n", path));

		if (deficon)
		{
			success = iconio_get_default_icon(path, o);
		}
		else
		{
			success = icon_read(path, o,
				ICONTAG_Ancillary, TRUE,
				ICONTAG_Deficon, TRUE,
				ICONTAG_Infowin, TRUE,
			TAG_DONE);
		}

		if (success)
		{
			methodstack_push_sync(obj, 2, MM_Infoicongroup_AddDragIcon, o);
		}
		else
		{
			MUI_DisposeObject(o);
		}
	}
	/* XXX */

	return (TRUE);
}
