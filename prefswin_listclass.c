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
 * $Id: prefswin_listclass.c,v 1.6 2006/09/18 23:17:30 fab Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "mui_func.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "prefswin.h"

struct Data {
	APTR images[PREFSWIN_NUMPAGES];
	APTR iobjs[PREFSWIN_NUMPAGES];
	APTR ibm[PREFSWIN_NUMPAGES];
};


DEFNEW
{
	obj = DoSuperNew(cl, obj,
		InputListFrame,
		MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
		MUIA_List_DestructHook, MUIV_List_DestructHook_String,
		MUIA_List_MinLineHeight, 24, /* XXX: check.. */ 
		MUIA_List_AdjustWidth, TRUE,
		MUIA_List_AutoVisible, TRUE,
	End;

	if (!obj)
	{
		return (0);
	}
	return ((ULONG)obj);
}


DEFMMETHOD(List_Display)
{
	GETDATA;
	static TEXT tmp[40];
	
	snprintf(tmp, sizeof(tmp), "\033O[%08lx] %s", (ULONG)data->images[(ULONG)msg->array[-1]], (STRPTR)msg->entry);
	msg->array[0] = tmp;
	return (0);
}


DEFMMETHOD(Setup)
{
	ULONG c;
	GETDATA;

	if (!DOSUPER)
	{
		return (FALSE);
	}
	
	for (c = 0; c < PREFSWIN_NUMPAGES; c++)
	{
		if ((data->ibm[c] = gfx_bitmap_create(26, 20, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE)))
		{
			gfx_blit(prefsgrp[c].logo, data->ibm[c],
				BLITTAG_SrcType, BLITVAL_SrcType_Array,
				BLITTAG_SrcFormat, BLITVAL_SrcFormat_ARGB,
			TAG_DONE);

			data->iobjs[c] = BitmapObject,
				MUIA_Bitmap_Height, 20, MUIA_Bitmap_Width, 26,
				MUIA_FixHeight, 20, MUIA_FixWidth, 26,
				MUIA_Bitmap_Bitmap, gfx_bitmap_bm(data->ibm[c]),
				MUIA_Bitmap_Alpha, 0xffffffff,
			End;

			data->images[c] = (APTR)DoMethod(obj, MUIM_List_CreateImage, data->iobjs[c], 0);
		}
		else
		{
			/* XXX: we should fail.. and is Cleanup executed ? */
		}
	}

	return (TRUE);
}


DEFMMETHOD(Cleanup)
{
	GETDATA;
	int	c;

	for (c = PREFSWIN_NUMPAGES - 1; c > 0; c--)
	{
		DoMethod(obj, MUIM_List_DeleteImage, data->images[c]);
		MUI_DisposeObject(data->iobjs[c]);
		gfx_bitmap_delete(data->ibm[c]);
	}

	return (DOSUPER);
}


BEGINMTABLE
DECNEW
DECMMETHOD(List_Display)
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
ENDMTABLE

DECSUBCLASS_NC(MUIC_List, prefswin_listclass)
