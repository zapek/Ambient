/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005 Ambient Open Source Team
 * actionlistclass.c, Copyright 2005 by Adam Waldenberg
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
 * $Id: actionlistclass.c,v 1.6 2021/12/31 17:52:18 piru Exp $
 */

#include "ambient.h"
#include "action.h"
#include "descsaver.h"
#include "gfx_blit.h"
#include "methodstack.h"
#include "mimetype.h"
#include "threads.h"
#include "typescanner.h"

/* public */
#include <exec/lists.h>
#include <exec/types.h>
#include <proto/dos.h>

/* private */
#include <macros/vapor.h>
#include "mui_func.h"
#include "ambient_cat.h"

struct Data {
	BOOL internal;
	APTR iconobj;
	APTR radiobutton;
	STRPTR path;
	STRPTR radio[32]; /* Should be more than enough... */
	int radio_len;
};


static inline void getmime(struct Data *data)
{
	LONG cnt = 0;
	IPTR mimetype = NULL;

	// Thread dispatcher does an OM_RELEASE once it's done, that way the object remains valid
	// for as long as it is needed!
	Object *mimeTypeObject = DoMethod(NewObject(getmimetypeclass(), NULL, TAG_DONE), OM_RETAIN);

	if (do_action(data->iconobj, TA_MimeType_Scan,
		TT_MimeType_Scan_Path, data->path,
		TT_MimeType_Scan_MimetypeObject, mimeTypeObject ,
		TAG_DONE))
	{
		/*
		 * We give it max 1s to find the type.
		 */

		while( cnt < 25 && !xget(mimeTypeObject, MA_Mimetype_TypeResolved) )
		{
			Delay( 2 );
			cnt++;
			methodstack_check( FALSE );
		}
	}

	if ( xget(mimeTypeObject, MA_Mimetype_TypeResolved) )
		set( data->iconobj, MA_Icon_MimeType, xget(mimeTypeObject, MA_Mimetype_Type) );

	DoMethod(mimeTypeObject, OM_RELEASE);
}

DEFNEW
{
	APTR iconobj = NULL;
	struct Data *data;

	FORTAG(INITTAGS)
	{
		case MA_Actionlist_Iconobj:
			iconobj = (APTR) tag->ti_Data;
			break;
	}
	NEXTTAG

	obj = DoSuperNew(cl, obj, MUIA_Group_Horiz, TRUE,
		GroupFrameT("Menu & doubleclick actions"),
		TAG_MORE, INITTAGS
	);

	/*
	 * We check what actions are defined to this mimetype
	 * and add all mui objects to choose between them.
	 */
	if (obj && iconobj)
	{
		struct internal_mimetype_node *imn;

		data = INST_DATA(cl, obj);
		data->iconobj = iconobj;
		data->internal = FALSE;
		data->path = (STRPTR) getv(iconobj, MA_Icon_Path);
		memset(&data->radio, 32, sizeof(STRPTR));
		data->radio_len = 0;
		data->radiobutton = NULL;
		getmime(data);

		/*
		 * This shouldn't happen... if it does then... huho.. :)
		 */
		if (!(imn = (struct internal_mimetype_node *) getv(iconobj, MA_Icon_MimeType)))
			return (ULONG) obj;

		if (imn->flags & MIMETYPEFLAG_INTERNAL)
		{
			data->internal = TRUE;
		}

		if (!data->internal)
		{
			APTR an, hs;
			int defaultnum = -1;

			if ( !ISLISTEMPTY( imn->action_list ) )
			{
				APTR acobj;

				ITERATELIST( an, imn->action_list )
				{
					int actiontype = (int) actionnode_getattr(an, ACTIONNODETAG_EVENT);
					STRPTR actionname = actionnode_getattr(an, ACTIONNODETAG_NAME);

					if (actiontype != ACTION_EVENT_DOUBLECLICK && actiontype != ACTION_EVENT_MENU)
						continue;

					if (!(data->radio[data->radio_len] = malloc(strlen(actionname) + 1)))
						continue;

					/*
					 * The doubleclick event holds the currently chosen default action.
					 */
					if (actiontype == ACTION_EVENT_DOUBLECLICK)
						defaultnum = data->radio_len;

					strcpy(data->radio[data->radio_len], actionname);
					data->radio_len++;
				}

				if ((hs = HSpace(25)))
					DoMethod(obj, OM_ADDMEMBER, hs);

				if (data->radio_len)
				{
					acobj = HGroup,
						Child, data->radiobutton = RadioObject,
							MUIA_Radio_Entries, data->radio,
						End,
					End;

					if (acobj)
					{
						DoMethod(obj, OM_ADDMEMBER, acobj);
					
						DoMethod(data->radiobutton, MUIM_Notify,
							MUIA_Radio_Active, MUIV_EveryTime,
							obj, 2, MM_Actionlist_SetDefaultAction,
							MUIV_TriggerValue
						);

					}

					DoMethod(data->radiobutton, MUIM_NoNotifySet,
						MUIA_Radio_Active, defaultnum
					);
				}

				if ((hs = HSpace(25)))
					DoMethod(obj, OM_ADDMEMBER, hs);
			}
		}
		else /* Means its an internal filetype... Inform the user. */
		{
			APTR nobj = NewObject(getnotificationclass(), NULL,
				MA_Notification_Text, GSI(MSG_FILETYPE_INTERNAL),
			TAG_DONE);

			set(obj, MUIA_FrameTitle, NULL);

			if (nobj)
				DoMethod(obj, OM_ADDMEMBER, nobj);
		}
	}

	return (ULONG) obj;
}

DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_Actionlist_Internal:
			*msg->opg_Storage = data->internal;
			return (TRUE);

		case MA_Actionlist_Iconobj:
			*msg->opg_Storage = (ULONG) data->iconobj;
			return (TRUE);
	}

	return (DOSUPER);
}

DEFDISPOSE
{
	int i;
	GETDATA;

	for (i = 0; i < data->radio_len; i++)
		if (data->radio[i])
			free(data->radio[i]);

	return DOSUPER;
}

DEFSMETHOD(Actionlist_SetDefaultAction)
{
	GETDATA;
	struct internal_mimetype_node *imn = (APTR) getv(data->iconobj, MA_Icon_MimeType);

	if (!data->internal)
	{
		descriptor_change_defaultaction(imn, msg->value);
	}

	return 0;
}

BEGINMTABLE
DECNEW
DECGET
DECDISPOSE
DECSMETHOD(Actionlist_SetDefaultAction)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, actionlistclass)
