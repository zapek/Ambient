/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2006-2007 by Michal Wozniak
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
 * $Id: toolbutton_historyclass.c,v 1.6 2008/08/15 18:33:07 fab Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "prefs.h"
#include "history.h"
#include "vfs.h"

struct labelnode
{
	struct MinNode n;
	LONG index;
	TEXT label[0];
};

struct Data {
	APTR menu;
	APTR toolbar;

	struct history *history;
	struct MinList labels;

};

static void delete_labels( struct MinList *labels )
{
	struct labelnode *ln, *nextln;

	ITERATELISTSAFE(ln, nextln, labels)
	{
		free(ln);
	}
}

DEFNEW
{
	obj = DoSuperNew(cl, obj,
		MUIA_ContextMenu, 1,
		TAG_MORE, INITTAGS
	);

	if ( obj )
	{
		struct Data *data;

		data = INST_DATA(cl, obj);

		data->menu = NULL;
		data->toolbar = NULL;
		data->history = NULL;
		NEWLIST( &data->labels );
	}

	return ((ULONG)obj);
}

DEFDISP
{
	GETDATA;

	delete_labels( &data->labels );

	if ( data->menu )
		MUI_DisposeObject( data->menu );

	return DOSUPER;
}

DEFMMETHOD(ContextMenuBuild)
{
	GETDATA;

	delete_labels( &data->labels );
	NEWLIST( &data->labels );

	if ( data->menu )
	{
		MUI_DisposeObject( data->menu );
		data->menu = NULL;
	}

	/* find history context... */

	{
		APTR o = _parent( obj );
		struct history *history = NULL;

		while ( o )
		{
			if ( get( o, MA_Toolbar_History, &history ) )
			{
				data->toolbar = o;
				break;
			}
			o = _parent( o );
		}

		/* build menu from it */

		if ( history )
		{
			APTR m;
			APTR hn;
			ULONG i = 0;

			data->menu = MenustripObject,
							Child, m = MenuObject,
								MUIA_Menu_Title, "History",
								End,
							End;

			ITERATELIST( hn, &history->list )
			{
				TEXT buf[ 128 ], buf2[128];
				APTR o;
				STRPTR path = historynode_getattr( hn, HISTORYNODETAG_PATH );
				LONG off = 0;
				LONG l;
				struct labelnode *ln;

				path = vfs_resolve_path(path, buf2, sizeof(buf2), FALSE);

				l = path ? strlen( path ) : 0;

				if ( l )
				{
					if (l > 120)
					{
						off = l - 120;
					}

					if ( i == history->position )
						snprintf( buf, sizeof( buf ) - 1, "\033b%s%s", off ? "..." : "", path + off );
					else
						snprintf( buf, sizeof( buf ) - 1, "%s%s", off ? "..." : "", path + off );

					ln = malloc( sizeof( struct labelnode ) + strlen( buf ) + 1 );

					if ( ln )
					{
						strcpy( ln->label, buf );
						ln->index = i;

						o = MenuitemObject,
								MUIA_Menuitem_Title, ln->label,
								MUIA_UserData, ln,
								End;

						ADDTAIL( &data->labels, ln );

						if ( o )
							DoMethod( m, OM_ADDMEMBER, o );
					}
				}
				i++;
			}

			data->history = history;
		}

	}
	return (ULONG)data->menu;
}

DEFMMETHOD(ContextMenuChoice)
{
	struct labelnode *ln;

	if ((ln = (struct labelnode*)getv(msg->item, MUIA_UserData)))
	{
		GETDATA;

		/* we are not going through action disptacher as it would be waste of time */

		if ( data->toolbar )
		{
			LONG pos = (LONG)history_getattr( data->history, HISTORYTAG_POSITION );

			DoMethod( data->toolbar, MM_Toolbar_HistoryMove, ln->index - pos );
		}
	}
	return (0);
}

BEGINMTABLE
DECNEW
DECDISP
DECMMETHOD(ContextMenuBuild)
DECMMETHOD(ContextMenuChoice)
ENDMTABLE

DECSUBCLASSPTR_NC(toolbutton_actionclass, toolbutton_historyclass)

