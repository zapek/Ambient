/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2004 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2016 Ambient Open Source Team
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
 * $Id: appicon.c,v 1.13 2016/06/19 16:33:06 itix Exp $
 */

#include "globals.h"

/* public */
#include <string.h>
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <workbench/workbench.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <dos/dosextens.h>

/* private */
#include "clib/wb_protos.h"
#include "../ipc.h"
#include "../iconlib/icon_internal.h"
#include "macros/vapor.h"
#include "qport.h"


#define DB_ADDAPPICON 0
#define DB_REMOVEAPPICON 0

struct appiconmsg {
	struct ipcmessage    gmsg;
	struct ipc_appicon   msg;
};

/****** workbench.library/AddAppIconA() *******************************
*
* NAME
*   AddAppIconA -- Add an application icon to desktop.
*
* SYNOPSIS
*   AppIcon = AddAppIconA(id, userdata, text, msgport, lock, icon, taglist)
*
*   APTR AddAppIconA(ULONG, ULONG, UBYTE *, struct MsgPort *, BPTR,
*           struct DiskObject *, struct TagItem *);
*
* FUNCTION
*   Add an application icon to desktop. User can drag'n drop files to your AppIcon
*   or just doubleclick on the icon. An AppMessage is sent to your message port.
*
*   To add an icon, Ambient must be running.
*
* INPUTS
*   id       - private id
*   userdata - private user data
*   text     - text to be displayed on your icon
*   lock     - not used and must be ZERO (NULL)
*   msgport  - message port to receive AppMessage
*   icon     - pointer to a DiskObject icon
*   taglist  - no tags defined yet (you may pass NULL)
*
* RESULT
*   AppIcon  - A pointer to AppIcon handle or NULL if icon could not be added.
*
* SEE ALSO
*   RemoveAppIcon()
*
*****************************************************************************
*
*/
struct AppIcon *AddAppIconA(ULONG id, ULONG userdata, UBYTE *text, struct MsgPort *msgport, struct FileLock *lock, struct DiskObject *diskobj, struct TagItem *taglist)
{
	struct Image *img1;

	img1 = (struct Image *)diskobj->do_Gadget.GadgetRender;

	/*
	 * Find the public port, send a message.
	 */
	if (text && *text && msgport && diskobj && img1)
	{
		APTR pool;

		/*
		 * Ok, the rest is a mess. Basically we create a message then copy the
		 * diskobj, image and imagedata in it.
		 *
		 * NEW: There is now new tag WBAPPICONA_Clone which defaults to FALSE.
		 * Set it to TRUE if you wish to take an advantage of diskobj copy
		 * feature and ensure compatibility with future releases.
		 */
		D(ADDAPPICON,bug("text: %s, msgport: 0x%lx, diskobj: 0x%lx, imagedata: 0x%lx\n", text, (ULONG)msgport, (ULONG)diskobj, (ULONG)img1->ImageData));
		if ( (pool = CreatePool(MEMF_ANY, 2048, 2048)) )
		{
			struct appiconmsg *msg;

			if ( (msg = AllocPooled(pool, sizeof(*msg))) )
			{
				if ( (msg->msg.text = AllocPooled(pool, strlen(text) + 1)) )
				{
					strcpy(msg->msg.text, text);

					if ( (msg->msg.diskobj = AllocPooled(pool, sizeof(*msg->msg.diskobj))) )
					{
						ULONG imgsize = RASSIZE(img1->Width, img1->Height) * img1->Depth;

						memcpy(msg->msg.diskobj, diskobj, sizeof(*msg->msg.diskobj));

						/* XXX: well, this is screwy to have to recopy all the stuff all the time.. perhaps there's a better way.. */

						if (diskobj->do_Gadget.GadgetRender)
						{
							if ( (msg->msg.diskobj->diskobj.do_Gadget.GadgetRender = AllocPooled(pool, sizeof(struct Image))) )
							{
								memcpy(msg->msg.diskobj->diskobj.do_Gadget.GadgetRender, diskobj->do_Gadget.GadgetRender, sizeof(struct Image));

								if ( (((struct Image *)msg->msg.diskobj->diskobj.do_Gadget.GadgetRender)->ImageData = AllocPooled(pool, imgsize)) )
								{
									ULONG imgsize2 = NULL;
									struct Image *img2 = (struct Image *)diskobj->do_Gadget.SelectRender;

									D(ADDAPPICON,bug("imgsize1: %ld bytes\n", imgsize));

									memcpy(((struct Image *)msg->msg.diskobj->diskobj.do_Gadget.GadgetRender)->ImageData, img1->ImageData, imgsize);

									if (img2)
									{
										imgsize2 = RASSIZE(img2->Width, img2->Height) * img2->Depth;
										D(ADDAPPICON,bug("imgsize2: %ld bytes\n", imgsize2));
									}

									if (!img2 || (msg->msg.diskobj->diskobj.do_Gadget.SelectRender = AllocPooled(pool, sizeof(struct Image))))
									{
										if (img2)
										{
											memcpy(msg->msg.diskobj->diskobj.do_Gadget.SelectRender, diskobj->do_Gadget.SelectRender, sizeof(struct Image));
										}

										if (!img2 || (((struct Image *)msg->msg.diskobj->diskobj.do_Gadget.SelectRender)->ImageData = AllocPooled(pool, imgsize2)))
										{
											struct IpcData *ipcdata;
											ULONG error = 0;

											if (img2->ImageData)
											{
												memcpy(((struct Image *)msg->msg.diskobj->diskobj.do_Gadget.SelectRender)->ImageData, img2->ImageData, imgsize2);
											}

											if (ISOWN(diskobj) && ((struct OwnDiskObject *)diskobj)->path)
											{
												msg->msg.diskobj->ownptr = msg->msg.diskobj;
												msg->msg.diskobj->path = AllocPooled(pool, strlen(((struct OwnDiskObject *)diskobj)->path) + 1);
												if (msg->msg.diskobj->path == NULL)
													error = 1;
												else
													strcpy(msg->msg.diskobj->path, ((struct OwnDiskObject *)diskobj)->path);
											}

											if (error == 0)
											{
												D(ADDAPPICON,bug("now sending message..\n"));

												/*
												 * Phew.. now we can send that to Ambient.
												 */

												D(ADDAPPICON,bug("init gmsg\n"));
												msg->gmsg.pool             = pool;
												msg->gmsg.msgtype          = &msg->msg;

												D(ADDAPPICON,bug("init msg\n"));
												msg->msg.id                = id;
												msg->msg.userdata          = userdata;
												msg->msg.userport          = msgport;

												Forbid();
												ipcdata = (struct IpcData*)FindSemaphore("Ambient IPC");
												if (ipcdata != NULL)
												{
													struct IpcAppiconNode *n = AllocPooled(pool, sizeof(*n));
													ObtainSemaphore(&ipcdata->sema);
													Permit();

													if (n != NULL)
													{
														n->entry = (struct ipcmessage*)msg;
														n->newentry = TRUE;
														n->removedentry = FALSE;
														n->loadedentry = FALSE;
														ADDTAIL(&ipcdata->appicon_list, n);
													}

													ReleaseSemaphore(&ipcdata->sema);
													Signal(ipcdata->task, 1UL << ipcdata->sigbit);
													return ((struct AppIcon *)msg);
												}
											}
											Permit();
										}
									}
								}
							}
						}
					}
				}
			}
			DeletePool(pool);
		}
	}
	return (0);
}


/****** workbench.library/RemoveAppIcon() ****************************
*
* NAME
*   RemoveAppIcon -- remove an application icon from desktop.
*
* SYNOPSIS
*   Success = RemoveAppIcon(AppIcon)
*
*   BOOL RemoveAppIcon(APTR);
*
* FUNCTION
*   Remove an application icon from desktop.
*
*   You MUST check the AppMessage port after removal.
*
* INPUTS
*   AppIcon  - an AppIcon handle returned by AddAppIconA().
*
* RESULT
*   Success  - TRUE if the icon was removed or FALSE if an error occured.
*
* SEE ALSO
*   AddAppIconA()
*
*****************************************************************************
*
*/
BOOL RemoveAppIcon(struct AppIcon *appIcon)
{
	if (appIcon)
	{
		struct appiconmsg *msg = (struct appiconmsg *)appIcon;
		struct IpcData *ipcdata;

		Forbid();

		ipcdata = (struct IpcData*)FindSemaphore("Ambient IPC");
		if (ipcdata != NULL)
		{
			/*
			 * iterate shared icon list. when entry is found, dispose it and mark node as deleted.
			 * if icon was already added, node itself will be deleted from ambient after icon is removed.
			 * otherwise delete the node immediately.
			 */

			struct IpcAppiconNode *n;

			ObtainSemaphore(&ipcdata->sema);
			Permit();

			ITERATELIST(n, &ipcdata->appicon_list)
			{
				if (n->entry == (struct ipcmessage*)appIcon)
				{
					D(REMOVEAPPICON,bug("IPC:Remove appicon:0x%x, %s\n", n->entry, msg->msg.text));
					if (n->newentry == TRUE)
					{
						/* node wasn't added to view yet, so don't try to do that and dispose node here. */
						REMOVE(n);
						DeletePool(msg->gmsg.pool);
					}
					else
					{
						/* can't deallocate memory here as ambient is using it. memory will be disposed in ambient thread. */
						n->removedentry = TRUE;
					}
					break;
				}
			}

			ReleaseSemaphore(&ipcdata->sema);
			Signal(ipcdata->task, 1UL << ipcdata->sigbit);
			return (0);
		}
		Permit();
	}
	return (0);
}

