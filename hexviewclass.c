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
 * $Id: hexviewclass.c,v 1.1 2022/07/23 02:20:35 jacadcaps Exp $
 */

#include "ambient.h"


#if USE_VIEW_HEX

#include "mui_func.h"
#include "storage.h"
#include "rexx.h"
#include "file_io.h"
#include <mui/Hex_mcc.h>
#include <proto/dos.h>

struct Data {
	APTR hscroller;
	APTR vscroller;
	Object *hex;
};

struct windowpos {
	LONG *left, *top;
	LONG *width, *height;
};

static void doset(APTR obj UNUSED, struct Data *data, struct TagItem *tags)
{
	FORTAG(tags)
	{
		case MA_Viewgroup_HScroller:
			data->hscroller = (APTR)tag->ti_Data;
			break;

		case MA_Viewgroup_VScroller:
			data->vscroller = (APTR)tag->ti_Data;
			break;
	}
	NEXTTAG
}

DEFNEW
{
	struct Data *data;
	APTR hex, sGroup;

	obj = DoSuperNew(cl, obj,
		Child, hex = HexObject,
			InnerSpacing(2,2),                     /* force some space between window boarder and text */
			MUIA_Frame, MUIV_Frame_None,           /* no frame required */ 
			MUIA_Background, MUII_TextBack,
		End,
//		Child, sGroup = NewObject(getsearchbarclass(), NULL, MUIA_ShowMe, FALSE, TAG_DONE),
		TAG_MORE, INITTAGS
	);

	if (!obj)
	{
		return ((ULONG)NULL);
	}

	data = INST_DATA(cl, obj);
	data->hex = hex;
	data->hscroller = NULL;
	data->vscroller = NULL;

	doset(obj, data, INITTAGS);

	if ( !data->hscroller )
		data->hscroller = (APTR)getv(obj, MUIA_Scrollgroup_HorizBar);
	if ( !data->vscroller )
		data->vscroller = (APTR)getv(obj, MUIA_Scrollgroup_VertBar);

//	set(data->searchGroup, MA_Searchbar_Target, obj);

//	DoMethod(data->vscroller, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, data->text,3,MUIM_Set,MUIA_Textinput_TopOffset,MUIV_TriggerValue);
//	DoMethod(data->hscroller, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, data->text,3,MUIM_Set,MUIA_Textinput_LeftOffset,MUIV_TriggerValue);

	DoMethod(hex, MUIM_Notify, MUIA_Hex_Rows, MUIV_EveryTime, data->vscroller, 3, MUIM_Set, MUIA_Prop_Entries, MUIV_TriggerValue);
	DoMethod(hex, MUIM_Notify, MUIA_Hex_VisibleRows, MUIV_EveryTime, data->vscroller, 3, MUIM_Set, MUIA_Prop_Visible, MUIV_TriggerValue);
	DoMethod(hex, MUIM_Notify, MUIA_Hex_FirstRow, MUIV_EveryTime, data->vscroller, 3, MUIM_NoNotifySet, MUIA_Prop_First, MUIV_TriggerValue);
	DoMethod(data->vscroller, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, hex, 3, MUIM_NoNotifySet, MUIA_Hex_FirstRow, MUIV_TriggerValue);

	return ((ULONG)obj);
}

DEFGET
{
	switch (msg->opg_AttrID)
	{
		case MA_View_NeedsBackfill:
			*msg->opg_Storage = TRUE;	/* XXX: Just for now until i figure how to change it on the fly */
			return (TRUE);

		case MA_View_HasBackground:
			*msg->opg_Storage = FALSE;
			return (TRUE);

		case MA_View_NewWin:
			*msg->opg_Storage = FALSE;//TRUE;
			return (TRUE);

		case MA_View_Type:
			*msg->opg_Storage = MV_View_Type_Text;
			return (TRUE);

		case MA_View_ShowDevices:
			*msg->opg_Storage = FALSE;
			return (TRUE);
	}
	return (DOSUPER);
}

DEFSET
{
	GETDATA;

	doset(obj, data, INITTAGS);

	return (DOSUPER);
}

DEFTMETHOD(View_LoadURI)
{
	GETDATA;

	DoSuperMethod(cl, obj, MM_View_SetStatus, MF_View_SetStatus_Window, "%s", FilePart((STRPTR)getv(obj, MA_View_Path)));
	DoMethod(data->hex, MUIM_Hex_Load, getv(obj, MA_View_Path));

	return (0);
}

DEFMMETHOD(Setup)
{
	ULONG rc;

	if ((rc = DOSUPER))
	{
		GETDATA;

        /* initial window position */

        {
			STRPTR buff = NULL;

			storage_get(STORAGE_HEXVIEW_SNAPSHOT, STORAGE_STRING, (APTR*)&buff);
			if (buff != NULL)
			{
				struct windowpos wp;
				struct RDArgs *rda;

				memset(&wp, 0, sizeof(wp));
				rda	= readargsstring(buff, "LEFT/N,TOP/N,WIDTH/N,HEIGHT/N", (ULONG*)&wp);

				if (rda != NULL)
				{
					if (wp.left != 0 && wp.top != NULL)
					{
						set(_win(obj), MUIA_Window_LeftEdge, *wp.left);
						set(_win(obj), MUIA_Window_TopEdge, *wp.top);
					}

					freeargsstring(rda);
				}
			}
		}

	}
	return (rc);
}

DEFMMETHOD(AskMinMax)
{
    STRPTR buff = NULL;

	DOSUPER;

	storage_get(STORAGE_HEXVIEW_SNAPSHOT, STORAGE_STRING, (APTR*)&buff);
	if (buff != NULL)
	{
		struct windowpos wp;
		struct RDArgs *rda;

		memset(&wp, 0, sizeof(wp));
		rda	= readargsstring(buff, "LEFT/N,TOP/N,WIDTH/N,HEIGHT/N", (ULONG*)&wp);

		if (rda != NULL)
		{
			msg->MinMaxInfo->DefWidth = *wp.width != 0 ? *wp.width : 640;
			msg->MinMaxInfo->DefHeight = *wp.height != 0 ? *wp.height : 480;

			freeargsstring(rda);
		}

	}
	else
	{
		msg->MinMaxInfo->DefWidth = 640;
		msg->MinMaxInfo->DefHeight = 480;
	}

	return (0);
}

DEFSMETHOD(Rexx_Snapshot)
{
	LONG left, top;
	LONG width, height;
	TEXT buff[128];

	left = getv(_win(obj), MUIA_Window_LeftEdge);
	top	= getv(_win(obj), MUIA_Window_TopEdge);
	width = _width(obj);
	height = _height(obj);

	snprintf(buff, sizeof(buff), "%ld %ld %ld %ld", left, top, width, height);
	storage_set(STORAGE_TEXTVIEW_SNAPSHOT, STORAGE_STRING, buff);

	return (0);
}

DEFSMETHOD(Rexx_Unsnapshot)
{
	/* XXX: Note to self: add storage_remove() */
	storage_set(STORAGE_TEXTVIEW_SNAPSHOT, STORAGE_STRING, "");
	return (DOSUPER);
}

BEGINMTABLE
DECNEW
DECGET
DECSET
DECMMETHOD(Setup)
DECMMETHOD(AskMinMax)
DECTMETHOD(View_LoadURI)
DECSMETHOD(Rexx_Snapshot)
DECSMETHOD(Rexx_Unsnapshot)
ENDMTABLE

DECSUBCLASSPTR_NC(viewclass, hexviewclass)

#endif
