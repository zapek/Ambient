/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2006 Ambient Open Source Team
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
 * $Id: clickpathclass.c,v 1.15 2013/10/28 20:02:39 geit Exp $
 */

#include "ambient.h"

/* public */
#include <proto/dos.h>

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "file_func.h"
#include "prefs_desktop.h"
#include "legacy.h"
#include "vfs.h"


#define SCROLLMETHOD(a, b) \
DoMethod(a, MUIM_Notify, MUIA_Pressed, MUIV_InputMode_RelVerify, obj, 1, MM_Clickpath_Scroll##b); \
DoMethod(a, MUIM_Notify, MUIA_Timer, MUIV_EveryTime, obj, 1, MM_Clickpath_Scroll##b)

struct Data {
	APTR pathgrp, rw_button, fw_button, space;
	STRPTR path;
	STRPTR current_path;
	LONG position;
	ULONG xpos;
	LONG arrowsvisible;
	ULONG pushid;
};

static inline void setscrollbuttons(APTR obj, struct Data *data)
{
	LONG width, actual_width, rw_width, fw_width, shown;

	get(data->pathgrp, MUIA_Virtgroup_Width, &width);
	get(data->pathgrp, MUIA_Width, &actual_width);
	get(data->rw_button, MUIA_Width, &rw_width);  /* used only when shown so size is known */
	get(data->fw_button, MUIA_Width, &fw_width);
	shown = data->arrowsvisible;

	if ((width - (shown ? (rw_width + fw_width) : 0)) > actual_width)
	{
		LONG remwidth = 0;
		BOOL foundfocus = FALSE;

		/*
		 * We need to check what element we are supposed to focus by.
		 */
		FORCHILD(data->pathgrp, MUIA_Group_ChildList)
		{
			if (!getv(child, MUIA_UserData) && foundfocus == FALSE)
			{
				remwidth += getv(child, MUIA_Width);
			}
			else foundfocus = TRUE;
		}
		NEXTCHILD

		/* ...Make sure we scroll the clickpath to the right place. */
		data->xpos = min(remwidth, width - actual_width);
		set(data->pathgrp, MUIA_Virtgroup_Left, data->xpos);

		shown = TRUE;
	}
	else
	{
		shown = FALSE;
	}

	if ( data->pushid )
		DoMethod( app, MUIM_Application_KillPushMethod, NULL, data->pushid, 0);

	data->pushid = 0;

	if ( data->arrowsvisible != shown )
	{
		data->pushid = DoMethod( app, MUIM_Application_PushMethod, obj, 3 | MUIV_PushMethod_Delay(150) | MUIF_PUSHMETHOD_NOTLAGGING | MUIF_PUSHMETHOD_VERIFY, MUIM_Set, MA_Clickpath_PathArrows, shown );
	}

};

static APTR create_pathbutton(CONST_STRPTR path, LONG position, CONST_STRPTR name, BOOL arrows, BOOL check_isdir)
{
	TEXT tmpname[/*strlen(name) + 1*/PATH_SIZE];
	//STRPTR vfs_sourcepath = NULL;
	ULONG dropable = arrows ? FALSE : TRUE;
	ULONG isroot = FALSE;

	if (check_isdir)
	{
		STRPTR tpath = malloc( position + 1 );
		if ( tpath )
		{
			stccpy( tpath, path, position + 1 );

			#warning isdir() calls Lock() + Examine() but we are in the main thread
			if ( !isdir( tpath ) )
				dropable = FALSE;

			free( tpath );
		}
		else
		{
			dropable = FALSE;
		}
	}

	vfs_resolve_path((STRPTR) name, tmpname, sizeof(tmpname), TRUE);

	/*
	 * Get rid of "/" signs.... except in "My MorphOS" name
	 */
	if (!path || strcmp("devices://", path))
	{
		if (tmpname[0])
		{
			STRPTR p = tmpname + 1;

			while (*p) { p++; }

			if (p[-1] == '/')
				p[-1] = '\0';
		}

		isroot = TRUE;
	}
	else
	{
		dropable = FALSE;
	}

	return NewObject(getclickpathbuttonclass(), NULL,
		MA_ClickpathButton_Path, path,
		MA_ClickpathButton_Position, position,
		MUIA_Text_Contents, tmpname,
		MUIA_CycleChain, arrows ? 0 : 1,
		arrows ? MUIA_FixWidthTxt : TAG_IGNORE, name,
		MUIA_Dropable, dropable,
		isroot ? MUIA_Draggable : TAG_IGNORE, TRUE,
		TAG_DONE
	);
}

static STRPTR get_next(STRPTR path)
{
	STRPTR p = path;

	while (*p != ':' && *p != '/')
	{
		if (*p == 0)
			return p;

		p++;
	}

	return p+1;
}

static void set_new_path(APTR obj, struct Data *data, STRPTR path)
{
	APTR newgrp;
	ULONG parent = FALSE; /* When new path is parent to current one */
	LONG parent_num = 0; /* index of last common element */

	/*
	 * Check what to do.
	 */
	if ( data->current_path )
		free( data->current_path );

	if ((data->current_path = malloc( strlen( path ) + 1 )))
		strcpy( data->current_path, path );

	if (*path == '\0')
		parent = TRUE;
	else if (data->position == -1)
		data->position = 0;

	if ( data->path )
	{
		ULONG l = strlen( data->path );
		ULONG l_new = strlen( path );

		if ( l > l_new )
		{
			if( 0 == strnicmp( data->path, path, l_new ) )
			{
				if ( !(strlen(data->path) > l_new) || ( data->path[l_new] == '/' || data->path[l_new - 1] == ':') )
				{
					/*
					 * Find out last common element.
					 */
					STRPTR next = path;
					LONG count = -1;

					do
					{
						next = get_next(next);
						count++;
					}
					while (*next);

					parent_num = count;
					parent = TRUE;
					path = data->path;
				}
			}
		}
	}

	if ( !data->path || !parent)
	{
		if ( data->path )
			free( data->path );

		if ((data->path = malloc( strlen( path ) + 1)))
			strcpy( data->path, path );
	}

	if ((newgrp = HGroupV, MUIA_Group_HorizSpacing, 0, End))
	{
		STRPTR p;
		ULONG  len = strlen(data->position == -1 ? data->path : path) + 1;

		if ((p = AllocTaskPooled(len)))
		{
			STRPTR next = p, buf = p;
			APTR rbutton, button;
			LONG count = -1, c;
			LONG i = 0;
			LONG current_len = 0;

			strcpy(p, data->position == -1 ? data->path : path);

			do
			{
				next = get_next(next);
				count++;
			}
			while (*next);

			/* Add "My MorphOS" button... This one is always visible */
			if ((rbutton = create_pathbutton("devices://", sizeof("devices://") - 1, dprefs_mymorphos_name_get(), FALSE, FALSE)))
			{
				DoMethod(rbutton, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3, MUIM_Set, MA_Clickpath_Position, -1);
				DoMethod(newgrp, OM_ADDMEMBER, rbutton);

				if (*path == '\0' || data->position == -1)
				{
					SetAttrs(rbutton,
						MUIA_Text_PreParse, "\033b",
						MUIA_UserData, TRUE, /* This button should be focused. */
						TAG_DONE
					);
				}

				do
				{
					next = get_next(p);
					c = *next;
					*next = 0;
					current_len += strlen( p );

					/*
					 * If this happens we entered "My MorphOS" initially...
					 */
					if (*p == '\0')
						break;

					/*
					 * We assume that we have got valid path where only the last file component
					 * could be a file. This saves from extensive isdir() usage in main thread. -itix
					 */

					if (!(button =  create_pathbutton(data->path, current_len, p, FALSE, c ? FALSE : TRUE)))
						break;

					DoMethod(button, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3, MUIM_Set, MA_Clickpath_Position, current_len );
					DoMethod(newgrp, OM_ADDMEMBER, button);

					if (((parent && i == parent_num) || (c == 0 && !parent)) &&
					    data->position != -1)
					{
						SetAttrs(button,
							MUIA_Text_PreParse, "\033b",
							MUIA_UserData, TRUE, /* This button should be focused. */
							TAG_DONE
						);
					}

					count--;
					i++;
					*next = c;
					p = next;
				}
				while (c);

				FreeTaskPooled(buf, len);
			}
		}

		/* swap objects and dispose */
		DoMethod(obj, MUIM_Group_InitChange);
		DoMethod(obj, OM_REMMEMBER, data->space);
		DoMethod(obj, OM_REMMEMBER, data->pathgrp);
		DoMethod(obj, OM_ADDMEMBER, newgrp);
		DoMethod(obj, OM_ADDMEMBER, data->space);

		if ( data->arrowsvisible )
			DoMethod( obj, MUIM_Group_MoveMember, data->fw_button, -2 );

		DoMethod(app, MUIM_Application_PushMethod, app, 2, MM_Application_DisposeObject, data->pathgrp);
		data->pathgrp = newgrp;
		setscrollbuttons(obj, data);
		DoMethod(obj, MUIM_Group_ExitChange);
	}
}

static void doset(APTR obj, struct Data *data, struct TagItem *tags)
{
	FORTAG(tags)
	{
		case MA_Clickpath_Path:
			set_new_path(obj, data, (STRPTR)tag->ti_Data);
			break;

		/*
		 * Build new current_path.
		 */
		case MA_Clickpath_Position:

			data->position = tag->ti_Data;

			if ( data->current_path )
				free( data->current_path );

			/*
			 * Are we dealing with ":"/rootwin or not?
			 */
			if (tag->ti_Data == (ULONG)-1)
			{
				if ((data->current_path = malloc(sizeof("devices://") - 1 + 1)))
				{
					strcpy( data->current_path, "devices://");
				}
			}
			else if ((data->current_path = malloc( tag->ti_Data + 1 )))
			{
				stccpy( data->current_path, data->path, tag->ti_Data + 1);
			}

			break;

		case MA_Clickpath_PathArrows:
			if ( data->arrowsvisible != (LONG)tag->ti_Data )
			{
				data->arrowsvisible = tag->ti_Data;

				DoMethod( obj, MUIM_Group_InitChange );

				if ( data->arrowsvisible )
				{
					DoMethod( obj, MUIM_Group_AddHead, data->rw_button );
					DoMethod( obj, MUIM_Group_AddTail, data->fw_button );
					DoMethod( obj, MUIM_Group_MoveMember, data->fw_button, -2 );
				}
				else
				{
					DoMethod( obj, OM_REMMEMBER, data->rw_button );
					DoMethod( obj, OM_REMMEMBER, data->fw_button );
				}

				DoMethod( obj, MUIM_Group_ExitChange );
			}
			break;
	}
	NEXTTAG
}

DEFNEW
{
	APTR pathgrp, space, rw_button, fw_button;

	obj = DoSuperNew(cl, obj,
		MUIA_Group_Horiz, TRUE,
		MUIA_Group_Spacing, 1, 
		Child, pathgrp = HGroupV, End,
		Child, space = HSpace(0),
		TAG_MORE, INITTAGS
	);

	rw_button = create_pathbutton(NULL, 0, "<", TRUE, FALSE);
	fw_button = create_pathbutton(NULL, 0, ">", TRUE, FALSE);

	if (obj && fw_button && rw_button)
	{
		struct Data *data;

		data = INST_DATA(cl, obj);
		data->pathgrp = pathgrp;
		data->rw_button = rw_button;
		data->fw_button = fw_button;
		data->space = space;
		data->xpos = 0;
		data->path = NULL;
		data->current_path = NULL;
		data->arrowsvisible = FALSE;
		data->pushid = 0;
		set(space, MUIA_Weight, 1);

		SCROLLMETHOD(data->rw_button, Back);
		SCROLLMETHOD(data->fw_button, Forward);
		doset(obj, data, INITTAGS);
	}
	else
	{
		MUI_DisposeObject( fw_button );
		MUI_DisposeObject( rw_button );
	}

	return (ULONG) obj;
}

DEFDISP
{
	GETDATA;

	if (data->path)
		free(data->path);

	if (data->current_path)
		free(data->current_path);

	if ( !data->arrowsvisible )
	{
		MUI_DisposeObject( data->rw_button );
		MUI_DisposeObject( data->fw_button );
	}

	if ( data->pushid )
		DoMethod( app, MUIM_Application_KillPushMethod, obj, data->pushid);

	return (DOSUPER);
}

DEFSET
{
	GETDATA;

	doset(obj, data, INITTAGS);
	return DOSUPER;
}

DEFGET
{
	switch (msg->opg_AttrID)
	{
		case MA_Clickpath_Path:
			{
				GETDATA;
				*msg->opg_Storage = (ULONG)data->current_path;
				return (TRUE);
			}
		case MA_Clickpath_FullPath:
			{
				GETDATA;
				*msg->opg_Storage = (ULONG)data->path;
				return (TRUE);
			}
		case MA_Clickpath_Position:
			{
				GETDATA;
				*msg->opg_Storage = data->position;
				return (TRUE);
			}
	}
	return DOSUPER;
}

DEFMMETHOD(Draw)
{
	GETDATA;
	DOSUPER;

	setscrollbuttons(obj, data);
	return 0;
}

DEFTMETHOD(Clickpath_ScrollBack)
{
	GETDATA;
	LONG xpos = data->xpos = data->xpos - 15;

	set(data->pathgrp, MUIA_Virtgroup_Left, xpos < 0 ? (data->xpos = 0) : xpos);
	return 0;
}

DEFTMETHOD(Clickpath_ScrollForward)
{
	GETDATA;
	LONG width, actual_width, xpos = data->xpos = data->xpos + 15;

	get(data->pathgrp, MUIA_Virtgroup_Width, &width);
	get(data->pathgrp, MUIA_Width, &actual_width);
	set(data->pathgrp, MUIA_Virtgroup_Left, xpos > (width -
	    actual_width) ? (data->xpos = width - actual_width) : xpos);

	return 0;
}

DEFTMETHOD(Clickpath_Parent)
{
	GETDATA;

	if ( data->path && data->current_path )
	{
		STRPTR p = data->current_path;

		/* hack to retrieve container directory for vfs */
		if(vfs_is_vfs_device(data->current_path) &&
		   data->current_path[strlen(data->current_path) - 1] == ':')
		{
			TEXT owner_dir[PATH_SIZE];

			vfs_get_owner_path(data->current_path, owner_dir, sizeof(owner_dir));

			set(obj, MA_Clickpath_Path, (STRPTR) owner_dir);
			set(obj, MA_Clickpath_Position, strlen(data->path));
		}
		else
		{
			while ( *p != 0 )
			{
				STRPTR np = get_next( p );

				if ( np - data->current_path >= strlen( data->current_path ) )
				{
					ULONG pos = p - data->current_path;

					if ( pos > 0 )
						set( obj, MA_Clickpath_Position, pos );
					else
						set( obj, MA_Clickpath_Position, -1 );

					return 0;
				}

				p = np;
			}
		}
	}

	return 0;
}

BEGINMTABLE
DECDISP
DECNEW
DECSET
DECGET
DECMMETHOD(Draw)
DECTMETHOD(Clickpath_ScrollBack)
DECTMETHOD(Clickpath_ScrollForward)
DECTMETHOD(Clickpath_Parent)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, clickpathclass)

