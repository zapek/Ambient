/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2007 by Ilkka Lehtoranta
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
 * $Id: prefs_startup.c,v 1.4 2017/11/06 20:18:11 bitrocky Exp $
 */

#include "ambient.h"

/* public */
#include <devices/keyboard.h>
#include <exec/interrupts.h>
#include <proto/dos.h>

/* private */
#include "classes.h"
#include "methodstack.h"
#include "prefs.h"
#include "prefs_startup.h"
#include "viewapi.h"
#include "viewclass.h"

extern struct Task *MainTask;

static VOID sprefs_resethandler(void);

static const struct EmulLibEntry handlertrap = { TRAP_LIBNR, 0, &sprefs_resethandler };
static struct MsgPort port = { { NULL, NULL, NT_MSGPORT, 0, NULL }, PA_SIGNAL, SIGB_SINGLE, NULL, { (APTR)&port.mp_MsgList.lh_Tail, NULL, (APTR)&port.mp_MsgList.lh_Head, 0, 0 } };
static struct IOStdReq io;
static struct Interrupt handler = { { NULL, NULL, NT_INTERRUPT, 0, "Ambient" }, NULL, (APTR)&handlertrap };
static LONG resethandler, reset;

VOID sprefs_setup(void)
{
	MAINTASK;

	if (!resethandler)
	{
		port.mp_SigTask = FindTask(NULL);

		io.io_Message.mn_ReplyPort = &port;
		io.io_Message.mn_Length    = sizeof(io);

		if (OpenDevice("keyboard.device", 0, (struct IORequest *)&io, 0) == 0)
		{
			resethandler  = TRUE;
			io.io_Command = KBD_ADDRESETHANDLER;
			io.io_Data    = &handler;
			DoIO((struct IORequest *)&io);
		}
	}
}


VOID sprefs_cleanup(void)
{
	MAINTASK;

	if (resethandler)
	{
		resethandler = FALSE;
		io.io_Command = KBD_REMRESETHANDLER;
		io.io_Data    = &handler;
		DoIO((struct IORequest *)&io);
		CloseDevice((struct IORequest *)&io);
	}
}


ULONG tr_sprefs_load(APTR obj)
{
	APTR prefspool;
	THREAD;
	CHECKOBJECT(obj);

	prefspool = prefspool_create(0);

	if (prefspool)
	{
		if (prefspool_read(prefspool, PREFS_PATH STARTUP_FILE, STARTUPPREFSID, TRUE) == PREFSPOOL_IO_OK)
		{
			APTR pl, pi; /* list, item */

			if ((pl = prefspool_item_get(prefspool, NULL, DDSI_LISTPOOL_WINDOW, NULL, NULL)))
			{
				ULONG i = 0, slen = 0;
				STRPTR path, buf = NULL;

				while ((pi = prefspool_item_get(prefspool, pl, i | DSF_LISTPOOL, NULL, NULL)) && prefspool_item_get(prefspool, pi, DDSI_LISTPOOL_WINDOW_PATH, (APTR)&path, NULL))
				{
					ULONG flen, devstuff;

					i++;

					devstuff = !strcmp(path, "devices://");

					if (!devstuff && !_conf(misc_remember_documents))
					{
						BPTR fh = Open(path, MODE_OLDFILE);

						if (fh)
						{
							Close(fh);
							continue;
						}
					}

					flen = strlen(path) + 200;

					if (flen > slen)
					{
						if (buf)
							FreeTaskPooled(buf, slen);

						slen = flen;

						buf = AllocTaskPooled(slen);
					}

					if (buf)
					{
						CONST_STRPTR view = "", mode = "";
						LONG x = 0, y = 0, w = 128, h = 128, browser = FALSE, iconified = FALSE, *dummy;

						if (prefspool_item_get(prefspool, pi, DDSI_LISTPOOL_WINDOW_LEFT, (APTR)&dummy, NULL))
							x = *dummy;
						if (prefspool_item_get(prefspool, pi, DDSI_LISTPOOL_WINDOW_TOP, (APTR)&dummy, NULL))
							y = *dummy;
						if (prefspool_item_get(prefspool, pi, DDSI_LISTPOOL_WINDOW_WIDTH, (APTR)&dummy, NULL))
							w = *dummy;
						if (prefspool_item_get(prefspool, pi, DDSI_LISTPOOL_WINDOW_HEIGHT, (APTR)&dummy, NULL))
							h = *dummy;
						if (prefspool_item_get(prefspool, pi, DDSI_LISTPOOL_WINDOW_BROWSER, (APTR)&dummy, NULL))
							browser = *dummy;
						/* Don't try to iconify views for now, as long as these issues aren't fixed:
						 * - icons appear before shortcuts and mess layout. -> iconified view icons coordinates should be remembered
						 * - window first appears and then disappears (ugly).
						 * - listview icon thread is interrupted at iconification for some reason and doesn't resume when window is opened.
						 */
						/*
						if (prefspool_item_get(prefspool, pi, DDSI_LISTPOOL_WINDOW_ICONIFIED, (APTR)&dummy, NULL))
							iconified = *dummy;
						*/
						if (prefspool_item_get(prefspool, pi, DDSI_LISTPOOL_WINDOW_LISTER, (APTR)&dummy, NULL))
						{
							if (*dummy)
							{
								if(devstuff)
									view = "&view=DList";
								else
									view = "&view=List";
							}
							else
							{
								if(devstuff)
									view = "&view=Icons";
								else
									view = "&view=Icon";
							}
						}

						if (prefspool_item_get(prefspool, pi, DDSI_LISTPOOL_WINDOW_MODEINDEX, (APTR)&dummy, NULL))
						{
							switch (*dummy)
							{
								case LVM_ICONS:
									mode = "&mode=icons";
									break;

								case LVM_SHOWALL:
									mode = "&mode=all";
									break;

								case LVM_THUMBS:
									mode = "&mode=thumbs";
									break;
							}
						}

						snprintf(buf, slen, "LoadURI \"%s%s?left=%ld&top=%ld&width=%ld&height=%ld%s%s\" NEWWIN %s %s", devstuff ? "" : "file://", path, x, y, w, h, view, mode, iconified ? "ICONIFIED" : "", browser ? "BROWSER" : "");
						methodstack_push_sync(app, 8, MM_Application_DoRexx, TRUE, NULL, buf, NULL, NULL, 0, NULL);
					}
				}

				if (buf)
				{
					FreeTaskPooled(buf, slen);
				}
			}
		}

		prefspool_delete(prefspool);
	}

	return (TRUE);
}


VOID sprefs_save(void)
{
	APTR prefspool = prefspool_create(0);

	if (prefspool)
	{
		APTR pl, pi;

		if (!(pl = prefspool_item_get(prefspool, NULL, DDSI_LISTPOOL_WINDOW, NULL, NULL)))
		{
			pl = prefspool_item_add(prefspool, NULL, DDSI_LISTPOOL_WINDOW, NULL, 0);
		}

		if (pl)
		{
			ULONG i = 0;

			FORCHILD(app, MUIA_Application_WindowList)
			{
				if (getv(child, MA_Window_Type) == MV_Window_Type_View)
				{
					if (!(pi = prefspool_item_get(prefspool, pl, i | DSF_LISTPOOL, NULL, NULL)))
					{
						pi = prefspool_item_add(prefspool, pl, i | DSF_LISTPOOL, NULL, 0);
					}

					if (pi)
					{
						CONST_STRPTR p;
						APTR view = (APTR)getv(child, MA_Window_Viewobj);

						i++;

						p = (APTR)getv(child, MA_Window_Path);

						if (!p || *p == '\0')
							p = "devices://";

						/* kiero: use same calculations as in viewclass window snapshotting code */

						setprefsstr_lp(prefspool, pi, DDSI_LISTPOOL_WINDOW_PATH, p);
						setprefslong_lp(prefspool, pi, DDSI_LISTPOOL_WINDOW_LEFT, getv(child, MUIA_Window_LeftEdge));
						setprefslong_lp(prefspool, pi, DDSI_LISTPOOL_WINDOW_TOP, getv(child, MUIA_Window_TopEdge));
						if (view != NULL)
						{
							LONG width = _window(view)->Width;
							LONG height = _mheight(view) + (_window(view)->Height - _mbottom(view));
							setprefslong_lp(prefspool, pi, DDSI_LISTPOOL_WINDOW_WIDTH, width);
							setprefslong_lp(prefspool, pi, DDSI_LISTPOOL_WINDOW_HEIGHT, height);
						}
						else
						{
							setprefslong_lp(prefspool, pi, DDSI_LISTPOOL_WINDOW_WIDTH, getv(child, MUIA_Window_Width));
							setprefslong_lp(prefspool, pi, DDSI_LISTPOOL_WINDOW_HEIGHT, getv(child, MUIA_Window_Height));
						}
						setprefslong_lp(prefspool, pi, DDSI_LISTPOOL_WINDOW_ICONIFIED, getv(child, MA_Window_IsIconified));
						setprefslong_lp(prefspool, pi, DDSI_LISTPOOL_WINDOW_BROWSER, getv(child, MA_Window_Browser));

						if (view)
						{
							ULONG idx = getv(view, MA_Viewgroup_ViewIndex);

							setprefslong_lp(prefspool, pi, DDSI_LISTPOOL_WINDOW_LISTER, viewapi_compare_idx2name(idx, "list") || viewapi_compare_idx2name(idx, "DList"));
							setprefslong_lp(prefspool, pi, DDSI_LISTPOOL_WINDOW_MODEINDEX, getv(view, MA_Viewgroup_ViewModeIndex));
						}
					}
				}
			}
			NEXTCHILD
		}

		prefspool_write(prefspool, PREFS_PATH STARTUP_FILE, STARTUPPREFSID, FALSE);
		prefspool_delete(prefspool);
	}
}


static VOID sprefs_resethandler(void)
{
	reset = 1;
	Signal(MainTask, SIGBREAKF_CTRL_D);
}


STATIC VOID flushvolume(CONST_STRPTR path)
{
	struct DevProc *dvp;
	struct Process *proc = (APTR) FindTask(NULL);
	APTR oldwinptr;

	oldwinptr = proc->pr_WindowPtr;
	proc->pr_WindowPtr = (APTR) -1;

	dvp = GetDeviceProc(path, NULL);
	if (dvp)
	{
		DoPkt0(dvp->dvp_Port, ACTION_FLUSH);

		FreeDeviceProc(dvp);
	}

	proc->pr_WindowPtr = oldwinptr;
}

VOID sprefs_resetsave(void)
{
	if (reset)
	{
		reset = 0;
		sprefs_save();
		flushvolume(PREFS_PATH);
		io.io_Command = KBD_RESETHANDLERDONE;
		io.io_Data    = &handler;
		DoIO((struct IORequest *)&io);
	}
}
