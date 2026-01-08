/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2007 Ambient Open Source Team
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
 * $Id: clickpathbuttonclass.c,v 1.7 2009/09/27 19:18:05 kiero Exp $
 */

#include "ambient.h"

/* public */
#include <proto/dos.h>

/* private */
#include "mui_func.h"
#include "actiondispatcherclass.h"
#include "mimetype.h"
#include "dragdrop.h"
#include "file_func.h"

struct Data
{
	STRPTR path;
	ULONG position;
	STRPTR storage;
};

DEFNEW
{
	obj = DoSuperNew(cl, obj,
        ButtonFrame,
		MUIA_Background, MUII_ButtonBack,
		MUIA_Font, MUIV_Font_Tiny,
		MUIA_InputMode, MUIV_InputMode_RelVerify,
		MUIA_Weight, 0,
		TAG_MORE, INITTAGS
	);

	if ( obj )
	{
		GETDATA;

		data->path = (STRPTR)GetTagData( MA_ClickpathButton_Path, NULL, INITTAGS );
		data->position = GetTagData( MA_ClickpathButton_Position, 0, INITTAGS );
		data->storage = NULL;

		if ( data->path && data->position )
		{
			if ( data->path[ data->position - 1 ] == '/' )
				data->position--;
		}
	}

	return (ULONG)obj;
}

DEFDISPOSE
{
	GETDATA;

	if(data->storage)
	{
		free(data->storage);
	}

	return (DOSUPER);
}

DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_Icon_Path:

			if(data->storage)
			{
				free(data->storage);
			}

			if( (data->storage = malloc( data->position + 2 ) ) )
			{
				stccpy( data->storage, data->path, data->position + 1 );
			}

			*msg->opg_Storage = (ULONG) data->storage;

			return (TRUE);

		/* if we return MV_Icon_FileType_Directory, moving/copying between clickpathbuttons
		 * would be possible.
		 */
		case MA_Icon_FileType:
			*msg->opg_Storage = MV_Icon_FileType_None;
			return (TRUE);

		case MA_View_NumSelected:
			*msg->opg_Storage = 0;
			return (TRUE);

		case MA_DragDrop_Type:
			*msg->opg_Storage = MV_DragDrop_Type_Root;
			return (TRUE);

	}

	return (DOSUPER);
}

DEFMMETHOD(DragQuery)
{
	GETDATA;
	STRPTR srcpath = NULL;
	LONG ft = getv(msg->obj, MA_Icon_FileType);

	if (ft == MV_Icon_FileType_File || ft == MV_Icon_FileType_Directory)
	{
		if (getv(msg->obj, MA_View_NumSelected) > 1)
		{
			srcpath = (STRPTR)getv(msg->obj,MA_View_Path);
		}
		else
		{
			srcpath = (STRPTR)getv( msg->obj, MA_Icon_Path);

			if ( !isdevicename( srcpath ) )
			{
				APTR vo = _parent( msg->obj );
				if ( vo )
					srcpath = (STRPTR)getv( vo,MA_View_Path);
			}
		}
	}

	if ( srcpath )
	{
		/* check if we are not dropping to same dir */

		//DB(("Testing:<%s> vs <%s>\n", data->path, srcpath ));

		if ( strlen( srcpath ) != data->position || strncmp( srcpath, data->path, data->position ) )
			return (MUIV_DragQuery_Accept);
	}

	return (MUIV_DragQuery_Refuse);

}

DEFMMETHOD(DragDrop)
{
	GETDATA;
	STRPTR dest;

	dest = malloc( data->position + 2 );

	if ( dest )
	{
		stccpy( dest, data->path, data->position + 1 );
	}
	else
	{
		return 0;
	}

	//DB(("Object:0x%x\n", obj ));
	//DB(("Source:<%s>\n", getv(msg->obj,MA_View_Path)));
	//DB(("Destination:<%s>\n", dest));

	if (getv(msg->obj, MA_View_NumSelected) > 1)
	{
		APTR dispatcher = (APTR)DoMethod( app, MM_Application_CreateActionDispatcher );

		if ( dispatcher )
		{
			struct MinList ml;
			struct dragdropnode *ddn, *nextddn;

			SetAttrs(dispatcher,
				MA_ActionDispatcher_IQualifier, (APTR)getv( _parent(msg->obj ), MA_Iconview_Qualifier),
				MA_ActionDispatcher_Event, ACTION_EVENT_DRAGNDROP,
				MA_ActionDispatcher_DstURI, dest,
				MA_ActionDispatcher_DstID, (APTR)getv(_win( obj ), MA_Window_ID),
				MA_ActionDispatcher_SrcURI, (APTR)getv(msg->obj, MA_View_Path),
				MA_ActionDispatcher_RefWin, _win(msg->obj),
				TAG_DONE
			);

			NEWLIST(&ml);
			DoMethod(msg->obj, MM_View_GetSelectionList, &ml);

			ITERATELISTSAFE(ddn, nextddn, &ml)
			{
				if(!is_path_contained(dest, ddn->path))
				{
					DoMethod( dispatcher, MM_ActionDispatcher_AddURI, ddn->path, TRUE );
				}
				else
				{
					SDB(("Recursive drop, skipping\n"));
				}
				/* NOTE: no break since we must free all dragdropnodes regardless */
				free(ddn);
			}

			DoMethod( dispatcher, MM_ActionDispatcher_Execute );
		}
	}
	else
	{
		LONG filetype = getv(msg->obj, MA_Icon_FileType);
		if (filetype == MV_Icon_FileType_Directory || filetype == MV_Icon_FileType_File) /* XXX: and devices too, yep */
		{
			STRPTR path = (STRPTR) getv( msg->obj, MA_Icon_Path );

			if(!is_path_contained(dest, path))
			{
				APTR dispatcher = (APTR)DoMethod( app, MM_Application_CreateActionDispatcher );
				if ( dispatcher )
				{
					SetAttrs(dispatcher,
						MA_ActionDispatcher_IQualifier, getv(_parent( msg->obj ), MA_Iconview_Qualifier),
						MA_ActionDispatcher_Event, ACTION_EVENT_DRAGNDROP,
						MA_ActionDispatcher_DstURI, dest,
						MA_ActionDispatcher_DstID, getv(_win( obj ) , MA_Window_ID),
						MA_ActionDispatcher_SrcURI, path,
						MA_ActionDispatcher_RefWin, _win(msg->obj),
						TAG_DONE
					);

					DoMethod( dispatcher, MM_ActionDispatcher_AddURI, path, TRUE );
					DoMethod( dispatcher, MM_ActionDispatcher_Execute );
				}
			}
			else
			{
				SDB(("Recursive drop, skipping\n"));
			}
		}
	}

	free( dest );

	return(0);
}

BEGINMTABLE
DECNEW
DECGET
DECDISP
DECMMETHOD(DragQuery)
DECMMETHOD(DragDrop)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Text, clickpathbuttonclass)

