/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2004 by David Gerber <zapek@morphos.net>
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
 * $Id: private.c,v 1.7 2006/08/08 13:31:43 fab Exp $
 */

#include "globals.h"

/* public */
#include <stdarg.h>
#include <string.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

/* private */
#include "clib/wb_protos.h"
#include "../ipc.h"
#include "qport.h"


#define DB_UPDATEWB 0
#define DB_QUOTEWB 0
#define DB_WBCONFIG 0
#define DB_STARTWB 0


/* XXX: should be shared */
#define WBB_DEBUG       0
#define WBB_DELAY       1
#define WBB_CLEANUP     2
#define WBB_NEWPATH     3
#define WBB_NOWBSTARTUP 4
#define WBB_AMBIENT     31

#define WBF_DEBUG       (1 << WBB_DEBUG)
#define WBF_DELAY       (1 << WBB_DELAY)
#define WBF_CLEANUP     (1 << WBB_CLEANUP)
#define WBF_NEWPATH     (1 << WBB_NEWPATH)
#define WBF_NOWBSTARTUP (1 << WBB_NOWBSTARTUP)
#define WBF_AMBIENT     (1 << WBB_AMBIENT)


/*
 * That one is used by the original icon.library
 * to tell if an icon is added or removed.
 */
void UpdateWorkbench(STRPTR name, BPTR lock, LONG flag)
{
	D(UPDATEWB,bug("called\n"));
}


/*
 * That one is probably used for locale crapola.
 */
void QuoteWorkbench(void)
{
	D(QUOTEWB,bug("called\n"));
}

struct WBFontPrefs
{
	struct TextAttr *TextAttrPtr;
	struct TextFont *TextFont;
	UWORD DrawMode;
	UBYTE FrontPen;
	UBYTE BackPen;
	struct TextAttr TextAttr;
	UBYTE Name[1];
};



/*
 * Takes different parameters to update internal settings.
 * It returns the previous context so, for example it would
 * return the old WBFontPrefs when asked to set a new one, etc..
 */
LONG WBConfig(ULONG type, ULONG wbp)
{
	/*
	 * type:
	 *
	 * - 1: Tells the Workbench to update its pattern (WBP_ROOT).
	 *      wbp: a BitMap structure or NULL for no bitmap.
	 *
	 * - 2: Tells the Workbench to update its pattern (WBP_DRAWER).
	 *      wbp: a BitMap structure or NULL for no bitmap.
	 *
	 * - 3: Tells the Workbench to update its icon font (proportional) (FP_WBFONT).
	 *      wbp: WBFontPrefs structure.
	 *
	 * - 4: Tells the Workbench to update its lister font (fixed) (FP_SYSFONT).
	 *      wbp: WBFontPrefs structure.
	 *
	 * - 5: Tells the Workbench to update its locale settings.
	 *      wbp: NULL
	 */
	D(WBCONFIG,bug("called\n"));

	switch (type)
	{
		case 3:
		case 4:
			/*
			 * Tell IPrefs to fuck off and free the
			 * structure it just sent.
			 */
			return (wbp);
	}
	return (0);
}


static LONG errorreq(CONST_STRPTR body, CONST_STRPTR gadgets, ...)
{
	LONG res;
	struct EasyStruct es;
	va_list va;

	es.es_StructSize   = sizeof(struct EasyStruct);
	es.es_Flags        = 0;
	es.es_Title        = "Ambient error";
	es.es_TextFormat   = (STRPTR)body;
	es.es_GadgetFormat = (STRPTR) (gadgets ? gadgets : (CONST_STRPTR)"Ok");

	va_start(va, gadgets);

	res = EasyRequestArgs(NULL, &es, NULL, va->overflow_arg_area);

	va_end(va);

	return (res);
}


static ULONG checkpath(CONST_STRPTR name, BPTR *cdp)
{
	if (name && *name)
	{
		BPTR cd;

		if ( (cd = Lock(name, ACCESS_READ)) )
		{
			UBYTE _fib[sizeof(struct FileInfoBlock) + 3];
			struct FileInfoBlock *fib = (APTR) ((IPTR)(_fib + 3) & ~3);

			if (Examine(cd, fib) &&
			    fib->fib_DirEntryType > 0) /* must be dir */
			{
				BPTR olddir;
				BPTR l;

				olddir = CurrentDir(cd);
				l = Lock("Ambient", ACCESS_READ);
				(void) CurrentDir(olddir);
				if (l)
				{
					LONG res;

					res = Examine(l, fib);
					UnLock(l);

					if (res &&
					    fib->fib_DirEntryType < 0) /* must be file */
					{
						*cdp = cd;
						return (TRUE);
					}
				}

			}
			UnLock(cd);
		}
	}
	return (FALSE);
}


static void free_path(BPTR path)
{
	struct pathentry {
		BPTR pe_next;
		BPTR pe_lock;
	} *pe = BADDR(path), *peo;

	while (pe)
	{
		peo = pe;
		pe = BADDR(pe->pe_next);
		UnLock(peo->pe_lock);
		FreeVec(peo);
	}
}


/*
 * XXX: this is not reentrant.. put a semaphore or so
 */
LONG StartWorkbench(ULONG flags, BPTR ptr)
{
	UBYTE path[256]; /* - /Ambient - <spc>WBSTARTUP */
	BPTR cd, pd;
	struct MsgPort *mp = NULL;
	struct Process *pr = (struct Process *)FindTask(NULL);
	struct Window *oldwin;

	D(STARTWB,bug("called\n"));

	/*
	 * Find out if Ambient is already there.
	 */
	Forbid();
	if ( (mp = FindPort("Ambient IPC")) )
	{
		D(STARTWB,bug("Ambient is already present\n"));

		/*
		 * WBF_AMBIENT guarantees 'ptr' is passed in correct register
		 * and it's nodes are allocated with AllocVec().
		 */
		if ((flags & (WBF_NEWPATH | WBF_AMBIENT)) == (WBF_NEWPATH | WBF_AMBIENT))
		{
			/*
			 * Update the path.
			 */
			struct ipcmessage gmsg;
			struct ipc_newpath msg;
			struct MsgPort rport;

			CreateQPort(&rport);

			gmsg.pool             = NULL;
			gmsg.msg.mn_ReplyPort = &rport;
			gmsg.msg.mn_Length    = sizeof(gmsg);
			gmsg.type             = IPC_NEWPATH;
			gmsg.msgtype          = &msg;

			msg.pathlock          = ptr;

			PutMsg(mp, &gmsg.msg);

			Permit();

			WaitPort(&rport);
			(void)GetMsg(&rport);

			DeleteQPort(&rport);

			/*
			 * IPC_NEWPATH doesn't free the path list, just clone it,
			 * so free here.
			 */
			free_path(ptr);

			return (TRUE);
		}
	}
	Permit();

	if (mp)
	{
		D(STARTWB,bug("not launching\n"));
		return (FALSE);
	}

	path[0] = '\0';
	GetVar("Ambient_path", path, sizeof(path) - 18, 0);

	oldwin = pr->pr_WindowPtr;
	pr->pr_WindowPtr = (APTR)-1;

	if (!checkpath(path, &pd))
	{
		D(STARTWB,bug("not in $Ambient_path\n"));
		strcpy(path, "MOSSYS:Ambient");

		if (!checkpath(path, &pd))
		{
			D(STARTWB,bug("not in MOSSYS:Ambient\n"));
			strcpy(path, "SYS:System/Ambient");

			if (!checkpath(path, &pd))
			{
				pr->pr_WindowPtr = oldwin;
				D(STARTWB,bug("not in SYS:System/Ambient\n"));
				errorreq("No Ambient installation detected. I looked at:\n"
				         "- $Ambient_path\n"
				         "- MOSSYS:Ambient\n"
				         "- SYS:System/Ambient",
				         NULL);

				SystemTags("newshell",
					SYS_Asynch, TRUE,
					SYS_Input, NULL,
					SYS_Output, NULL,
					NP_Priority, 0,
				TAG_DONE);

				return (FALSE);
			}
		}
	}

	/*
	 * The following combination took me a while to
	 * find out.
	 */
	D(STARTWB,bug("ok.. trying to run Ambient, path: <%s>\n", path));

	if ( (cd = Lock("RAM:", ACCESS_READ)) )
	{
		AddPart(path, "Ambient", sizeof(path));
		if (!(flags & WBF_NOWBSTARTUP))
		{
			strcat(path, " WBSTARTUP");
		}

		pr->pr_WindowPtr = oldwin;
		if (!SystemTags(path,
			SYS_Input, NULL,
			SYS_Output, NULL,
			SYS_Asynch, TRUE,
			SYS_FilterTags, FALSE,       /* we don't want NP_HomeDir to be filtered out */
			NP_CurrentDir, cd,
			NP_HomeDir, pd,
			NP_Name, (ULONG)"Workbench", /* this is needed for FindTask("Workbench") to succeed. Some programs get the WB path that way */
			NP_CopyVars, FALSE,
			TAG_DONE)
		)
		{
			D(STARTWB,bug("run successfully\n"));

			/*
			 * We don't actually currently use the path list in this case
			 * (SystemTags clones the path for us), so if the caller is new
			 * Ambient LoadWB, free the path list.
			 */
			if (flags & WBF_AMBIENT)
			{
				free_path(ptr);
			}

			return (TRUE);
		}
		else
		{
			UnLock(pd);
			D(STARTWB,bug("failed to run\n"));
			errorreq("Couldn't start %s", NULL, path);
		}
	}
	else
	{
		pr->pr_WindowPtr = oldwin;
		D(STARTWB,bug("no lock on RAM:\n"));
		errorreq("Failed to lock RAM:", "Huh");
	}

	SystemTags("newshell",
		SYS_Asynch, TRUE,
		SYS_Input, NULL,
		SYS_Output, NULL,
		NP_Priority, 0,
	TAG_DONE);

	return (FALSE);
}
