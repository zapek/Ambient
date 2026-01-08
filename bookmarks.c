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
 * $Id: bookmarks.c,v 1.4 2017/11/25 20:51:35 bitrocky Exp $
 */

#include "ambient.h"

/* public */
#include <libraries/gadtools.h>

/* private */
#include "ambient_cat.h"
#include "contextmenu.h"
#include "mui_func.h"
#include "command.h"
#include "tags.h"
#include "storage.h"
#include "mimeuri.h"
#include "mimetype.h"
#include "bookmarks.h"
#include "prefs.h"
#include "name.h"

static APTR create_menuitem(STRPTR uri, STRPTR name, LONG viewid)
{
	APTR m = NULL;
	struct command_menu *cm = malloc( sizeof (*cm ) );

	/*
	 * We assign this action node to this entry. will be processed when selected.
	 */

	if(cm != NULL)
	{
		cm->name = malloc(strlen(name) + 1);

		if(cm->name != NULL)
		{
			cm->args = malloc(strlen(uri) + 32);

			if (cm->args != NULL)
			{

				strcpy(cm->name, name);
				sprintf((STRPTR)cm->args, "LoadURI \"%s\" VIEWID %ld TOFRONT", uri, viewid);
				cm->type = AC_INTERNAL;
				cm->actionnode = NULL;

				m = NewObject(getmenuitemclass(), NULL,
						MA_Menuitem_FreeCommand, TRUE,
						MUIA_Menuitem_Title, cm->name,
						MA_Menuitem_Command, cm,
						TAG_DONE);
				
				if (m != NULL)
				{
					set(m, MUIA_UserData, m);
					return m;
				}
			}
		}

		contextmenu_delete_cm(cm);
	}

	return NULL;
}

APTR bookmarks_buildmenuobj(LONG viewid)
{
	STRPTR *entries[ 2 ];
	APTR menustrip, menu;
	LONG ei;
	LONG added = 0;
	LONG allowadding = TRUE;

	APTR wo = (APTR)DoMethod(app, MM_Application_FindWindowByID, viewid);

	if (wo != NULL)
	{
		STRPTR path = (STRPTR)getv(wo, MA_Window_Path);
		if (path == NULL || *path == '\0')
			allowadding = FALSE;
	}
	else
		allowadding = FALSE;

	menustrip = MenustripObject,
				Child, menu = MenuObject,
					MUIA_Menu_Title, GSI(MSG_BOOKMARKS_MENUTITLE),
					End,
				End;

	if (menustrip == NULL)
		return NULL;

	if (allowadding)
	{
		APTR c1 = MenuitemObject,
					MUIA_Menuitem_Title, GSI( MSG_BOOKMARKS_ADD ),
					MUIA_UserData, 1,
					End;

		APTR c2 = MenuitemObject,
					MUIA_Menuitem_Title, GSI( MSG_BOOKMARKS_ADD_TEMPORARY ),
					MUIA_UserData, 2,
					End;

		APTR c3 = MenuitemObject,
					MUIA_Menuitem_Title, NM_BARLABEL,
					MUIA_UserData, 0,
					End;

		if (c1 != NULL)
			DoMethod(menu, MUIM_Family_AddTail, c1);
		if (c2 != NULL)
			DoMethod(menu, MUIM_Family_AddTail, c2);
		if (c3 != NULL)
			DoMethod(menu, MUIM_Family_AddTail, c3);
	}

	storage_get(STORAGE_BOOKMARKS, STORAGE_STRARRAY, (APTR *)&entries[ 0 ]);
	storage_get(STORAGE_TEMPORARY_BOOKMARKS, STORAGE_MSTRARRAY, (APTR *)&entries[ 1 ]);

	for(ei=0; ei<2; ei++)
	{
		LONG i;

		/* spacer between permanent and temporary */

		if (added != 0 && entries[ ei ] != NULL && entries[ ei ][ 0 ] != NULL)
		{
			APTR mi = MenuitemObject,
					MUIA_Menuitem_Title, NM_BARLABEL,
					MUIA_UserData, 0,
					End;

			if (mi != NULL)
				DoMethod(menu, MUIM_Family_AddTail, mi);
		}

		/* */

		for(i=0; entries[ ei ] != NULL && entries[ ei ][ i ] != NULL; i+=2)
		{
			TEXT label[ 384 ];
			APTR mi;
			STRPTR uri = entries[ ei ][ i ];
			STRPTR path = uri;
			STRPTR path_short = NULL;
			STRPTR name = entries[ ei ][ i + 1 ];
			LONG llen = 0;
			APTR mimectx;

			if ((mimectx = mimeuri_create()))
			{
				if (mimeuri_gather(mimectx, uri,
					MIMEURIGATHERTAG_FileIO, FALSE,
					MIMEURIGATHERTAG_Extension, FALSE,
					MIMEURIGATHERTAG_Protocol, FALSE,
					TAG_DONE))
				{
					path = mimeuri_getattr(mimectx, MIMEURIATTR_PATH);
				}
			}

			if (path != NULL) {
				path_short = malloc(128);
				name_shorten_ellipsis(path, path_short, 64, NAME_ELLIPSIS_MIDDLE);
			}

			if (strlen(name) > 1 && ( _conf(bookmarks_showonly) != BM_LOCATIONONLY) )
				if( _conf(bookmarks_showonly == BM_NAMESONLY) )
				{
					llen = snprintf( label, sizeof(label), "%s", name);
				} else {
					llen = snprintf( label, sizeof(label), "\033b%s\033n - %s", name, path_short);
				}
			else
				llen = snprintf( label, sizeof(label), "%s", path_short);

			if (llen >= sizeof(label) - 1)
			{
				if (strlen(name) > 1 && ( _conf(bookmarks_showonly) != BM_LOCATIONONLY) )
					if( _conf(bookmarks_showonly == BM_NAMESONLY) )
					{
						snprintf( label, sizeof(label) - 4, "%s", name);
					} else {
						snprintf( label, sizeof(label) - 4, "\033b%s\033b - %s", name, path_short);
					}
				else
					snprintf( label, sizeof(label) - 4, "%s", path_short);

				strcat(label, "...");
			}

			if (mimectx != NULL)
				mimeuri_delete(mimectx);

			mi = create_menuitem(uri, label, viewid);
			if (mi != NULL)
			{
				DoMethod(menu, MUIM_Family_AddTail, mi);
				added++;
			}

			if (path_short != NULL)
				free(path_short);
		}

		free(entries[ ei ]);
	}

	return menustrip;

}

void bookmarks_adduri(STRPTR uri, STRPTR description, LONG permanent)
{
	STRPTR nullentries[] = {NULL};
	STRPTR *entries = nullentries;
	STRPTR *mementries;
	STRPTR *newentries;
	LONG i;

	if (permanent)
		storage_get(STORAGE_BOOKMARKS, STORAGE_STRARRAY, (APTR *)&mementries);
	else
		storage_get(STORAGE_TEMPORARY_BOOKMARKS, STORAGE_MSTRARRAY, (APTR *)&mementries);

	if (mementries != NULL)
		entries = mementries;

	for (i = 0; entries[ i ] != NULL; i++)
	{
	}

	newentries = malloc(( i + 3 ) * sizeof(STRPTR));

	if (newentries != NULL)
	{
		memcpy(newentries, entries, i * sizeof(STRPTR));
		newentries[ i++ ] = uri;
		newentries[ i++ ] = (description != NULL && *description) ? description : (STRPTR)" ";
		newentries[ i ] = NULL;

		if (permanent)
			storage_set(STORAGE_BOOKMARKS, STORAGE_STRARRAY, newentries);
		else
			storage_set(STORAGE_TEMPORARY_BOOKMARKS, STORAGE_MSTRARRAY, newentries);

		free(newentries);
	}

	if (mementries != NULL)
		free(mementries);

}

void bookmarks_updatefromlist(APTR list, LONG permanent)
{
	STRPTR *entries;
	LONG num;

	num	= getv(list, MUIA_List_Entries);

	entries = malloc(( num * 2 + 1 ) * sizeof(STRPTR));

	if (entries != NULL)
	{
		LONG i;

		for(i=0; i<num; i++)
		{
			struct bookmarkitem *item;

			DoMethod(list, MUIM_List_GetEntry, i, &item);

			entries[ i * 2 + 0 ] = item->location;
			entries[ i * 2 + 1 ] = (*item->description) ? item->description : (STRPTR)" ";
		}

		entries[ num * 2 ] = NULL;

		if (permanent)
			storage_set(STORAGE_BOOKMARKS, STORAGE_STRARRAY, entries);
		else
			storage_set(STORAGE_TEMPORARY_BOOKMARKS, STORAGE_MSTRARRAY, entries);

		free(entries);
	}

}

