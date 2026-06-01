/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2006 by Michal Wozniak
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
 * $Id: toolbutton_bookmarksclass.c,v 1.8 2025/07/23 23:54:26 geit Exp $
 */

#include "ambient.h"

/* public */
#include <graphics/rpattr.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "gfx_blit.h"
#include "gfx_bitmap.h"
#include "imagecache.h"
#include "name.h"
#include "action.h"
#include "rexx.h"
#include "command.h"
#include "contextmenu.h"
#include "bookmarks.h"
#include "viewapi.h"


struct Data {
	APTR bm;
	ULONG args[ 1 ];
	APTR viewobj; /* cache it */
};

static APTR get_view(APTR obj, struct Data *data)
{
	if (data->viewobj == NULL)
		data->viewobj = _view(obj);

	return data->viewobj;
}

DEFNEW
{
	obj = DoSuperNew(cl, obj,
		MUIA_InputMode, MUIV_InputMode_RelVerify, /* no other modes supported, atm */
		TAG_MORE, INITTAGS
	);

	if (obj != NULL)
	{
		struct Data *data;

		data = INST_DATA(cl, obj);

		memset(data, 0, sizeof( struct Data ));

		data->bm = imagecache_getbitmap("toolbar/bookmarks.png", 24);

		DoMethod(obj, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_Toolbutton_Execute);
	}
	else
	{
		DB(("Failed to create toolbutton\n"));
	}

	return ((ULONG)obj);
}

DEFDISP
{
	return DOSUPER;
}


DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_Toolbutton_Args:
		{
			data->args[ 0 ] = 0;
			*msg->opg_Storage = (ULONG)data->args;
			return (TRUE);
		}
	}
	return (DOSUPER);
}


DEFMMETHOD(AskMinMax)
{
	ULONG h, w;
	GETDATA;

	DOSUPER;

	w = gfx_bitmap_width (data->bm);
	h = gfx_bitmap_height(data->bm);

	msg->MinMaxInfo->MinWidth  += w + 2;
	msg->MinMaxInfo->MinHeight += h + 2;
	msg->MinMaxInfo->MaxWidth  += w + 2;
	msg->MinMaxInfo->MaxHeight += h + 2;
	msg->MinMaxInfo->DefWidth  += w + 2;
	msg->MinMaxInfo->DefHeight += h + 2;

	return 0;
}


/* since this is like an icon we could abuse iconclass.c here? */
DEFMMETHOD(Draw)
{
	DOSUPER;

	if (msg->flags & MADF_DRAWOBJECT)
	{
		GETDATA;

		if (data->bm != NULL)
		{
			struct RastPort *rp;
			ULONG mleft, mtop, mwidth; //, mheight;

			ULONG imgwidth = gfx_bitmap_width (data->bm);
			//ULONG imgheight = gfx_bitmap_height(data->bm);

			mleft   = _mleft(obj);
			mtop    = _mtop(obj);
			mwidth  = _mwidth(obj);
//			mheight = _mheight(obj);

			rp    = _rp(obj);

			gfx_blit(data->bm,  rp,
				BLITTAG_DstType, BLITVAL_DstType_RastPort,
				BLITTAG_DstX,    mleft + (mwidth - imgwidth) / 2,
				BLITTAG_DstY,    mtop + 1,
				BLITTAG_Alpha,   0xffffffff,
			TAG_DONE);
		}
	}

	return 0;
}

DEFTMETHOD(Toolbutton_Update)
{
	MUI_Redraw(obj, MADF_DRAWOBJECT);

	return 0;
}

DEFTMETHOD(Toolbutton_Execute)
{
	GETDATA;

	APTR wo = _win(obj);
	APTR vo = get_view(obj, data);
	APTR menustrip = bookmarks_buildmenuobj(getv(wo, MA_Window_ID));

    /* show menu and get result */

	if (menustrip != NULL)
	{
		TEXT cmd[1024];
		LONG poprc = DoMethod(menustrip, MUIM_Menustrip_Popup, obj, 0 , _left(obj),_bottom(obj)+1);
		STRPTR uriquoted = NULL;
		struct viewnode *vn = viewapi_findbyid(getv(vo, MA_Viewgroup_ViewIndex));

		cmd[ 0 ] = 0;

		if (poprc == 1 || poprc == 2)
		{
			TEXT uri[1024];
			STRPTR path = (STRPTR)getv(vo, MA_View_Path);
			snprintf(uri, sizeof(uri), "%s?view=%s&mode=%s", path, vn->name, viewapi_getmodename(vn, getv(vo, MA_Viewgroup_ViewModeIndex)));
			uriquoted = name_build_readargs_quoted(uri, NULL, 0);
		}

		if (poprc == 1)
		{
			/* add bookmark entry */
			snprintf(cmd, sizeof(cmd), "AddBookmark URI %s PERMANENT", uriquoted);
		}
		else if (poprc == 2)
		{
			/* add temporary bookmark entry */
			snprintf(cmd, sizeof(cmd), "AddBookmark URI %s", uriquoted);
		}
		else if (poprc != 0)
		{
			APTR menuitem = (APTR)poprc; /* menuitem is stored in userdata field, so we get it here */
			contextmenu_execute(vo, NULL, menuitem, FALSE);
		}

		if (*cmd)
			execute_command( vo, AC_INTERNAL, cmd, NULL);

		if (uriquoted != NULL)
			name_delete(uriquoted);

		MUI_DisposeObject(menustrip);

	}

	return 0;
}


BEGINMTABLE
DECNEW
DECDISP
DECGET
DECMMETHOD(AskMinMax)
DECMMETHOD(Draw)
DECTMETHOD(Toolbutton_Execute)
DECTMETHOD(Toolbutton_Update)
ENDMTABLE

DECSUBCLASSPTR_NC(toolbuttonclass, toolbutton_bookmarksclass)

