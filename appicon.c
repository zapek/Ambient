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
 * $Id: appicon.c,v 1.13 2025/07/23 23:54:26 geit Exp $
 */

#include "ambient.h"

/* public */
#include <graphics/gfx.h>
#include <workbench/workbench.h>
#include <proto/exec.h>

/* private */
#include "appicon.h"
#include "mui_func.h"
#include "ipc.h"
#include "image.h"
#include "screen.h"
#include "methodstack.h"
#include "iconlib/icon_internal.h"
#include "iconio.h"
#include "threads.h"

static void setattrs(APTR obj, struct ipc_appicon *msg)
{
	if (!((msg->diskobj->diskobj.do_CurrentX == NO_ICON_POSITION) || (msg->diskobj->diskobj.do_CurrentY == NO_ICON_POSITION)))
	{
		if (msg->diskobj->diskobj.do_CurrentX < 0)
		{
			msg->diskobj->diskobj.do_CurrentX = 0;
		}

		methodstack_push(obj, 3,
			MUIM_Set,
			MA_Icon_X, msg->diskobj->diskobj.do_CurrentX
		);

		if (msg->diskobj->diskobj.do_CurrentY < 0)
		{
			msg->diskobj->diskobj.do_CurrentY = 0;
		}

		methodstack_push(obj, 3,
			MUIM_Set,
			MA_Icon_Y, msg->diskobj->diskobj.do_CurrentY
		);

		methodstack_push(obj, 3,
			MUIM_Set,
			MA_Icon_HasPos, TRUE
		);
	}

	methodstack_push(obj, 3,
		MUIM_Set,
		MA_Icon_Type, MV_Icon_Type_AppIcon
	);

	methodstack_push(obj, 3,
		MUIM_Set,
		MA_Icon_FileType, MV_Icon_FileType_None
	);
	
	methodstack_push(obj, 3,
		MUIM_Set,
		MA_Icon_AppAddress, msg
	);

	methodstack_push(obj, 3,
		MUIM_Set,
		MA_Icon_Name, msg->text
	);

	methodstack_push(obj, 3,
		MUIM_Set,
		MA_Icon_MsgPort, msg->userport
	);

	methodstack_push(obj, 3,
		MUIM_Set,
		MA_Icon_AppID, msg->id
	);

	methodstack_push_sync(obj, 3,
		MUIM_Set,
		MA_Icon_AppUserData, msg->userdata
	);

	D(APPICON,bug("Loaded appicon %s\n", msg->text));
}


ULONG tr_appicon_read(APTR obj UNUSED, APTR o, struct ipc_appicon *msg)
{
	CHECKOBJECT(obj);
	ASSERT(msg);

	if (icon_read(msg->diskobj->path, o,
		ICONTAG_Position, FALSE,
		ICONTAG_End, FALSE,
	TAG_DONE))
	{
		setattrs(o, msg);
	
		return (TRUE);
	}
	return (FALSE);
}


/*
 * Creates and display an AppIcon out of an
 * ipc_appicon message. Not to be called from a
 * thread.
 */
static ULONG appicon_create(struct ipcmessage *imsg)
{
	APTR o;
	struct ipc_appicon *msg;

	MAINTASK;
	ASSERT(imsg);

	msg = imsg->msgtype;

	ASSERT(msg);

	if ( (o = (APTR)DoMethod(app, MM_Application_CreateIcon, TRUE, MV_ViewID_Root)) )
	{
		struct Image *img;
		ULONG imgwidth;

		if (ISOWN(msg->diskobj))
		{
			if (do_action(app, TA_Appicon_Read,
				TT_Appicon_Read_Message, imsg,
				TT_Appicon_Read_Object, o,
			TAG_DONE))
			{
				return (ASYNC);
			}
		}
		else
		{
			//struct Screen *screen;

			img = msg->diskobj->diskobj.do_Gadget.GadgetRender;
			imgwidth = RASSIZE(img->Width, 1);

			//screen = get_screen();
			remap_image(o, MV_Icon_BitMap_Normal, img, imgwidth, img->ImageData);

			if (msg->diskobj->diskobj.do_Gadget.SelectRender)
			{
				img = msg->diskobj->diskobj.do_Gadget.SelectRender;
				imgwidth = RASSIZE(img->Width, 1);

				remap_image(o, MV_Icon_BitMap_Selected, img, imgwidth, img->ImageData);
			}

			setattrs(o, msg);

			DoMethod(o, MM_Icon_End);

			DoMethod(app, MM_Application_AddAppIcon, o);

			return (TRUE);
		}
	}
	return (FALSE);
}


static void appicon_delete(struct ipcmessage *msg)
{
	MAINTASK;
	ASSERT(msg);

	D(APPICON,bug("Remove Appicon:0x%x\n", msg->msgtype));
	DoMethod(app, MM_Application_DeleteAppIcon, msg->msgtype);
	D(APPICON,bug("Removed\n"));
}

/*
 * iterate iconview child list and for each appicon on it check if it exists on shared appicon list.
 * if on shared list it's marked as removed, remove from iconview. second pass scans shared list and
 * looks for newly created appicon entries. if such appicon is found, it's added to view.
 */

void appicon_synchronize(void)
{
	struct IpcAppiconNode *n, *nn;

	D(APPICON,bug("Synchronize appicons...\n"));

	ObtainSemaphore(&ipcdata.sema);

	ITERATELISTSAFE(n, nn, &ipcdata.appicon_list)
	{
		if (n->removedentry)
		{
			struct ipc_appicon *imsg  UNUSED = n->entry->msgtype;
			D(APPICON,bug("Synchronize:Remove entry on list:0x%x. loadedentry:%d:%s\n", n->entry, n->loadedentry, imsg->text));

			if (n->loadedentry == TRUE)
			{
				appicon_delete(n->entry);
				REMOVE(n);
				DeletePool(n->entry->pool);
			}
			else
			{
				D(APPICON,bug("Postpone removal.\n"));
			}
		}
	}

	ITERATELISTSAFE(n, nn, &ipcdata.appicon_list)
	{
		if (n->newentry)
		{
			struct ipc_appicon *imsg UNUSED = n->entry->msgtype;
			D(APPICON,bug("Synchronize:New entry on list:0x%x (0x%x):%s.removedentry:%d\n", n->entry, imsg, imsg->text, n->removedentry));
			n->newentry = FALSE;
			if (appicon_create(n->entry) != ASYNC)
			{
				D(APPICON,bug("Mark As Loaded: entry on list:0x%x.%s, newovedentry:%d\n", n->entry, imsg->text, n->newentry));
				n->loadedentry = TRUE;
			}
		}
	}

	ReleaseSemaphore(&ipcdata.sema);

	D(APPICON,bug("Synchronized!\n"));

}

/*
 * Finds an icon on appicon list and marks it as loaded. If icon is marked as loaded it can be removed.
 */

void appicon_markasloaded(APTR appicon)
{
	struct IpcAppiconNode *n, *nn;

	D(APPICON,bug("Mark as loaded...\n"));

	ObtainSemaphore(&ipcdata.sema);

	ITERATELISTSAFE(n, nn, &ipcdata.appicon_list)
	{
		struct ipc_appicon *imsg = n->entry->msgtype;
		if (imsg == appicon)
		{
			n->loadedentry = TRUE;
			D(APPICON,bug("Mark As Loaded: entry on list:0x%x.%s, newovedentry:%d\n", n->entry, imsg->text, n->newentry));
		}
	}

	ReleaseSemaphore(&ipcdata.sema);

	/* process any pending entries which might be blocked by this icon loading */

	appicon_synchronize();

}

