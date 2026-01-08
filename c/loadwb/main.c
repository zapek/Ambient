/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * © 2001-2004 by David Gerber 
 * © 2005-2006 Ambient Open Source Team
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
 * $Id: main.c,v 1.7 2017/07/29 16:26:48 piru Exp $
 */

#include <dos/rdargs.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>


#define TEMPLATE "DEBUG=-DEBUG/S,DELAY/S,CLEANUP/S,NEWPATH/S,NOWBSTARTUP/S"

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

struct DosLibrary *DOSBase;
struct Library *WorkbenchBase;

struct pathentry {
	BPTR pe_next;
	BPTR pe_lock;
};

struct options {
	LONG debug;
	LONG delay;
	LONG cleanup;
	LONG newpath;
	LONG nowbstartup;
};

static const char verstring[]
#if __GNUC__ > 2
__attribute__((used))
#endif
= "$VER: LoadWB 50.4 (12.4.2006) © 2002-2004 David Gerber, © 2005-2006 Ambient Open Source Team";


static void free_path(BPTR path)
{
	struct pathentry *pe = BADDR(path), *peo;

	while (pe)
	{
		peo = pe;
		pe = BADDR(pe->pe_next);
		UnLock(peo->pe_lock);
		FreeVec(peo);
	}
}


static BPTR clone_path(BPTR path)
{
	struct pathentry *pe, *pen = NULL, *peo = NULL;
	BPTR res = NULL;

	for (pe = BADDR(path); pe; pe = BADDR(pe->pe_next))
	{
		if ((pen = AllocVec(sizeof(*pen), MEMF_ANY)))
		{
			pen->pe_next = NULL;

			if (peo)
			{
				peo->pe_next = MKBADDR(pen);
			}
			else
			{
				res = MKBADDR(pen);
			}

			if (!(pen->pe_lock = DupLock(pe->pe_lock)))
			{
				break;
			}
			peo = pen;
		}
	}

	if (!pen || !pen->pe_lock)
	{
		free_path(res);
		res = NULL;
	}

	return (res);
}


int main(void)
{
	LONG rc = RETURN_ERROR;

	if ((DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 50)))
	{
		if ((WorkbenchBase = OpenLibrary("workbench.library", 40)))
		{
			struct RDArgs *rda;
			struct options opt;

			memset(&opt, 0, sizeof(opt));

			if ((rda = ReadArgs(TEMPLATE, (LONG *)&opt, NULL)))
			{
				struct CommandLineInterface *cli;
				BPTR pathlock = NULL;
				BPTR oldlock;
				struct MsgPort *oct;
				ULONG flags = 0;

				if (opt.debug)
					flags |= WBF_DEBUG;

				if (opt.delay)
					flags |= WBF_DELAY;

				if (opt.cleanup)
					flags |= WBF_CLEANUP;

				if (opt.newpath)
					flags |= WBF_NEWPATH;

				if (opt.nowbstartup)
					flags |= WBF_NOWBSTARTUP;

				/*
				 * Copy current path.
				 */
				if ((cli = Cli()))
				{
					if (cli->cli_CommandDir) /* do we have a path ? */
					{
						pathlock = clone_path(cli->cli_CommandDir);
					}
				}

				oct = SetConsoleTask(NULL);
				/*
				 * The following is normal. It's up to the WB to set
				 * a proper CurrentDir (uses RAM:)
				 */
				oldlock = CurrentDir(NULL);

				/*
				 * WBF_AMBIENT guarantees that 'pathlock' is passed in
				 * REG_D1 and that it's nodes are allocated with AllocVec().
				 * This flag is required for detecting the new, bugfixed
				 * LoadWB. - piru
				 *
				 * Workaround a bug in old Ambients and send path list
				 * in both D1 (correct) and A0 (wrong, but expected by
				 * old Ambient). - piru
				 */
				REG_D0 = WBF_AMBIENT | flags;
				REG_D1 = (ULONG) pathlock; /* correct register */
				REG_A0 = (ULONG) pathlock; /* for old, broken ambient */
				REG_A6 = (ULONG) WorkbenchBase;
				if (MyEmulHandle->EmulCallDirectOS(-0x2a))
				{
					rc = RETURN_OK;
				}
				else
				{
					rc = RETURN_WARN;
					if (pathlock)
					{
						free_path(pathlock);
					}
				}

				CurrentDir(oldlock);
				SetConsoleTask(oct);

				FreeArgs(rda);
			}
			else
			{
				PrintFault(IoErr(), NULL);
			}
			CloseLibrary(WorkbenchBase);
		}
		else
		{
			PrintFault(ERROR_INVALID_RESIDENT_LIBRARY, NULL);
			SetIoErr(ERROR_INVALID_RESIDENT_LIBRARY);
		}
		CloseLibrary((struct Library *)DOSBase);
	}
	else
	{
		((struct Process *) FindTask(NULL))->pr_Result2 = ERROR_INVALID_RESIDENT_LIBRARY;
	}
	return (rc);
}
