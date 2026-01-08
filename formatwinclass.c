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
 * $Id: formatwinclass.c,v 1.13 2020/08/16 03:15:18 jacadcaps Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "methodstack.h"
#include "threads.h"
#include "device_func.h"
#include "smartreq.h"
#include "screen.h"


struct Data {
	ULONG closing;
	APTR lv_devices;
	APTR grp_format;
	APTR bt_quick;
	APTR bt_format;
	APTR bt_verify;
	APTR bt_cancel;
	TEXT wintitle[64];
	ULONG formatting;
};


static void disable_format_buttons(APTR obj, struct Data *data, ULONG disable)
{
	DoMethod(obj, MUIM_MultiSet, MUIA_Disabled, disable,
		data->bt_quick,
		data->bt_format,
		data->bt_verify,
		NULL
	);
}


DEFNEW
{
	struct Data *data;
	struct TagItem *ti;
	APTR mi_quit;
	APTR bt_quick, bt_format, bt_verify, bt_cancel;
	APTR grp, grp_format;

	ti = FindTagItem(MA_Window_Path, INITTAGS);

	obj = DoSuperNew(cl, obj,
		MUIA_Window_Screen, get_screen(),
		MUIA_Window_ScreenTitle, screentitle,
		(ti && ti->ti_Data) ? MUIA_Window_LeftEdge : TAG_IGNORE, MUIV_Window_LeftEdge_Moused,
		(ti && ti->ti_Data) ? MUIA_Window_TopEdge : TAG_IGNORE, MUIV_Window_TopEdge_Moused,
		MUIA_Window_Width, MUIV_Window_Width_MinMax(10),
		MUIA_Window_Height, MUIV_Window_Width_MinMax(10),
		MUIA_Window_ShowIconify, FALSE,
		MUIA_Window_ShowPrefs, FALSE,
		MUIA_Window_ShowJump, FALSE,
		MUIA_Window_ShowAbout, FALSE,
		MUIA_Window_Menustrip, MenustripObject,
			Child, MenuObject,
				MUIA_Menu_Title, GSI( MSG_FORMAT_MENU_PROJECT ),
				Child, mi_quit = MenuitemObject,
					MUIA_Menuitem_Title,    GSI( MSG_FORMAT_MENU_QUIT ),
					MUIA_Menuitem_Shortcut, "Q",
				End,
			End,
		End,

		WindowContents, VGroup,
			Child, grp = HGroup,
				Child, grp_format = NewObject(getformatclass(), NULL, (ti && ti->ti_Data) ? MA_Window_Path : TAG_IGNORE, ti ? ti->ti_Data : NULL, TAG_DONE),
			End,
		
			Child, MUI_MakeObject(MUIO_HBar, 2),

			Child, HGroup,
				Child, bt_quick  = MUICreateButton( MSG_FORMAT_QUICK, "FORMAT_QUICK"),
				Child, bt_format = MUICreateButton( MSG_FORMAT, "FORMAT_FORMAT"),
				Child, bt_verify = MUICreateButton( MSG_FORMAT_AND_VERIFY, "FORMAT_FORMATANDVERYFY"),
				Child, bt_cancel = MUICreateButton( MSG_FORMAT_CLOSE, "FORMAT_CLOSE"),
			End,

		End,
	End;

	if (!obj)
	{
		return (NULL);
	}

	data = INST_DATA(cl, obj);
	data->grp_format = grp_format;
	data->bt_quick = bt_quick;
	data->bt_format = bt_format;
	data->bt_verify = bt_verify;
	data->bt_cancel = bt_cancel;

	/* XXX: if someone is reworking these classes, he should replace the "" by the proper device name. */

	snprintf(data->wintitle, sizeof(data->wintitle), GSI(MSG_FORMAT_WINDOW_TITLE), "");
	set(obj, MUIA_Window_Title, data->wintitle);

	/*
	 * Check if we have a path. If yes, there's
	 * no lister.
	 */
	if (ti && ti->ti_Data)
	{
		//stccpy(data->fname, (STRPTR)ti->ti_Data, strlen((STRPTR)ti->ti_Data));
		//set(data->str_name, MUIA_String_Contents, data->fname);
		//strcat(data->fname, ":");
	}
	else
	{
		data->lv_devices = ListviewObject,
			MUIA_CycleChain, 1,
			MUIA_Weight, 0,
			MUIA_Listview_List, NewObject(getformatlistclass(), NULL, TAG_DONE),
		End;

		if (!data->lv_devices)
		{
			CoerceMethod(cl, obj, OM_RELEASE);
			return (NULL);
		}
		DoMethod(grp, OM_ADDMEMBER, data->lv_devices);
		DoMethod(grp, MUIM_Group_Sort, data->lv_devices, grp_format, NULL);
		
		DoMethod(data->lv_devices, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
			obj, 1, MM_Formatwin_SetDevice
		);
		
		set(data->lv_devices, MUIA_List_Active, 0); /* XXX: fail if there's no devices.. who knows :) */
	}

	DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
		app, 4, MUIM_Application_PushMethod, obj, 1, MM_Formatwin_Close
	);

	DoMethod(bt_cancel, MUIM_Notify, MUIA_Pressed, FALSE,
		app, 4, MUIM_Application_PushMethod, obj, 1, MM_Formatwin_Cancel
	);

	DoMethod(bt_quick, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_Formatwin_Format, MV_Format_Format_Quick
	);

	DoMethod(bt_format, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_Formatwin_Format, MV_Format_Format_Format
	);

	DoMethod(bt_verify, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_Formatwin_Format, MV_Format_Format_Verify
	);

/* menu item notifies */

	DoMethod(mi_quit, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, app, 4, MUIM_Application_PushMethod, obj, 1, MM_Formatwin_Close);

	return ((ULONG)obj);
}


DEFGET
{
	//GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_Window_ID:
			*msg->opg_Storage = 0;
			return (TRUE);

		case MA_Window_Path:
			//*msg->opg_Storage = (ULONG)data->fname; /* XXX: that fname should change dynamically.. */
			/* XXX */
			return (TRUE);

		case MA_Window_Type:
			*msg->opg_Storage = MV_Window_Type_Format;
			return (TRUE);
	}
	return (DOSUPER);
}


DEFTMETHOD(Formatwin_SetDevice)
{
	struct device_info *di;
	GETDATA;

	if (data->lv_devices)
	{
		DoMethod(data->lv_devices, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &di);
	}

	snprintf(data->wintitle, 256, GSI(MSG_FORMAT_WINDOW_TITLE), di->name);
	set(obj, MUIA_Window_Title, data->wintitle);

	DoMethod(data->grp_format, MUIM_Group_InitChange); /* this is needed because MUI has difficulties handling this group change from the notify (MUI sucks) */
	DoMethod(data->grp_format, MM_Format_ChangeOptions, di);
	DoMethod(data->grp_format, MUIM_Group_ExitChange);

	disable_format_buttons(obj, data, !getv(data->grp_format, MA_Format_Formattable));

	return (0);
}


DEFTMETHOD(Formatwin_Cancel)
{
	GETDATA;

	if (data->formatting)
	{
		threads_abort(obj, NULL);
	}
	else
	{
		DoMethod(obj, MM_Formatwin_Close);
	}
	return (0);
}


DEFTMETHOD(Formatwin_Close)
{
	GETDATA;

	set(obj, MUIA_Window_Open, FALSE);
	data->closing = TRUE;
	threads_abort(obj, NULL);
	return (0);
}


DEFSMETHOD(Thread_Finished)
{
	GETDATA;

	data->formatting = FALSE;

	if (data->closing)
	{
		DoMethod(app, OM_REMMEMBER, obj);
		MUI_DisposeObject(obj);
	}
	else
	{
		DoMethod(data->grp_format, MM_Format_RemoveGauge);

		DoMethod(data->grp_format, MM_Format_Busy, FALSE);

		if (data->lv_devices)
		{
			set(data->lv_devices, MUIA_Disabled, FALSE);
		
			//DoMethod(obj, MM_Formatwin_SetDevice); /* XXX: that doesn't work.. I think we'd have to wait a moment for the device to validate.. hm, IDCMP_DISKINSERTED is the correct place anyway */
		}
		disable_format_buttons(obj, data, FALSE);
		set(data->bt_cancel, MUIA_Text_Contents, GSI(MSG_CLOSE));
	}
	return (0);
}


DEFTMETHOD(Formatwin_SetText)
{
	GETDATA;

	return (DoMethodA(data->grp_format, msg)); /* forwarder */
}


DEFTMETHOD(Formatwin_SetGauge)
{
	GETDATA;

	return (DoMethodA(data->grp_format, msg)); /* forwarder */
}


DEFSMETHOD(Formatwin_Format)
{
	GETDATA;
	struct device_info *di;

	di = (APTR)getv(data->grp_format, MA_Format_DeviceInfo);
	
	set(obj, MUIA_Window_Sleep, TRUE);

	switch ((LONG)getv(data->grp_format, MA_Format_NameStatus))
	{
		case MV_Format_NameStatus_Reserved:
			smartreq_request(obj, obj, GSI(MSG_FORMAT_ERROR), MM_Formatwin_FormatReally, (LONG)msg->mode, GSI(MSG_OK), MV_Notification_Error, GSI(MSG_FORMAT_ERROR_RESERVED), NULL);
			break;

		case MV_Format_NameStatus_Empty:
			smartreq_request(obj, obj, GSI(MSG_FORMAT_ERROR), MM_Formatwin_FormatReally, (LONG)msg->mode, GSI(MSG_OK), MV_Notification_Error, GSI(MSG_FORMAT_ERROR_NOTHING), NULL);
			break;

		case MV_Format_NameStatus_Valid:
			smartreq_request(obj, obj, GSI(MSG_FORMAT_FORMATING), MM_Formatwin_FormatReally, (LONG)msg->mode, GSI(MSG_YESNO), MV_Notification_Warning, GSI(MSG_FORMAT_REALLY), di->name);
			break;

		#ifdef DEBUG
		default:
			PDB(("out of bounds\n"));
			break;
		#endif
	}
	return (0);
}


DEFSMETHOD(Formatwin_FormatReally)
{
	GETDATA;

	set(obj, MUIA_Window_Sleep, FALSE);

	if (msg->butnum)
	{
		if (data->lv_devices)
		{
			set(data->lv_devices, MUIA_Disabled, TRUE);
		}

		disable_format_buttons(obj, data, TRUE);
		set(data->bt_cancel, MUIA_Text_Contents, GSI(MSG_CANCEL));

		DoMethod(data->grp_format, MM_Format_Busy, TRUE);

		DoMethod(data->grp_format, MUIM_Group_InitChange); /* MUI suckage again */
	
		DoMethod(data->grp_format, MM_Format_Format, msg->userdata);

		DoMethod(data->grp_format, MUIM_Group_ExitChange);

		data->formatting = TRUE;
	}
	return (0); /* XXX */
}


BEGINMTABLE
DECNEW
DECGET
DECTMETHOD(Formatwin_SetDevice)
DECTMETHOD(Formatwin_Cancel)
DECTMETHOD(Formatwin_Close)
DECSMETHOD(Thread_Finished)
DECTMETHOD(Formatwin_SetText)
DECTMETHOD(Formatwin_SetGauge)
DECSMETHOD(Formatwin_Format)
DECSMETHOD(Formatwin_FormatReally)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, formatwinclass)
