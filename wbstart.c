/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2013 Ambient Open Source Team
 * All Rights Reserved
 *
 * Largely inspired from Ralph Babel's Guru book and
 * Stefan Becker's wbstart.library
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
 * $Id: wbstart.c,v 1.16.6.3 2025/01/09 17:00:23 piru Exp $
 */

#include "ambient.h"

/* public */
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>

/* private */
#include "ambient_cat.h"
#include "wbstart.h"
#include "path.h"
#include "mui_func.h"
#include "methodstack.h"
#include "name.h"
#include "iconio.h"
#include "threads.h"
#include "ipc.h"
#include "prefs.h"
#include "file_func.h"
#include "smartreq.h"
#include "qport.h"

/* use compatible way of locating relative defauil tool */
/* fix to https://bugtracker.morphos.net/view.php?id=7355 */
#define USE_COMPAT_TOOLBASEDIR 1

static ULONG running_wbproc;

static struct MsgPort *wbstartport;

ULONG wbstartsig;

struct wbrun {
	struct WBStartup wbstartup;
	BPTR homedir;
	ULONG stacksize;
	ULONG free_homedir;
	ULONG pri;
	ULONG launchtype; /* WB, Shell, script or Arexx */
	LONG numarg_copy; /* in case app changes the passed message */
	struct WBArg *currentarg;
};

#define WBLT_WB      0
#define WBLT_TOOLCLI 1
#define WBLT_REXX    2
#define WBLT_SCRIPT  3
#define WBLT_CLI     4

/*
 * Hack: fake them as a type
 */
#define WBCLI 64
#define WBREXX 65

static const struct TagItem proctags[] = {
	{ NP_FreeSeglist,        FALSE },
	{ NP_Input      , (IPTR) NULL  },
	{ NP_CloseInput ,        FALSE },
	{ NP_Output     , (IPTR) NULL  },
	{ NP_CloseOutput,        FALSE },
	{ NP_Error      , (IPTR) NULL  },
	{ NP_CloseError ,        FALSE },
	{ NP_CurrentDir , (IPTR) NULL  },
	{ NP_ConsoleTask, (IPTR) NULL  },
	{ NP_WindowPtr  , (IPTR) NULL  },
	{ TAG_DONE      , (IPTR) NULL  }
};


ULONG wbstart_init(void)
{
	if ( (wbstartport = CreateMsgPort()))
	{
		wbstartsig = 1L << wbstartport->mp_SigBit;

		return (TRUE);
	}
	return (FALSE);
}


void wbstart_cleanup(void)
{
	if (wbstartport)
	{
		#if USE_STUNTZIHACK
		if (!wbstartport->mp_SigTask)
		{
			return;
		}
		#endif
		/* XXX: remove everything */
		ASSERT(!running_wbproc);
		DeleteMsgPort(wbstartport);
	}
}


static CONST CONST_STRPTR tta[] = {
	"TOOLPRI",
	"CLI",
	"REXX",
	NULL
};


/*
 * The following is needed because methodstack_push_sync() uses
 * tc_UserData and this call is done on the wbstart.library's
 * caller context.
 */

/*
 * Sends a message using the IPC to create an icon and
 * get its properties.
 */
static ULONG ipc_get_icon_properties(STRPTR filename, ULONG *type, LONG *pri, ULONG *stacksize, STRPTR defaulttool)
{
	struct MsgPort mp;
	ULONG retval = FALSE;
	struct {
		struct ipcmessage     ipm;
		struct ipc_wbstartlib wbsm;
	} *msg;

	ASSERT(filename);
	ASSERT(type);
	ASSERT(pri);
	ASSERT(stacksize);

	D(WBSTART,bug("wbstart.library induced icon property.. trying to get them..\n"));

	if ( (msg = malloc(sizeof(*msg))))
	{
		struct MsgPort *amp;

		/* no need to Forbid().. wbstart.library is created by Ambient */
		if ( (amp = FindPort("Ambient IPC")))
		{
			CreateQPort(&mp);

			msg->ipm.type             = IPC_WBSTARTLIB;
			msg->ipm.msg.mn_ReplyPort = &mp;
			msg->ipm.msg.mn_Length    = sizeof(msg->ipm);

			msg->ipm.msgtype          = &msg->wbsm;

			msg->wbsm.filename        = filename;
			msg->wbsm.type            = type;
			msg->wbsm.pri             = pri;
			msg->wbsm.stacksize       = stacksize;
			msg->wbsm.defaulttool     = defaulttool;
			msg->wbsm.status          = FALSE;

			PutMsg(amp, &msg->ipm.msg);

			D(WBSTART,bug("port found, message sent\n"));

			WaitPort(&mp);
			(void) GetMsg(&mp);

			DeleteQPort(&mp);

			D(WBSTART,bug("got reply, status: %ld\n", msg->wbsm.status));
			retval = msg->wbsm.status;
		}
		#ifdef DEBUG
		else
		{
			PDB(("can't happen (tm)\n"));
		}
		#endif
		free(msg);
	}

	return (retval);
}


/*
 * Is called by the ICP and dispatches a thread.
 */
void launch_get_icon_properties(struct ipcmessage *msg)
{
	do_action(NULL, TA_Icon_GetProperties, TT_Icon_GetProperties_Message, msg, TAG_DONE);
}


/*
 * Does the actual work.
 */
ULONG get_icon_properties(CONST_STRPTR filename, ULONG *type, LONG *pri, ULONG *stacksize, STRPTR defaulttool)
{
	APTR o;
	ULONG retval = FALSE;

	D(WBSTART,bug("hm, about to create icon, name is <%s>..\n", filename));
	if ( (o = (APTR)methodstack_push_sync(app, 3, MM_Application_CreateIcon, FALSE, MV_ViewID_Unknown)))
	{
		STRPTR n;

		D(WBSTART,bug("icon created\n"));

		if ( (n = name_build_info(filename)) )
		{
			if (icon_read(n, o, ICONTAG_Position, 0, ICONTAG_End, 0, ICONTAG_GetImage, 0, TAG_DONE))
			{
				ULONG launch_cli;
				ULONG launch_rexx;

				methodstack_push(o, 3,
					OM_GET, MA_Icon_Type, type
				);

				methodstack_push(o, 3,
					OM_GET, MA_Icon_StackSize, stacksize
				);

				methodstack_push_sync(o, 5, MM_Icon_GetToolTypes,
					tta,
					pri,
					&launch_cli,
					&launch_rexx
				);

				if (*pri < -128 || *pri > 127)
				{
					*pri = 0;
				}

				if (launch_cli)
				{
					*type = WBCLI;
				}
				else if (launch_rexx)
				{
					*type = WBREXX;
				}

				if (defaulttool)
				{
					STRPTR dt;

					D(WBSTART,bug("icon has a default tool\n"));

					if (*type == WBPROJECT || *type == WBCLI) /* XXX: probably add WBREXX as well */
					{
						methodstack_push_sync(o, 3,
							OM_GET, MA_Icon_DefaultTool, &dt
						);

						if (dt)
						{
							stccpy(defaulttool, dt, PATH_SIZE);
						}
						else
						{
							*defaulttool = 0x00;
						}
					}
					else
					{
						*defaulttool = 0x00;
					}
				}
				retval = TRUE;
			}
			/* XXX */
			name_delete(n);
		}
		methodstack_push(o, 1, OM_RELEASE);
	}
	D(WBSTART,bug("returning..\n"));
	return (retval);
}


/*
 * 'name' can have a full path.
 * Tries to Lock() <name>.elf and returns its parent lock.
 * If no .elf is present, tries to Lock() <name> and returns its parent lock.
 * This allows to have:
 *
 * - foobar.info
 * - foobar.elf
 * - foobar
 *
 * Which will make .elf executable automatically run. They must NOT
 * try to get their own name though the WB startup because it's the
 * icon name which is given. Most programs use it for GetDiskObject()
 * so it's ok.
 */
#if 0 /* XXX: needs integration.. bah */
static BPTR get_executable(STRPTR name, STRPTR *newname)
{
	BPTR l, pl;

	stccpy(*newname, name, PATH_SIZE - 5);

	len = strlen(name);
	if (len > 4 && strnicmp(".elf", &name[len - 4], 4))
	{
		strcat(*newname, ".elf");

		if (l = Lock(*newname, SHARED_LOCK))
		{
			pl = ParentDir(l);
			UnLock(l);
			return (pl);
		}
		*newname[len - 4] = '\0';
	}

	if (l = Lock(name, SHARED_LOCK))
	{
		pl = ParentDir(l);
		UnLock(l);
		return (pl);
	}

	return (NULL);
}
#endif


static ULONG scanloadpath(CONST_STRPTR name, BPTR currentdir)
{
	BPTR rc = 0;
	#if 0 /* needs some work, disabled */
	struct Segment *seg;

	/*
	 * Speed up the process by scanning for resident commands first. If
	 * the command is resident there's no point in doing any filesystem
	 * accesses as resident commands have no homedir anyway. - piru
	 */
	Forbid();
	seg = FindSegment(name, NULL, 0);
	if (!seg || (seg->seg_UC < 0 && seg->seg_UC != CMD_INTERNAL))
	{
		seg = FindSegment(name, NULL, 1);
	}
	if (!seg || (seg->seg_UC < 0 && seg->seg_UC != CMD_INTERNAL))
	{
		Permit();
	#endif

		/* check current directory */
		if ( (rc = Lock(name, ACCESS_READ)) )
		{
			D(WBSTART,bug("program <%s> found in current directory\n", name));
			UnLock(rc);
			rc = DupLock(currentdir); /* XXX: not enough memory ? */
		}
		else
		{
			STRPTR p;

			if ( (p = path_build(name)) )
			{
				if ( (rc = Lock(p, ACCESS_READ)) )
				{
					ULONG rc2;
					D(WBSTART,bug("program <%s> found in <%s>\n", name, p));
					rc2 = ParentDir(rc); /* XXX: not enough memory ? */
					UnLock(rc);
					rc = rc2;
				}
				path_free(p);
			}
		}
#if 0
	}
	else
	{
		Permit();
		D(WBSTART,bug("program <%s> is a resident\n", name));
	}
#endif

	return (rc);
}


/*
 * Adds an argument to current WBArg
 */
static ULONG fill_current_arg_x(struct wbrun *wbr, BPTR currentdir, CONST_STRPTR name, BOOL tool)
{
	struct WBArg *wa = wbr->currentarg++;
	ULONG rc = FALSE;

	if ( (wa->wa_Name = malloc(strlen(name) + 1)) )
	{
		STRPTR filepart;

		strcpy(wa->wa_Name, name);

		wbr->wbstartup.sm_NumArgs++;
		wbr->numarg_copy++;

		wa->wa_Lock = (BPTR) NULL;

		D(WBSTART,bug("wa->wa_Name = '%s'\n", wa->wa_Name));
		if ((filepart = FilePart(wa->wa_Name)) == (STRPTR)wa->wa_Name)
		{
			/* no path, just duplicate current dir lock */
			rc = wa->wa_Lock = DupLock(currentdir);
		}
		else
		{
			TEXT savechar;
#if USE_COMPAT_TOOLBASEDIR
			BPTR prevdir;
#endif

			D(WBSTART,bug("name has a path\n"));

			savechar = *filepart;
			*filepart = '\0';

			//D(WBSTART,bug("wa->wa_Name = '%s'\n", wa->wa_Name));
#if USE_COMPAT_TOOLBASEDIR
			/*
			* If this is a Default Tool, make sure we change directory
			* to the "project" base directory before attempting to
			* resolve the path. This is the 2nd part of MT#7355 fix.
			*/
			if (tool)
			{
				prevdir = CurrentDir(currentdir);
			}
#endif
			rc = wa->wa_Lock = Lock(wa->wa_Name, SHARED_LOCK);

			*filepart = savechar;
			//D(WBSTART,bug("wa->wa_Name = '%s'\n", wa->wa_Name));

			if (rc)
			{
				BPTR filelock;
				BPTR parentlock = 0;

				if ( (filelock = Lock(wa->wa_Name, SHARED_LOCK)) )
				{
					rc = parentlock = ParentDir(filelock);

					UnLock(filelock);
				}

				if (rc &&
					((*(filepart - 1) == '/') || /* path has at least one directory ? */
					(parentlock &&               /* path is a device/volume/assign. is the parent lock valid ? */
					(SameLock(parentlock, wa->wa_Lock) == LOCK_SAME)))) /* and parentdir lock is the same as the path lock ? */
				{
					D(WBSTART,bug("path contains a directory or no multi-assign\n"));
					strcpy(wa->wa_Name, filepart); /* valid pathlock.. delete the path */
				}

				UnLock(parentlock);
			}
#if USE_COMPAT_TOOLBASEDIR
			if (tool)
			{
				CurrentDir(prevdir);
			}
#endif
		}
	}

	return (rc);
}

static ULONG fill_current_arg(struct wbrun *wbr, BPTR currentdir, CONST_STRPTR name)
{
	return fill_current_arg_x(wbr, currentdir, name, FALSE);
}

#if USE_COMPAT_TOOLBASEDIR
static ULONG fill_current_arg_tool(struct wbrun *wbr, BPTR toolbasedir, CONST_STRPTR name)
{
	return fill_current_arg_x(wbr, toolbasedir, name, TRUE);
}
#endif

static ULONG wbr_fillcli_x(struct wbrun *wbr, BPTR currentdir, CONST_STRPTR name, BOOL tool)
{
	D(WBSTART,bug("setting up CLI path for <%s>..\n", name));
	if (fill_current_arg_x(wbr, currentdir, name, tool))
	{
		if (FilePart(name) != name)
		{
			/* full path */
			D(WBSTART,bug("full path specified, using that..\n"));
#if USE_COMPAT_TOOLBASEDIR
			if (tool)
			{
				BPTR prevdir = CurrentDir(currentdir);
				BPTR toollock = Lock(name, SHARED_LOCK);
				CurrentDir(prevdir);
				if (toollock)
				{
					wbr->homedir = ParentDir(toollock);
					UnLock(toollock);
					if (wbr->homedir)
					{
						wbr->free_homedir = TRUE;
						return (TRUE);
					}
				}
				return (FALSE);
			}
#endif
			if (wbr->launchtype == WBLT_CLI)
			{
				D(WBSTART,bug("pure cli mode, using the shell's current dir as homedir\n"));
				wbr->homedir = currentdir;
			}
			else
			{
				D(WBSTART,bug("using the program's location as homedir\n"));
				wbr->homedir = wbr->wbstartup.sm_ArgList->wa_Lock;
			}
		}
		else
		{
			D(WBSTART,bug("trying to find out homedir..\n"));
			if ( (wbr->homedir = scanloadpath(name, currentdir)) )
			{
				D(WBSTART,bug("homedir found\n"));
				wbr->free_homedir = TRUE;
			}
		}
		return (TRUE);
	}
	return (FALSE);
}

static ULONG wbr_fillcli(struct wbrun *wbr, BPTR currentdir, CONST_STRPTR name)
{
	return wbr_fillcli_x(wbr, currentdir, name, FALSE);
}

#if USE_COMPAT_TOOLBASEDIR
static ULONG wbr_fillcli_tool(struct wbrun *wbr, BPTR toolbasedir, CONST_STRPTR name)
{
	return wbr_fillcli_x(wbr, toolbasedir, name, TRUE);
}
#endif

static ULONG wbr_fillscript(struct wbrun *wbr, BPTR currentdir, CONST_STRPTR name)
{
	D(WBSTART,bug("setting up script path for <%s>..\n", name));

	if (fill_current_arg(wbr, currentdir, name))
	{
		if (FilePart(name) != name)
		{
			/* full path */
			D(WBSTART,bug("full path specified, using that..\n"));
			wbr->homedir = wbr->wbstartup.sm_ArgList->wa_Lock;
		}
		else
		{
			D(WBSTART,bug("trying to find out homedir..\n"));
			if ( (wbr->homedir = scanloadpath(name, currentdir)) )
			{
				D(WBSTART,bug("homedir found\n"));
				wbr->free_homedir = TRUE;
			}
		}
		return (TRUE);
	}
	return (FALSE);
}


static ULONG build_path_from_parent(BPTR pl, CONST_STRPTR name, STRPTR buf, ULONG buflen)
{
	if (NameFromLock(pl, buf, buflen))
	{
		if (AddPart(buf, name, buflen))
		{
			return (TRUE);
		}
	}
	return (FALSE);
}


/*
 * Tries to figure out how to run the program using
 * its icon, etc..
 */
static ULONG wbload_program(ULONG fromlib, struct wbrun *wbr, BPTR currentdir, CONST_STRPTR name)
{
	TEXT buf[PATH_SIZE];
	TEXT icon_dt[PATH_SIZE];
	BPTR oldlock;
	BPTR l = 0;
	ULONG rc = FALSE;
	ULONG icon_type;
	LONG icon_pri;
	ULONG icon_stack;
	STRPTR n;
#if USE_COMPAT_TOOLBASEDIR
	BPTR toolbasedir = 0;
#endif

	/* go to the program's directory */
	oldlock = CurrentDir(currentdir);

	/* try to lock the icon */
	D(WBSTART,bug("trying to lock icon..\n"));
	if ( (n = name_build_info(name)) )
	{
		D(WBSTART,bug("trying to lock <%s>..\n", n));
		if ( (l = Lock(n, ACCESS_READ)) ) /* XXX: might break with .elf */
		{
#if USE_COMPAT_TOOLBASEDIR
			/*
			* Figure out the tool root dir in case icon_dt is used. This is
			* used later to determine the tool location. This is the first
			* part of the MT#7355 fix.
			*/
			toolbasedir = ParentDir(l);
#endif
			NameFromLock(l, buf, sizeof(buf)); /* XXX: quick hack for read_icon().. */
			D(WBSTART,bug("resulting lockname: <%s>\n", buf));
			UnLock(l);
		}
		name_delete(n);
	}

	D(WBSTART,bug("end of locking attempt\n"));

	if (l && (!fromlib ? get_icon_properties(buf, &icon_type, &icon_pri, &icon_stack, icon_dt) : ipc_get_icon_properties(buf, &icon_type, &icon_pri, &icon_stack, icon_dt)))
	{
		wbr->stacksize = max(wbr->stacksize, icon_stack);
		wbr->pri = icon_pri;
		D(WBSTART,bug("icon properties: name <%s>, type %ld, pri %ld, stack %ld, defaulttool <%s>\n", name, icon_type, icon_pri, icon_stack, icon_dt));

		switch (icon_type)
		{
			case WBTOOL:
				{
					BPTR l;

					D(WBSTART,bug("tool\n"));
					rc = fill_current_arg(wbr, currentdir, name);
					wbr->homedir = wbr->wbstartup.sm_ArgList->wa_Lock;

					/*
					 * Check if it's not a script.
					 */

					if ( (l = Lock(name, ACCESS_READ)) )
					{
						D_S(struct FileInfoBlock, fib);

						if (Examine(l, fib) && fib->fib_DirEntryType < 0) /* file */
						{
							if (fib->fib_Protection & FIBF_SCRIPT)
								wbr->launchtype = WBLT_SCRIPT;
							else
								wbr->launchtype = WBLT_WB;
						}
						UnLock(l);
					}
				}
				break;

			case WBPROJECT:
			case WBCLI:
				{
					STRPTR tool = icon_dt;
					D(WBSTART,bug("project or cli\n"));

					/* XXX: check if defaulttool is empty.. */
					rc = tool[0];

					if (rc)
					{
						STRPTR p = FilePart(tool);

						if (p != tool)
						{
							/* contains a relative/absolute path */
							D(WBSTART,bug("full path\n"));

							if (icon_type == WBPROJECT)
							{
#if USE_COMPAT_TOOLBASEDIR
								if ( (rc = fill_current_arg_tool(wbr, toolbasedir, tool) &&
#else
								if ( (rc = fill_current_arg(wbr, currentdir, tool) &&
#endif
								      fill_current_arg(wbr, currentdir, name)) )
								{
									wbr->homedir = wbr->wbstartup.sm_ArgList->wa_Lock;
									wbr->launchtype = WBLT_WB;
								}
							}
							else if (icon_type == WBCLI)
							{
								wbr->launchtype = WBLT_TOOLCLI;
#if USE_COMPAT_TOOLBASEDIR
								rc = wbr_fillcli_tool(wbr, toolbasedir, tool) &&
#else
								rc = wbr_fillcli(wbr, currentdir, tool) &&
#endif
								     fill_current_arg(wbr, currentdir, name);
							}
							#ifdef DEBUG
							else
							{
								PDB(("ahee.. no case for that type\n"));
							}
							#endif
						}
						else
						{
							struct WBArg *first = wbr->wbstartup.sm_ArgList;
							D(WBSTART,bug("no path. trying to find out..\n"));

							/* no path, use the project's lock as current dirlock */
							if ( (rc = (((icon_type == WBPROJECT) ? wbr_fillcli(wbr, currentdir, name) : fill_current_arg(wbr, currentdir, name)) &&
							      fill_current_arg(wbr, first->wa_Lock, tool))) )
							{
								struct WBArg temp;
								struct WBArg *second = first + 1;

								/* swap as we have to fill in the wrong order, structcopy */
								temp = *first;
								*first = *second;
								*second = temp;

								/* scan the path to get the tool's homedir */
								if ( (wbr->homedir = scanloadpath(tool, currentdir)) )
								{
									wbr->free_homedir = TRUE;
								}
								else
								{
									wbr->homedir = wbr->wbstartup.sm_ArgList->wa_Lock;
								}
								wbr->launchtype = (icon_type == WBPROJECT) ? WBLT_WB : WBLT_TOOLCLI;
							}
						}
					}
					else
					{
						/*
						 * Tool in CLI mode
						 */
						if (icon_type == WBCLI)
						{
							wbr->launchtype = WBLT_TOOLCLI;
							rc = wbr_fillcli(wbr, currentdir, name);
						}

						/*
						 * Some hack to make other icons be executed when default tool is empty.
						 */

						if (icon_type == WBTOOL || icon_type == WBPROJECT)
						{
							wbr->launchtype = WBLT_TOOLCLI;
							rc = wbr_fillcli(wbr, currentdir, name);
						}


					}
				}
				break;

			case WBREXX:
				DB(("REXX tooltype NYI\n"));
				wbr->launchtype = WBLT_REXX;
				/* XXX: set rc.. */
				break;
		}
	}
	else
	{
		D(WBSTART,bug("no icon found\n"));

		CurrentDir(currentdir);
		if ( (l = Lock(name, ACCESS_READ)) )
		{
			D_S(struct FileInfoBlock, fib);

			if (Examine(l, fib) && fib->fib_DirEntryType < 0) /* file */
			{
				if (fib->fib_Protection & FIBF_SCRIPT)
				{
					D(WBSTART,bug("it's a script (+s)\n"));
					wbr->launchtype = WBLT_SCRIPT;
					rc = wbr_fillscript(wbr, currentdir, name);
				}
			}
			/* XXX */
			UnLock(l);
		}
		/* XXX */

		if (!rc)
		{
			D(WBSTART,bug("starting in CLI mode..\n"));
			//wbr->launchtype = WBLT_CLI;
			wbr->launchtype = WBLT_TOOLCLI;
			rc = wbr_fillcli(wbr, currentdir, name);
		}
	}

#if USE_COMPAT_TOOLBASEDIR
	if (toolbasedir)
	{
		UnLock(toolbasedir);
	}
#endif

	if (rc)
	{
		switch (wbr->launchtype)
		{
			case WBLT_WB:
				D(WBSTART,bug("LoadSeg()ing program <%s>..\n", wbr->wbstartup.sm_ArgList->wa_Name));

				CurrentDir(wbr->homedir);

				if ( (rc = wbr->wbstartup.sm_Segment = NewLoadSeg(wbr->wbstartup.sm_ArgList->wa_Name, NULL)) ) /* XXX: hell.. that won't work for cli launching.. */
				{
					D(WBSTART,bug("program loaded successfully\n"));
				}
				break;
		}
	}

	CurrentDir(oldlock);

	return (rc);
}


/*
 * Copies the argument list.
 */
static ULONG copy_arguments(struct wbrun *wbr, struct WBArg *orig, ULONG count)
{
	struct WBArg *dest = wbr->currentarg;
	ULONG rc = TRUE;

	while (count--)
	{
		int len = strlen(orig->wa_Name);

		if ( (dest->wa_Name = malloc(len + 1)) )
		{
			strcpy(dest->wa_Name, orig->wa_Name);
		}
		else
		{
			rc = FALSE;
			break;
		}

		wbr->wbstartup.sm_NumArgs++;
		wbr->numarg_copy++;

		if (!(dest->wa_Lock = DupLock(orig->wa_Lock)))
		{
			if (!len || dest->wa_Name[len - 1] != ':')
			{
				rc = FALSE;
				break;
			}
		}
		orig++;
		dest++;
	}
	return (rc);
}


/*
 * Free the wbrun structure properly.
 */
static void free_wbrun(struct wbrun *wbr, ULONG seglist)
{
	struct WBArg *wa = wbr->wbstartup.sm_ArgList;
	LONG i = wbr->wbstartup.sm_NumArgs;

	if ( wbr->numarg_copy != i )
	{
		D(WBSTART,bug("Got message with modified argnum: %d->%d\n", wbr->numarg_copy, i ));
		i = wbr->numarg_copy;
	}

	while ( i > 0 )
	{
		#ifdef DEBUG
		if ( wa->wa_Lock == (BPTR)0x55555555 )
		{
			DB(("Corrupted pointer! Args left to release: %d (%d). Bailing out\n", i, wbr->wbstartup.sm_NumArgs));
			break;
		}
		#endif

		D(WBSTART,bug("unlocking (0x%x) : %s\n", wa->wa_Lock, wa->wa_Name ? (STRPTR)wa->wa_Name : (STRPTR)"Unnamed"));
		if (wa->wa_Lock) UnLock(wa->wa_Lock); // bitRocky:
		if (wa->wa_Name)
		{
			D(WBSTART,bug("freeing...\n"));
			free(wa->wa_Name);
		}
		wa++;
		i--;
	}

	if (seglist && wbr->wbstartup.sm_Segment)
	{
		D(WBSTART,bug("freeing seglist..\n"));
		UnLoadSeg(wbr->wbstartup.sm_Segment);
	}
	if (wbr->free_homedir)
	{
		D(WBSTART,bug("unlocking homedir..\n"));
		UnLock(wbr->homedir);
	}
	D(WBSTART,bug("freeing wbr..\n"));
	free (wbr);
	D(WBSTART,bug("done\n"));
}


static ULONG wbr_cat_args(struct wbrun *wbr, CONST_STRPTR from, STRPTR to, LONG tolen)
{
	UBYTE *p;
	LONG i;

	if ( tolen < 1 )
	{
		return FALSE;
	}

	p = to;
	stccpy(p, from, tolen);

	i = wbr->wbstartup.sm_NumArgs - 1;
	if ( i > 0 )
	{
		LONG len;
		struct WBArg *wa;

		len = strlen(p);
		p += len;
		tolen -= len;

		wa = wbr->wbstartup.sm_ArgList + 1;

		do
		{
			UBYTE arg[NAME_SIZE * 2]; /* Yep, arbitrary maximum buffer size. Should be ok here. - Piru */
			LONG arglen;

			name_build_readargs_quoted(wa->wa_Name, arg, sizeof(arg));
			arglen = strlen(arg);
			len = arglen + 1; /* ' ' */
			if ( len + 1 > tolen )
			{
				/* Would overflow */
				return FALSE;
			}
			tolen -= len;

			*p++ = ' ';
			memcpy(p, arg, arglen);
			p += arglen;

			wa++;

		} while ( --i );

		*p = '\0';
	}

	return TRUE;
}

/*********************/
static BOOL wbstart_getpath( CONST_STRPTR fullpath, STRPTR allocatedpath, ULONG allocatedpathsize )
{
	STRPTR pathpart_ptr;

	stccpy(allocatedpath, fullpath, allocatedpathsize);
	pathpart_ptr = PathPart( allocatedpath );

	if (pathpart_ptr)
	{
		*pathpart_ptr = '\0';
		return TRUE;
	}

	return FALSE;
}

/*
 * Runs a program in WB mode (XXX: rewrite that crap :)
 *
 * 'filename': filename of the program/project (without .info)
 * 'type': WBTOOL or WBPROJECT, 0 if unknown (will then find out everything by itself)
 * 'pri': process priority (finds out if type is 0)
 * 'stacksize': stack size (finds out if type is 0)
 *
 * XXX: does arglist work? check that..
 */

ULONG v_wbstart(CONST_STRPTR filename, struct TagItem *tags)
{
	BPTR dirlock = 0;
	ULONG fromlib = 0;
	LONG pri = 0;
	ULONG stacksize = 0;
	ULONG argcnt = 0;
	struct WBArg *wba = NULL;
	LONG wbacnt = 0;
	LONG wbaexternal = FALSE;
	ULONG rc = FALSE;

	THREAD;

	FORTAG(tags)
	{
		case WBSTARTTAG_FromLib:
			fromlib = tag->ti_Data;
			break;

		case WBSTARTTAG_DirLock:
			dirlock = tag->ti_Data;
			break;

		case WBSTARTTAG_Priority:
			pri = minmax(-128, (LONG)tag->ti_Data, 127);
			break;

		case WBSTARTTAG_Stack:
			stacksize = tag->ti_Data;
			break;

		case WBSTARTTAG_Argument:
			if ( tag->ti_Data )
			{
				argcnt++;
			}
			break;

		case WBSTARTTAG_Argument_List:
			if ( tag->ti_Data )
			{
				struct wbsnode *wbsn;
				ITERATELIST( wbsn, tag->ti_Data )
				{
					argcnt++;
				}
			}
			break;

		case WBSTARTTAG_WBArgs:
			wba = (struct WBArg *)tag->ti_Data;
			wbaexternal = TRUE;
			break;

		case WBSTARTTAG_WBArgsCount:
			wbacnt = tag->ti_Data;
			break;

		#ifdef DEBUG
		default:
			PDB(("unknown tag 0x%lx\n", tag->ti_Tag));
			break;
		#endif
	}
	NEXTTAG

	if (argcnt)
	{
		ASSERT(!wba);

		if ( (wba = malloc(argcnt * sizeof(struct WBArg))) )
		{
			TEXT p[PATH_SIZE];
			ULONG i = 0;

			FORTAG(tags)
			{
				case WBSTARTTAG_Argument:

					if ( tag->ti_Data != 0 )
					{
//PDB(("WBSTARTTAG_Argument: i=%ld, tag->ti_Data = '%s'\n", i, (STRPTR)tag->ti_Data));
						if ( wbstart_getpath( (STRPTR)tag->ti_Data, p, sizeof(p) ) )
						{
							(wba + i)->wa_Lock = Lock((p[0] == '"') ? p+1 : p, ACCESS_READ); // bitRocky: the sub mprefs could (is) quoted so check it!
//PDB(("WBSTARTTAG_Argument: i=%ld, p '%s', wa_Lock = 0x%08lx\n", i, p, (wba + i)->wa_Lock));
							(wba + i)->wa_Name = (STRPTR)tag->ti_Data;
//PDB(("WBSTARTTAG_Argument: i=%ld, wa_Name = '%s'\n", i, (wba + i)->wa_Name));
							i++;
						}
						/* XXX */
					}
					break;

				case WBSTARTTAG_Argument_List:
					if ( tag->ti_Data != 0 )
					{
						struct wbsnode *wbsn;

						ITERATELIST( wbsn, tag->ti_Data )
						{
							if ( wbstart_getpath( wbsn->arg, p, sizeof(p) ) )
							{
								(wba + i)->wa_Lock = Lock(p, ACCESS_READ);
								(wba + i)->wa_Name = wbsn->arg;
								i++;
							}
							/* XXX */
						}
					}
					break;
			}
			NEXTTAG
		}
		/* XXX */
	}
	else if ( wba && wbacnt )
	{
		/* we use wbacnt to check if releasing is wba should be done */

		argcnt = wbacnt;
	}

	running_wbproc++;


	D(WBSTART,bug("attempting to run..\n"));

	if ( (filename && *filename && argcnt < 2048) )
	{
		struct wbrun *wbr;

		stacksize = max(stacksize, M68K_STACKSIZE);

		D(WBSTART,bug("file: <%s>, argcount: %ld\n", filename, argcnt));

		if( (wbr = malloc(sizeof(*wbr) + sizeof(struct WBArg) * (argcnt + 2))) ) /* 2 arguments reserved */
		{
			wbr->wbstartup.sm_Message.mn_Node.ln_Pri = 0;
			wbr->wbstartup.sm_Message.mn_ReplyPort = wbstartport;
			wbr->wbstartup.sm_Message.mn_Length = sizeof(struct WBStartup); /* not needed but who knows if some software check that.. */
			wbr->wbstartup.sm_Segment = 0;
			wbr->wbstartup.sm_NumArgs = 0;
			wbr->wbstartup.sm_ToolWindow = NULL;
			wbr->wbstartup.sm_ArgList = (struct WBArg *)(wbr + 1);
			wbr->stacksize = stacksize;
			wbr->pri = pri;
			wbr->free_homedir = FALSE;
			wbr->launchtype = WBLT_WB;
			wbr->currentarg = wbr->wbstartup.sm_ArgList;
			wbr->numarg_copy = 0;

//PDB(("before wbload_program(): wba->wa_Name = '%s', wba->wa_Name = 0x%08lx\n", wba->wa_Name ? wba->wa_Name : "NULL", wba->wa_Name));
			if (wbload_program(fromlib, wbr, dirlock, filename))
			{

//PDB(("after wbload_program(): wba->wa_Name = '%s', wba->wa_Name = 0x%08lx\n", wba->wa_Name ? wba->wa_Name : "NULL", wba->wa_Name));
				if (wbr->launchtype == WBLT_TOOLCLI || wbr->launchtype == WBLT_CLI)
				{
					D(WBSTART,bug("file <%s> loaded.. launching as CLI mode..\n", filename));

					running_wbproc--;

					if (copy_arguments(wbr, wba, argcnt))
					{
						BPTR output;

						if ( (output = Open(_conf(cli_device), MODE_NEWFILE)) ) /* XXX: check if _conf(cli_device) is not empty.. no output in that case.. or NIL: */
						{
							BPTR input;
							struct MsgPort *old;

							old = SetConsoleTask(((struct FileHandle *)BADDR(output))->fh_Type);
							input = Open("*", MODE_OLDFILE);
							SetConsoleTask(old);
							D(WBSTART,bug("input: 0x%lx\n", input));

							if (input)
							{
								TEXT n[PATH_SIZE];

								D(WBSTART,bug("wbr->homedir 0x%lx filename <%s>\n", wbr->homedir, filename));

								if (wbr->launchtype == WBLT_CLI || !wbr->homedir)
								{
									ULONG len;

									/* in CLI mode (from shell) the path is absolute and currentdir is the shells */
									len = strlen(filename);
									if (len < sizeof(n))
									{
										memcpy(n, filename, len + 1);
										rc = TRUE;
									}
								}
								else
								{
									rc = build_path_from_parent(wbr->homedir, wbr->wbstartup.sm_ArgList->wa_Name, n, sizeof(n));
								}

								if (rc)
								{
									STRPTR t;

									rc = FALSE;

									if ( (t = name_build_readargs_quoted(n, NULL, 0)) )
									{
										TEXT ta[PATH_SIZE]; /* This buffer is quite small, should be larger IMO. - Piru */

										if (wbr_cat_args(wbr, t, ta, sizeof(ta)))
										{
											BPTR olddir;

											olddir = CurrentDir(wbr->homedir);

											D(WBSTART,bug("command name: <%s>\n", ta));

											if (!systemtags(ta,
												SYS_Asynch, TRUE,
												SYS_Input, input,
												SYS_Output, output,
												NP_StackSize, max(wbr->stacksize, _conf(cli_stack)),
												/* NP_PPCStackSize, max(wbr->stacksize, _conf(cli_stack)) * 2, */
												NP_Priority, 0,
												TAG_DONE))
											{
												rc = TRUE;
											}

											CurrentDir(olddir);
										}

										name_delete(t);
									}
								}

								if (!rc)
								{
									Close(input);
								}
							}

							if (!rc)
							{
								Close(output);
							}
						}
					}
					free_wbrun(wbr, FALSE);
					goto done;
				}
				else if (wbr->launchtype == WBLT_SCRIPT)
				{
					BPTR output;

					running_wbproc--;

					/* XXX: no argument support ? check the copy_arguments() if it's needed, etc.. */
					if (!_conf(cli_device) || !_conf(cli_device)[0] || !(output = Open(_conf(cli_device), MODE_NEWFILE)))
					{
						output = Open("NIL:", MODE_NEWFILE);
					}

					if (output)
					{
						BPTR input;
						struct MsgPort *old;

						old = SetConsoleTask(((struct FileHandle *)BADDR(output))->fh_Type);
						input = Open("*", MODE_OLDFILE);
						SetConsoleTask(old);
						D(WBSTART,bug("input: 0x%lx\n", input));

						if (input)
						{
							TEXT n[PATH_SIZE];

							if (build_path_from_parent(wbr->homedir, wbr->wbstartup.sm_ArgList->wa_Name, n, sizeof(n)))
							{
								BPTR olddir;

								D(WBSTART,bug("file <%s> to be run in script mode..\n", n));
								olddir = CurrentDir(wbr->homedir);

								rc = execute_async(n, input, output); /* XXX: add stacksize arguments.. */

								CurrentDir(olddir);
								D(WBSTART,bug("ran, status: %ld\n", rc));
							}

							if (!rc)
							{
								Close(input);
							}
						}

						if (!rc)
						{
							Close(output);
						}
					}
					free_wbrun(wbr, FALSE);
					goto done;
				}
				#if 0
				else if (wbr->launchtype == WBLT_REXX)
				{
					running_wbproc--;

					/* XXX: no clue about the output */


				}
				#endif
				else
				{
					D(WBSTART,bug("file <%s> loaded\n", filename));
//PDB(("before copy_arguments(): wba->wa_Name = '%s'\n", wba->wa_Name ? wba->wa_Name : "NULL"));
					if (copy_arguments(wbr, wba, argcnt))
					{
						BPTR homelock;

						D(WBSTART,bug("arguments copied\n"));

						/* We shouldnt set NP_PPCStackSize here but unfortunately some 68k programs (namely IBrowse)
						 * tend to run out of stack with JIT. To do it properly we should check is binary PPC native
						 * and ignore stacksize setting but oh well...
						 */

						if ( (homelock = DupLock(wbr->homedir)) )
						{
							struct Process *pr;

							if ( (pr = CreateNewProcTags(
								NP_Seglist, wbr->wbstartup.sm_Segment,
								NP_StackSize, wbr->stacksize,
								NP_PPCStackSize, wbr->stacksize,
								NP_Name, wbr->wbstartup.sm_ArgList->wa_Name,
								NP_Priority, wbr->pri,
								NP_HomeDir, homelock,
								TAG_MORE, proctags)) )
							{
								struct MsgPort *mp = &pr->pr_MsgPort;
								D(WBSTART,bug("new process created at %p\n", pr));

								wbr->wbstartup.sm_Process = mp;

								PutMsg(mp, (struct Message *)wbr);

								D(WBSTART,bug("startup message sent (0x%x, %d args) to 0x%x\n", wbr, wbr->wbstartup.sm_NumArgs, mp));

								rc = TRUE;
								goto done;
							}
							else
							{
								UnLock(homelock);
							}
						}
					}
				}
			}
			free_wbrun(wbr, TRUE);
		}

	}
	running_wbproc--;

	done:

	if (argcnt)
	{

		if ( !wbaexternal )
		{
			/* custom wba was allocated */

			BPTR l;

			while (argcnt--)
			{
				if( (l = (wba + argcnt)->wa_Lock))
				{
					UnLock(l);
				}
			}

			free(wba);
		}

		FORTAG(tags)
		{
			case WBSTARTTAG_Argument_List:
				{
					struct wbsnode *wbsn, *nextwbsn;

					ITERATELISTSAFE(wbsn, nextwbsn, tag->ti_Data)
					{
						free(wbsn);
					}
					free((struct MinList *) tag->ti_Data);
				}
				break;
		}
		NEXTTAG
	}

	return (rc);
}


void wbstart_handle(void)
{
	struct wbrun *wbr;

	while ( (wbr = (struct wbrun *)GetMsg(wbstartport)))
	{
		D(WBSTART,bug("incoming message..(0x%x, %d args)\n", wbr, wbr->wbstartup.sm_NumArgs));
		free_wbrun(wbr, TRUE);
		running_wbproc--;
	}
}


ULONG preclose_wbstart(void)
{
	if (running_wbproc)
	{
		smartreq_request(NULL, NULL, NULL, 0, 0, "Ok", MV_Notification_Warning,
		                 "You have %lu desktop processes running.\n"
		                 "Close them before exiting.", running_wbproc);

		return (FALSE);
	}
	return (TRUE);
}

#ifdef USE_STUNTZIHACK
void castrate_wbstartport(void)
{
	wbstartport->mp_SigTask = NULL;
}
#endif


ULONG tr_wbstart(CONST_STRPTR filename, LONG pri, ULONG stack, STRPTR arg1, struct MinList *arglist, ULONG freenames)
{
	ULONG res;

	ASSERT(filename);
	THREAD;

	res = wbstart(filename,
		WBSTARTTAG_Priority, pri,
		stack ? WBSTARTTAG_Stack : TAG_IGNORE, stack,
		arg1 ? WBSTARTTAG_Argument : TAG_IGNORE, arg1,
		arglist ? WBSTARTTAG_Argument_List : TAG_IGNORE, arglist,
	TAG_DONE);

	if (!res)
	{
		smartreq_ok(MSG_AMBIENT_REQ, MV_Notification_Error, MSG_PROGRAM_NOT_STARTED_REQ, filename);
	}

	/*
	 * Here we can free arguments (if requested).
	 */

	if ( freenames && arg1 )
		name_delete( arg1 );

	return res;
}
