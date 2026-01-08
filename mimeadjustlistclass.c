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
 * $Id: mimeadjustlistclass.c,v 1.7 2013/10/29 23:40:55 geit Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "ambient_cat.h"
#include "mimeadjustgroupclass.h"
#include "mui_func.h"
#include "action.h"
#include "mimetype.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "actioneditor_dnd_logo.h"
#include "actioneditor_menu_logo.h"


struct Data {
	APTR pctx; /* prefspool ctx */
	APTR pn;   /* parent prefspool listnode */
	ULONG id;  /* that one is incremented all the time to keep track of subwindows */

	struct
	{
		APTR dnd_o, dnd_bmp, dnd_list, menu_o, menu_bmp, menu_list;
	} images;
};

static BOOL set_img(APTR *muio, APTR *bitmap, APTR img, int w, int h)
{
	*bitmap = gfx_bitmap_create(w, h, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE);

	if (*bitmap)
	{
		gfx_blit(img, *bitmap, BLITTAG_SrcType, BLITVAL_SrcType_Array,
			BLITTAG_SrcFormat, BLITVAL_SrcFormat_ARGB, TAG_DONE
		);

		*muio = BitmapObject,
			MUIA_Bitmap_Width, w, MUIA_Bitmap_Height, h,
			MUIA_FixHeight, h, MUIA_FixWidth, w,
			MUIA_Bitmap_Bitmap, gfx_bitmap_bm(*bitmap),
			MUIA_Bitmap_Alpha, 0xffffffff,
		End;

		return *muio ? TRUE : FALSE;
	}

	return FALSE;
}

MUI_HOOK(mimeadjust_constructfunc, APTR pool, APTR * aearray)
{
	struct ActionEntry *ae;

	if ((ae = AllocPooled(pool, sizeof(struct ActionEntry))))
	{
		ae->action_node = *aearray++;
		ae->inherited   = (ULONG) *aearray;
	}

	return (ULONG) ae;
}

MUI_HOOK(mimeadjust_destructfunc, APTR pool, struct ActionEntry *ae)
{
	FreePooled(pool, ae, sizeof(struct ActionEntry));
	return 0;
}

DEFNEW
{
	obj = DoSuperNew(cl, obj,
		InputListFrame,
		MUIA_List_Title, TRUE,
		MUIA_List_Format,"C=0 MIW=-1 MAW=-2 BAR, C=1",
		MUIA_List_ConstructHook, (ULONG) &mimeadjust_constructfunc_hook,
		MUIA_List_DestructHook, (ULONG) &mimeadjust_destructfunc_hook,
		MUIA_List_MinLineHeight, 20,
		MUIA_List_AutoVisible, TRUE,
		MUIA_Dropable, TRUE,
		MUIA_List_DragType, MUIV_List_DragType_Immediate,
		TAG_MORE, INITTAGS
	);

	if (obj)
	{
		GETDATA;

		data->id = 0;

		data->images.dnd_list = data->images.menu_list = NULL;

		set_img(&data->images.dnd_o, &data->images.dnd_bmp, &actioneditor_dnd,
			ACTIONEDITOR_DND_WIDTH, ACTIONEDITOR_DND_HEIGHT
		);

		set_img(&data->images.menu_o, &data->images.menu_bmp, &actioneditor_menu,
			ACTIONEDITOR_MENU_WIDTH, ACTIONEDITOR_MENU_HEIGHT
		);

		if (data->images.dnd_o == NULL || data->images.dnd_bmp == NULL ||
		    data->images.menu_o == NULL || data->images.menu_bmp == NULL)
		{
			if (data->images.dnd_o)
				MUI_DisposeObject(data->images.dnd_o);

			if (data->images.menu_o)
				MUI_DisposeObject(data->images.menu_o);

			if (data->images.dnd_bmp)
				gfx_bitmap_delete(data->images.dnd_bmp);

			if (data->images.menu_bmp)
				gfx_bitmap_delete(data->images.menu_bmp);

			CoerceMethod(cl, obj, OM_DISPOSE);

			return NULL;
		}

	}

	return (ULONG)obj;
}

DEFDISPOSE
{
	GETDATA;

	MUI_DisposeObject(data->images.dnd_o);
	MUI_DisposeObject(data->images.menu_o);

	gfx_bitmap_delete(data->images.dnd_bmp);
	gfx_bitmap_delete(data->images.menu_bmp);

	return DOSUPER;
}

DEFMMETHOD(Setup)
{
	ULONG rc = DOSUPER;
	GETDATA;

	if (rc && _win(obj))
	{
		data->images.dnd_list = (APTR) DoMethod(obj, MUIM_List_CreateImage,
			data->images.dnd_o, 0L
		);

		data->images.menu_list = (APTR) DoMethod(obj, MUIM_List_CreateImage,
			data->images.menu_o, 0L
		);
	}

	return rc;
}

DEFMMETHOD(Cleanup)
{
	GETDATA;

	DoMethod(obj, MUIM_List_DeleteImage, data->images.dnd_list);
	DoMethod(obj, MUIM_List_DeleteImage, data->images.menu_list);

	return DOSUPER;
}

DEFMMETHOD(List_Display)
{
	GETDATA;

	if(msg->entry)
	{
		struct ActionEntry * ae = (struct ActionEntry *) msg->entry;
		STRPTR s = (STRPTR) actionnode_getattr(ae->action_node, ACTIONNODETAG_NAME);
		APTR listimg;
		STRPTR qualifier= "";
		STRPTR type="";
		BOOL bold = FALSE;

		switch ((ULONG) actionnode_getattr(ae->action_node, ACTIONNODETAG_EVENT))
		{
			case ACTION_EVENT_DOUBLECLICK:
				bold = TRUE;
				listimg = data->images.menu_list;
				type = GSI(MSG_MIMEADJUSTLISTCLASS_EVENTDOUBLE);
				break;
			case ACTION_EVENT_MENU:
				listimg = data->images.menu_list;
				type = GSI(MSG_MIMEADJUSTLISTCLASS_EVENTMENU);
				break;
			case ACTION_EVENT_DRAGNDROP:
				listimg = data->images.dnd_list;
				type = GSI(MSG_MIMEADJUSTLISTCLASS_EVENTDRAGNDROP);
				break;
			default: /* ACTION_EVENT_NONE */
				listimg = NULL;
				type = "";
				break;
		}

		snprintf(ae->name, sizeof(ae->name), "\33O[%08lx] %s%s%s",
			(ULONG) listimg, bold ? "\33b" : "", (ae->inherited ? GSI(MSG_MIMEADJUSTLISTCLASS_INHERITEDACTION) : (STRPTR) ""),s
		);

		msg->array[ 0 ] = ae->name;

		switch ((ULONG) actionnode_getattr(ae->action_node, ACTIONNODETAG_QUALIFIER))
		{
			case ACTION_QUALIFIER_SHIFT:
				qualifier = GSI(MSG_MIMEADJUSTLISTCLASS_QUALSHIFT);
				break;
			case ACTION_QUALIFIER_ALT:
				qualifier = GSI(MSG_MIMEADJUSTLISTCLASS_QUALALT);
				break;
			case ACTION_QUALIFIER_CONTROL:
				qualifier = GSI(MSG_MIMEADJUSTLISTCLASS_QUALCONTROL);;
				break;
			default: /* ACTION_QUALIFIER_NONE */
				qualifier = "";
				break;
		}

		snprintf(ae->type, sizeof(ae->type), "%s%s %s",
			ae->inherited ? "\033i" : "", type, qualifier
		);

		msg->array[ 1 ] = ae->type;


	}
	else
	{
		msg->array[ 0 ] = GSI(MSG_MIMEADJUSTLISTCLASS_NAME);
		msg->array[ 1 ] = GSI(MSG_MIMEADJUSTLISTCLASS_EVENTTYPE);
	}

	return 0;
}

DEFMMETHOD(List_Compare)
{
	struct ActionEntry *e1 = (struct ActionEntry *)msg->entry1;
	struct ActionEntry *e2 = (struct ActionEntry *)msg->entry2;

	STRPTR str1 = (STRPTR) actionnode_getattr(e1->action_node, ACTIONNODETAG_NAME);
	STRPTR str2 = (STRPTR) actionnode_getattr(e2->action_node, ACTIONNODETAG_NAME);

	return stricmp(str1, str2);
}

DEFGET
{
	switch (msg->opg_AttrID)
	{
		case MA_Actioneditor_ActionNode:
		*msg->opg_Storage = TRUE;
		return(TRUE);
	}

	return (DOSUPER);
}

BEGINMTABLE
DECNEW
DECDISP
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECMMETHOD(List_Display)
DECMMETHOD(List_Compare)
DECGET
ENDMTABLE

DECSUBCLASS_NC(MUIC_List, mimeadjustlistclass)
