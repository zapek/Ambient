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
 * $Id: main.c,v 1.13 2009/09/27 19:18:06 kiero Exp $
 */

#include "ambient.h"

/* public */
#include <dos/dostags.h>
#include <prefs/screenmode.h>
#include <proto/commodities.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <exec/resident.h>
#include <exec/execbase.h>

/* private */
#include "ambient_cat.h"
#include "broker.h"
#include "debug.h"
#include "init.h"
#include "methodstack.h"
#include "mui_func.h"
#include "prefs_startup.h"
#include "prefs_advanced.h"
#include "threads.h"
#include "wbstart.h"
#include "screen.h"
#include "args.h"
#include "smartreq.h"
#include "dosnotify.h"
#include "errorreq.h"
#include "ipc.h"
#include "file_func.h"
#include "cx.h"
#include "rexx.h"
#include "libs.h"
#include "strap.h"
#include "serial.h"
#include "playsound.h"


/*
 * Startup code var
 */
long __stack = STACKSIZE;

APTR app;
ULONG EnableExtensions = FALSE;

static ULONG fullsigs;

/*
 * Event handling loop.
 */
static void doloop(APTR app)
{
	ULONG sigs = 0;
	LONG id;
	ULONG done = FALSE;
	ULONG do_quit = FALSE;

	while (!done)
	{
		id = DoMethod(app, MUIM_Application_NewInput, &sigs);

		/* unlock the screen after we enter main loop (it's safe to call it more than once */
		get_screen_release();

		methodstack_check(FALSE);

		switch (id)
		{
			case MUIV_Application_ReturnID_Quit:
				{
					#if USE_STUNTZIHACK
					{
						TEXT buf[2];

						if ((GetVar("stuntzi",         buf, sizeof(buf), GVF_GLOBAL_ONLY) != -1) ||
							(GetVar("AMBIENTQUITHACK", buf, sizeof(buf), GVF_GLOBAL_ONLY) != -1))
						{
							castrate_wbstartport();
							if (DoMethod(app, MM_Application_Cleanup))
							{
								done = TRUE;
							}
							else
							{
								do_quit = TRUE;
							}
							break;
						}
					}
					#endif

					if (args.wbstartup)
					{
						/*
						 * User is in WBR mode. Ask what to do (shutdown, etc..)
						 */
						struct Task *t;

						Forbid();
						if ( (t = FindTask("ColdRebootPatch")) )
						{
							Signal(t, SIGBREAKF_CTRL_C);
						}
						Permit();

						/*
						 * We check dynamically for MOS 1.5 to enable shut down option on the fly (geit)
						 */
						if ((SysBase->LibNode.lib_Version == 50 &&
						 SysBase->LibNode.lib_Revision >= 60) ||
						SysBase->LibNode.lib_Version > 50)
						{
							smartreq_request(app, NULL, NULL, MM_Application_Shutdown, 0, GSI(MSG_SHUTDOWN_SHUTDOWN_REBOOT_CANCEL), MV_Notification_Help, GSI(MSG_SHUTDOWN_SELECT), NULL);
						}
						else
						{
							smartreq_request(app, NULL, NULL, MM_Application_Shutdown, 0, GSI(MSG_SHUTDOWN_REBOOT_CANCEL), MV_Notification_Help, GSI(MSG_SHUTDOWN_SELECT), NULL);
						}
					}
					else
					{
						/*
						 * User is in non-WBR mode.
						 */
						if (init_preclose())
						{
							if (DoMethod(app, MM_Application_Cleanup))
							{
								done = TRUE;
							}
							else
							{
								do_quit = TRUE;
							}
							break;
						}
					}
				}
				break;
		}

		if (do_quit)
		{
			if (DoMethod(app, MM_Application_Cleanup))
			{
				done = TRUE;
			}
		}

		if (done)
		{
			break;
		}

		if (sigs)
		{
			sigs = Wait(sigs | fullsigs | exsig | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_E);

			if (sigs & threadsig)
			{
				threads_handle();
			}

			if (sigs & wbstartsig)
			{
				wbstart_handle();
			}

			if (sigs & SIGBREAKF_CTRL_C)
			{
				DoMethod(app, MUIM_Application_PushMethod, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
			}

			if (sigs & ipcsig)
			{
				ipc_handle();
			}

			#if USE_DOSNOTIFY
			if (sigs & dosnotifysig)
			{
				dosnotify_handle();
			}
			#endif

			if (sigs & cxsig)
			{
				cx_handle();
			}

			if (sigs & exsig)
			{
				exchange_handle();
			}

			#if USE_REXX
			if (sigs & rexxsig)
			{
				rexx_handle();
			}
			#endif

			if (sigs & SIGBREAKF_CTRL_D)
			{
				sprefs_resetsave();
			}
		}
		methodstack_check(FALSE);
	}
	methodstack_kill_methods(app); /* XXX: sure ? */

}

LONG zmain(STRPTR arg, ULONG arglen);
LONG zmain(STRPTR arg, ULONG arglen)
{
	ULONG spawn_shell = TRUE;
	struct Screen *screen;

	D(INIT,bug("single mode\n"));

	if (init_open())
	{
		if (args_start(arg, arglen))
		{
			ULONG oldpri;

			if (!args.wbstartup)
			{
				spawn_shell = FALSE;
			}

			/*
			 * Detect presence of MorphOS >1.4.x.
			 */

			{
				struct Resident *MyResident;

				MyResident = FindResident( "MorphOS" );

				if ( MyResident && ( MyResident->rt_Version > 1 || ( (MyResident->rt_Flags & RTF_EXTENDED) && MyResident->rt_Version == 1 && MyResident->rt_Revision > 4 ) ) )
				{
					EnableExtensions = TRUE;
				}
			}

			/*
			 * Set a default priority of 1 (performance issue).
			 */
			oldpri = SetTaskPri(FindTask(NULL), 1);

restart:
			/* get screen and hold it locked until we enter main loop. */
			if ( (screen = get_screen_hold()) )
			{
				if (GetBitMapAttr(screen->RastPort.BitMap, BMA_DEPTH) > 8)
				{
					app = NewObject(getappclass(), NULL,
					End;

					if (app)
					{
						CxObj *broker;
						struct MsgPort *broker_mp;
						BOOL havebrokerhook = FALSE;

						broker    = (APTR)getv(app, MUIA_Application_Broker);
						broker_mp = (APTR)getv(app, MUIA_Application_BrokerPort);
						if (broker && broker_mp)
						{
							STRPTR hotkey;

							if (EnableExtensions)
							{
								CxObj *sender;

								if ((sender = CxSender(broker_mp, EVT_EVENTSENDER)))
								{
									AttachCxObj(broker, sender);
									havebrokerhook = TRUE;
								}
							}

							hotkey = _ap_string(exchangehotkey);
							if (hotkey[0])
							{
								CxObj *filter;
								UBYTE tmp[7 + strlen(hotkey) + 1]; /* "rawkey " */

								strcpy(tmp, "rawkey ");
								strcat(tmp, hotkey);

								filter = CxFilter(tmp);
								AttachCxObj(filter, CxSender(broker_mp, EVT_EXCHANGE_HOTKEY));
								AttachCxObj(filter, CxTranslate(NULL));
								if (!CxObjError(filter))
								{
									AttachCxObj(broker, filter);
									havebrokerhook = TRUE;
								}
								else
								{
									DeleteCxObjAll(filter);
								}
							}
						}

						if (havebrokerhook)
						{
							set(app, MUIA_Application_BrokerHook, &BrokerHook);
						}

						if (strap_start_single())
						{
							spawn_shell = FALSE;

							/*
							 * Build sigs.
							 */
							fullsigs = threadsig;
							fullsigs |= wbstartsig;
							#if USE_DOSNOTIFY
							fullsigs |= dosnotifysig;
							#endif
							fullsigs |= ipcsig;
							fullsigs |= cxsig;
							#if USE_REXX
							fullsigs |= rexxsig;
							#endif

							D(INIT,bug("switched to multitasking mode\n"));

							strap_start_multi();

							PLAYSFX(boot);

							/* doloop will unlock the screen as soon as possible */

							doloop(app);
						}
						else
						{
							/* XXX: error (beware of keyfile stuff) */
						}
						MUI_DisposeObject(app);
					}
					else
					{
						PDB(("Couldn't initialize app for some reasons\n"));
						/* XXX: error */
					}

					get_screen_release();
					SetTaskPri(FindTask(NULL), oldpri);
				}
				else
				{
					/* screen doesn't meet the requirements */

					get_screen_release();
					switch ((LONG)errorreq("init", GSI(MSG_MAIN_SCREENMODE), GSI(MSG_MAIN_SCREENMODE_ACTION)))
					{
						case 1:
							{
								BPTR l;
								ULONG rc;

								if ( (l = Lock("MOSSYS:Prefs/Preferences", ACCESS_READ)) )
								{
									UnLock(l);
									rc = systemtags("MOSSYS:Prefs/Preferences MOSSYS:Prefs/mprefs/ScreenMode.mprefs",
										SYS_Asynch, FALSE,
										SYS_Input, NULL,
										SYS_Output, NULL,
										NP_Priority, 0,
									TAG_DONE);
									
									if (rc) /* failed */
									{
										errorreq("init", GSI(MSG_MAIN_PREFERENCES_START_ERROR), GSI(MSG_MAIN_RUN_SHELL));
									}
								}
								
								if (l)
								{
									if (!rc) /* success */
									{
										goto restart;
									}
								}
								else
								{
									errorreq("init", GSI(MSG_MAIN_PREFERENCES_FIND_ERROR), GSI(MSG_MAIN_RUN_SHELL));
								}

							}
							break;

						case 2:
							/* a shell will be spawned then */
							break;

						default:
							/* let go.. */
							break;
					}
				}
			}
			else
			{
				SERIAL_PRINTF("Hello, Ambient here. I couldn't open a screen so I'm trying to\n"
							  "communicate through the serial port. Hopefully someone can see this.\n\n"
							  "Please check the following:\n"
							  "- working and supported graphics card\n"
							  "- latest version of the boot.img kernel\n"
							  "- booting from the correct device\n"
							  "- driver installed in MOSSYS:Devs/Monitors\n\n"
							  "If it still doesn't work, seek help on the MorphOS support mailing-lists\n"
							  "or contact support@morphos-team.net\n"
				);
				spawn_shell = FALSE;
			}
			args_end();
		}
	}
	
	if (spawn_shell)
	{
		/*
		 * We didn't manage to run. Try to run a shell so
		 * that the user at least has something.
		 */
		if (!DOSBase)
		{
			DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 50);
		}
		if (DOSBase)
		{
			systemtags("newshell SYSCON:",
				SYS_Asynch, TRUE,
				SYS_Input, NULL,
				SYS_Output, NULL,
				NP_Priority, 0,
			TAG_DONE);
		}
	}
	init_close();

	return (0);
}
