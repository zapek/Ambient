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
 * $Id: editstringclass.c,v 1.4 2017/08/21 06:17:44 cyfm Exp $
 */

#include "ambient.h"

/* public */
#include <intuition/gadgetclass.h>

/* private */
#include "mui_func.h"

struct Data {
	LONG left;
	LONG top;
	LONG width;
	APTR reportobj;
	#if 0
	struct MUI_EventHandlerNode ehnode;
	#endif
};


DEFNEW
{
	struct Data *data;

	obj = DoSuperNew(cl, obj,
		MUIA_Floating, TRUE,
		MUIA_Frame, "110000",
		TAG_MORE, INITTAGS
	);

	if (!obj)
	{
		return ((ULONG)NULL);
	}

	data = INST_DATA(cl, obj);
	
	FORTAG(INITTAGS)
	{
		case MUIA_LeftEdge:
			data->left = (LONG)tag->ti_Data;
			break;

		case MUIA_TopEdge:
			data->top = (LONG)tag->ti_Data;
			break;

		case MUIA_Width:
			data->width = (LONG)tag->ti_Data;
			break;

		case MA_EditString_ReportObj:
			data->reportobj = (APTR)tag->ti_Data;
			break;

		case MUIA_String_Contents:
			set(obj, MUIA_String_BufferPos, strlen((STRPTR)tag->ti_Data));
			break;
	}
	NEXTTAG

	set(obj, MUIA_String_DoBoopsiForward, TRUE);

	return ((ULONG)obj);
}


#if 0
DEFMMETHOD(Setup)
{
	GETDATA;

	if (DOSUPER)
	{
		if (muiRenderInfo(obj) && _win(obj))
		{
			data->ehnode.ehn_Object = obj;
			data->ehnode.ehn_Class = cl;
			data->ehnode.ehn_Events = IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY;
			data->ehnode.ehn_Priority = 100;
			data->ehnode.ehn_Flags = 0;
			DoMethod(_win(obj), MUIM_Window_AddEventHandler, (ULONG)&data->ehnode);
		}
		return (TRUE);
	}
	return (FALSE);
}


/* XXX: sigh.. eventhandler doesn't work at all somehow.. bah. looks like the string eats everything anyway */
DEFMMETHOD(HandleEvent)
{
	if (msg->imsg)
	{
		dprintf("entered\n");
		if (msg->imsg->Class == IDCMP_MOUSEBUTTONS)
		{
			if (msg->imsg->Code == MENUDOWN)
			{
				DoMethod(obj, MM_EditString_Report, TRUE);
				return (MUI_EventHandlerRC_Eat);
			}
		}
		else if (msg->imsg->Class == IDCMP_RAWKEY)
		{
			if (msg->imsg->Code == 0x45) /* esc */
			{
				DoMethod(obj, MM_EditString_Report, TRUE);
				return (MUI_EventHandlerRC_Eat);
			}
		}
	}
	return (0);
}


DEFMMETHOD(Cleanup)
{
	GETDATA;

	if (muiRenderInfo(obj) && _win(obj))
	{
		DoMethod(_win(obj), MUIM_Window_RemEventHandler, (ULONG)&data->ehnode);
	}
	return (DOSUPER);
}
#endif


DEFMMETHOD(Layout)
{
	GETDATA;

	msg->left = data->left - _addleft(obj);
	msg->top = data->top - _addtop(obj) + 1;
	msg->width = data->width + _subwidth(obj);
	msg->height = _minheight(obj);

	return (DOSUPER);
}


DEFSMETHOD(EditString_Report)
{
	GETDATA;
	
	STRPTR s = NULL;

	if (!msg->abort)
	{
		get(obj, MUIA_String_Contents, &s);
	}

	DoMethod(data->reportobj ? data->reportobj : _parent(obj), MM_EditString_EditStop, s);

	return (0);
}


DEFMMETHOD(String_BoopsiForward)
{
	if (msg->BoopsiID == GM_GOINACTIVE)
	{
		STRPTR s = NULL;

		get(obj, MUIA_String_Contents, &s);

		/* XXX: there should be a way to trap RMB or so to abort.. or maybe 'esc'.. or maybe both :) */

		DoMethod(obj, MM_EditString_Report, FALSE);
	}
	return (0);
}


BEGINMTABLE
DECNEW
#if 0
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECMMETHOD(HandleEvent)
#endif
DECSMETHOD(EditString_Report)
DECMMETHOD(Layout)
DECMMETHOD(String_BoopsiForward)
ENDMTABLE

DECSUBCLASS_NC(MUIC_String, editstringclass)
