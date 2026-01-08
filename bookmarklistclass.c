/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006 Ambient Open Source Team
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
 * $Id: bookmarklistclass.c,v 1.2 2013/10/28 19:48:26 geit Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "command.h"
#include "rexx.h"
#include "contextmenu.h"
#include "mimetype.h"
#include "actiondispatcherclass.h"
#include "action.h"
#include "typescanner.h"
#include "file_func.h"
#include "methodstack.h"
#include "threads.h"
#include "mimeuri.h"
#include "name.h"
#include "storage.h"

struct Data {
	ULONG dummy;
};

struct bookmarkitem {
	STRPTR description;
	STRPTR location;
};


DEFMMETHOD(List_Construct)
{
	struct bookmarkitem *item = malloc(sizeof(struct bookmarkitem));
	struct bookmarkitem *useritem = (struct bookmarkitem*)msg->entry;

	if (item != NULL)
	{
		item->description = useritem ? name_build(useritem->description) : name_build(GSI(MSG_BOOKMARK_UNNAMED));
		item->location = useritem ? name_build(useritem->location) : name_build("");

		if (item->description == NULL || item->location == NULL)
		{
			if (item->description != NULL )
				name_delete(item->description);
			if (item->location != NULL )
				name_delete(item->location);

			free(item);
			return NULL;
		}
	}

	return (ULONG)item;
}

DEFMMETHOD(List_Destruct)
{
	struct bookmarkitem *item = (struct bookmarkitem*)msg->entry;

	if (item != NULL)
	{
		if (item->description != NULL )
			name_delete(item->description);
		if (item->location != NULL )
			name_delete(item->location);

		free(item);
	}

	return 0;
}

static void doset(APTR obj UNUSED, struct Data *data UNUSED, struct TagItem *tags)
{
	FORTAG(tags)
	{
		case MUIA_List_DoubleClick:
		{
			break;
		}
	}
	NEXTTAG
}


DEFNEW
{
	obj = DoSuperNew(cl, obj,
		MUIA_List_Format, "C=0 MIW=-1 BAR,C=1 MIW=-1",
		MUIA_List_Title, TRUE,
		InputListFrame,
		MUIA_CycleChain, TRUE,
		TAG_MORE, INITTAGS
	);

	if (obj)
	{
	}

	return (ULONG)obj;
}


DEFMMETHOD(List_Display)
{
	struct bookmarkitem *item = (struct bookmarkitem*)msg->entry;

	if (item == NULL)
	{
		msg->array[ 0 ] = GSI(MSG_BOOKMARK_NAME_LISTTITLE);
		msg->array[ 1 ] = GSI(MSG_BOOKMARK_LOCATION_LISTTITLE);
	}
	else
	{
		msg->array[ 0 ] = item->description;
		msg->array[ 1 ] = item->location;

		if( (ULONG)msg->array[ -1 ] % 2 )
		{
			msg->array[ -9 ] = (STRPTR) 10;
		}
	}

	return 0;
}

DEFSET
{
	GETDATA;

	doset(obj, data, INITTAGS);

	return (DOSUPER);
}

DEFMMETHOD(Setup)
{
	ULONG rc = DOSUPER;

	if (rc)
	{
		STRPTR *entries;

		storage_get(STORAGE_BOOKMARKS, STORAGE_STRARRAY, (APTR *)&entries);

		set(obj, MUIA_List_Quiet, TRUE);
		DoMethod(obj, MUIM_List_Clear);

		if (entries)
		{
			LONG i;
			struct bookmarkitem item;

			for (i = 0; entries[ i ] != NULL; i+=2)
			{
				item.location = entries[ i ];
				item.description = entries[ i + 1 ];
				DoMethod(obj, MUIM_List_InsertSingle, &item, MUIV_List_Insert_Bottom);
			}

			free(entries);
		}

		set(obj, MUIA_List_Quiet, FALSE);

	}

	return rc;
}

BEGINMTABLE
DECNEW
DECSET
DECMMETHOD(Setup)
DECMMETHOD(List_Display)
DECMMETHOD(List_Construct)
DECMMETHOD(List_Destruct)
ENDMTABLE

DECSUBCLASS_NC(MUIC_List, bookmarklistclass)
