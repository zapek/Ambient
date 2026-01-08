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
 * $Id: logoclass.c,v 1.7 2017/08/02 04:55:00 cyfm Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "mui_func.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "modules/about/ppcinline/about.h"
extern struct Library *AboutBase; /* XXX: not clean.. I should really use protos.. */

struct Data {
	ULONG isdrop;
	ULONG pushid;
	ULONG flashing;
	LONG add;
	ULONG val;
	ULONG running;
	ULONG ranonce;
	ULONG click;
	ULONG egg;
	APTR bmtype;
	struct MUI_EventHandlerNode ehnode;
};


DEFNEW
{
	struct Data *data;
	APTR arraytype = NULL;
	APTR bmtype = NULL;
	ULONG width, height, depth;

	FORTAG(INITTAGS)
	{
		case MA_Logo_Type:
			arraytype = About_GetLogo(tag->ti_Data, &width, &height, &depth);
			break;
	}
	NEXTTAG

	if (arraytype)
	{
		if ((bmtype = gfx_bitmap_create(width, height, depth, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE)))
		{
			ASSERT(depth == 32);

			gfx_blit(arraytype, bmtype,
				BLITTAG_SrcType, BLITVAL_SrcType_Array,
				BLITTAG_SrcFormat, BLITVAL_SrcFormat_ARGB,
			TAG_DONE);
		
			obj = DoSuperNew(cl, obj,
				MUIA_Bitmap_Width, width,
				MUIA_Bitmap_Height, height,
				MUIA_FixWidth, width,
				MUIA_FixHeight, height,
				MUIA_Bitmap_Bitmap, gfx_bitmap_bm(bmtype),
				TAG_MORE, INITTAGS
			);
		}
		else
		{
			return (0);
		}
	}
	else
	{
		obj = DoSuperNew(cl, obj,
			TAG_MORE, INITTAGS
		);
	}

	if (!obj)
	{
		gfx_bitmap_delete(bmtype);
		return (0);
	}

	data = INST_DATA(cl, obj);
	data->bmtype = bmtype;

	
	FORTAG(INITTAGS)
	{
		case MUIA_Dropable:
			data->isdrop = tag->ti_Data;
			break;
	
		case MA_Logo_Flashing:
			data->flashing = tag->ti_Data;
			break;
	}
	NEXTTAG

	return ((ULONG)obj);
}


DEFMMETHOD(Setup)
{
	GETDATA;

	if (!DOSUPER)
	{
		return (0);
	}

	if (muiRenderInfo(obj) && _win(obj) && data->flashing && !data->isdrop)
	{
		data->ehnode.ehn_Object = obj;
		data->ehnode.ehn_Class = cl;
		data->ehnode.ehn_Events = IDCMP_MOUSEBUTTONS;
		data->ehnode.ehn_Priority = 1; /* XXX: is that right ? */
		data->ehnode.ehn_Flags = MUI_EHF_GUIMODE;
		DoMethod(_win(obj), MUIM_Window_AddEventHandler, (ULONG)&data->ehnode);
	}
	return (TRUE);
}


DEFTMETHOD(Logo_Flash)
{
	GETDATA;

	if (data->pushid)
	{
		DoMethod(app, MUIM_Application_KillPushMethod, obj, data->pushid);
		data->pushid = 0;
	}

	if (data->ranonce)
	{
		data->val = 0xffffffff;
		data->add = -0x11111111;
	}
	else
	{
		data->val = 0x11111111;
		data->add = 0x11111111;
		data->ranonce = TRUE;
	}

	DoMethod(obj, MM_Logo_DoFlash);

	return (0);
}


DEFTMETHOD(Logo_DoFlash)
{
	GETDATA;

	if (data->val == 0x11111111) /* XXX: should be 0 but alpha blit of 0 doesn't work.. nag cyfm */
	{
		data->add = 0x11111111;
	}
	else if (data->val == 0xffffffff)
	{
		data->add = -0x11111111;
	}

	if (data->val == 0xffffffff && data->add == -0x11111111 && data->running)
	{
		data->running = FALSE;
		set(obj, MUIA_Bitmap_Alpha, data->val);
		return (0);
	}
	else
	{
		data->running = TRUE;
		data->pushid = DoMethod(app, MUIM_Application_PushMethod, obj, (1 | MUIV_PushMethod_Delay(20)), MM_Logo_DoFlash); /* delay isn't precise because it depends on intuiticks */
	}

	data->val += data->add;

	set(obj, MUIA_Bitmap_Alpha, data->val);

	return (0);
}


DEFMMETHOD(DragQuery)
{
	if (msg->obj != obj)
	{
		if (getv(msg->obj, MA_Logo_Egg))
		{
			return (MUIV_DragQuery_Accept);
		}
	}
	return (MUIV_DragQuery_Refuse);
}


DEFMMETHOD(DragDrop)
{
	GETDATA;

	if (data->isdrop)
	{
		set(obj, MA_Logo_Egg, TRUE);
	}
	return (0);
}


DEFMMETHOD(DragBegin)
{
	return (0); /* let's make things more sneaky :) */
}


DEFSET
{
	FORTAG(INITTAGS)
	{
		case MA_Logo_Egg:
			{
				GETDATA;
				data->egg = tag->ti_Data;
			}
			break;
	}
	NEXTTAG

	return (DOSUPER);
}


DEFGET
{
	switch (msg->opg_AttrID)
	{
		case MA_Logo_Egg:
			{
				GETDATA;
				*msg->opg_Storage = data->egg;
				return (TRUE);
			}
	}
	return (DOSUPER);
}


DEFMMETHOD(HandleEvent)
{
	GETDATA;

	if (msg->imsg)
	{
		if (msg->imsg->Class == IDCMP_MOUSEBUTTONS)
		{
			if (msg->imsg->Code == SELECTDOWN && (msg->imsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)))
			{
				if (_isinobject(msg->imsg->MouseX, msg->imsg->MouseY))
				{
					data->click++;
					
					if (data->click == 8)
					{
						SetAttrs(obj, MUIA_Draggable, TRUE, MA_Logo_Egg, TRUE, TAG_DONE);
					}
					return (MUI_EventHandlerRC_Eat);
				}
			}
		}
	}
	return (0);
}


DEFMMETHOD(Cleanup)
{
	GETDATA;

	if (muiRenderInfo(obj) && _win(obj) && data->flashing && !data->isdrop)
	{
		DoMethod(_win(obj), MUIM_Window_RemEventHandler, (ULONG)&data->ehnode);
	}
	return (DOSUPER);
}


DEFDISPOSE
{
	GETDATA;

	if (data->pushid)
	{
		DoMethod(app, MUIM_Application_KillPushMethod, obj, data->pushid);
		data->pushid = 0;
	}

	gfx_bitmap_delete(data->bmtype);
	return (DOSUPER);
}


BEGINMTABLE
DECNEW
DECDISPOSE
DECSET
DECGET
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECMMETHOD(HandleEvent)
DECMMETHOD(DragQuery)
DECMMETHOD(DragDrop)
DECMMETHOD(DragBegin)
DECTMETHOD(Logo_Flash)
DECTMETHOD(Logo_DoFlash)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Bitmap, logoclass)
