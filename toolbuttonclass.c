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
 * $Id: toolbuttonclass.c,v 1.5 2006/09/18 23:17:31 fab Exp $
 */

#define TOOLBUTTON(x) GSI(MSG_PREFSWIN_TOOLBAR_##x)

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
#include "prefs.h"
#include "imagecache.h"
#include "name.h"
#include "command.h"

struct Data {
	STRPTR name;    /* caption */
	STRPTR help;    /* help string (optional) */
};


DEFNEW
{
	obj = DoSuperNew(cl, obj,
		MUIA_Font, MUIV_Font_Tiny,
		InnerSpacing(2,0),
		TAG_MORE, INITTAGS
	);

	if (obj)
	{
		struct Data *data;

		data = INST_DATA(cl, obj);

		data->name = name_build( (STRPTR)GetTagData( MA_Toolbutton_Name, (ULONG)" ", INITTAGS) );
		data->help = name_build( (STRPTR)GetTagData( MA_Toolbutton_Help, (ULONG)"", INITTAGS) );
		set(obj, MUIA_ShortHelp, ( data->help && strlen( data->help ) ) ? data->help : data->name);
	}
	else
	{
		DB(("Failed to create toolbutton\n"));
	}

	return ((ULONG)obj);
}

DEFDISP
{
	GETDATA;

	if ( data->help )
		name_delete( data->help );

	if ( data->name )
		name_delete( data->name );

	return DOSUPER;
}


DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_Toolbutton_Name:
		case MA_Toolbutton_Label:
		{
			*msg->opg_Storage = (ULONG)data->name;
			return (TRUE);
		}

		case MA_Toolbutton_Single:
		{
			*msg->opg_Storage = TRUE;
			return (TRUE);
		}

		case MA_Toolbutton_Viewflags:
		{
			*msg->opg_Storage = ~0;
			return (TRUE);
		}
	}
	return (DOSUPER);
}


DEFMMETHOD(AskMinMax)
{
	DOSUPER;

	msg->MinMaxInfo->MinWidth  += 2;
	msg->MinMaxInfo->MinHeight += 2;
	msg->MinMaxInfo->MaxWidth  += 2;
	msg->MinMaxInfo->MaxHeight += 2;
	msg->MinMaxInfo->DefWidth  += 2;
	msg->MinMaxInfo->DefHeight += 2;

	return 0;
}


/*
 * Draw default background/stuff.
 */

DEFMMETHOD(Draw)
{
	DOSUPER;

	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECGET
DECMMETHOD(AskMinMax)
DECMMETHOD(Draw)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Area, toolbuttonclass)
