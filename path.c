/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
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
 * $Id: path.c,v 1.7 2017/07/29 00:04:53 cyfm Exp $
 */

#include "ambient.h"

/* public */
#include <proto/dos.h>
#include <proto/exec.h>

/* private */
#include "path.h"
#include "ipc.h"


struct pathentry {
	BPTR pe_next;
	BPTR pe_lock;
};


static void free_path(BPTR path)
{
	struct pathentry *pe = BADDR(path), *peo;

	D(PATH,bug("freeing path..\n"));

	while (pe)
	{
		peo = pe;
		pe = BADDR(pe->pe_next);
		D(PATH,bug("freeing lock 0x%lx\n", (ULONG)peo->pe_lock));
		UnLock(peo->pe_lock);
		FreeVec(peo);
	}
}


static BPTR clone_path(BPTR path, BOOL verify)
{
	struct pathentry *pe, *pen = NULL, *peo = NULL;
	BPTR res = (BPTR)NULL;
	struct FileInfoBlock fib;

	for (pe = BADDR(path); pe; pe = BADDR(pe->pe_next))
	{
		/*
		 * This check is paranoia really, path's don't include files.
		 *
		 * NEW: Don't Examine() if cli->cli_CommandDir (verify == FALSE)  -itix
		 */
		if (!verify || (Examine(pe->pe_lock, &fib) && (fib.fib_DirEntryType > 0))) /* only copy dirs */
		{
			if ( (pen = AllocVec(sizeof(*pen), MEMF_ANY)) )
			{
				pen->pe_next = (BPTR)NULL;

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
					break; /* XXX */
				}
				peo = pen;
			}
		}
	}

	if (!pen || !pen->pe_lock)
	{
		free_path(res);
		res = (BPTR)NULL;
	}

	return (res);
}


STRPTR path_build(CONST_STRPTR filename)
{
	STRPTR p;

	THREAD;
	ASSERT(filename);

	D(PATH,bug("building path for <%s>\n", filename));

	p = FilePart(filename);

	if (p == filename)
	{
		struct CommandLineInterface *cli;
		struct pathentry *pe;
		BPTR origdir;
		LONG exres;
		STRPTR t;
		BPTR l;
		struct FileInfoBlock fib;

		if ( (cli = Cli()) )
		{
			D(PATH,bug("CLI path found.. iterating..\n"));

			if ( (t = malloc(PATH_SIZE)) )
			{
				if (cli->cli_CommandDir)
				{
					origdir = CurrentDir(0);
					for (pe = BADDR(cli->cli_CommandDir); pe; pe = BADDR(pe->pe_next))
					{
						CurrentDir(pe->pe_lock);
						l = Lock(filename, ACCESS_READ);
						if (l)
						{
							exres = Examine(l, &fib);
							UnLock(l);

							if (exres && fib.fib_DirEntryType < 0) /* accept only files */
							{
								CurrentDir(origdir);

								if (NameFromLock(pe->pe_lock, t, PATH_SIZE) &&
								    AddPart(t, filename, PATH_SIZE))
								{
									D(PATH,bug("found <%s>\n", t));
									return (t);
								}

								/*
								 * Well, really. We found the file, but can't get the name,
								 * so fail. :-\
								 */
								free(t);
								return (NULL);
							}
						}
					}
					CurrentDir(origdir);
				}

				/*
				 * C: is done by the shell. Not in the real path.
				 */
				strcpy(t, "C:");
				stccpy(t + 2, filename, PATH_SIZE - 2);

				D(PATH,bug("testing C: internal path..\n"));

				if ( (l = Lock(t, ACCESS_READ)) )
				{
					D(PATH,bug("found\n"));
					UnLock(l);
					return (t);
				}
				free(t);
			}
			else
			{
				errormsg(ERR_NOMEM);
			}
		}
		else
		{
			/*
			 * Special support for guys like Henes who
			 * run Ambient using an icon..
			 */
			struct Process *wb;

			D(PATH,bug("trying to find Workbench..\n"));

			if ( (wb = (struct Process *)FindTask("Workbench")) )
			{
				struct CommandLineInterface *wbcli;

				D(PATH,bug("found wbtask at %p\n", wb));

				if ( (wbcli = BADDR(wb->pr_CLI)) )
				{
					D(PATH,bug("iterating..\n"));

					if ( (t = malloc(PATH_SIZE)) )
					{
						/*
						 * NOTE: No semaphore can help here, since we're scanning
						 * foreign process' CLI anyway. Things can go wrong here
						 * at any time and there is nothing we can do about it.
						 * Tough luck. - Piru
						 */

						if (wbcli->cli_CommandDir)
						{
							origdir = CurrentDir(0);
							for (pe = BADDR(wbcli->cli_CommandDir); pe; pe = BADDR(pe->pe_next))
							{
								CurrentDir(pe->pe_lock);
								l = Lock(filename, ACCESS_READ);
								if (l)
								{
									exres = Examine(l, &fib);
									UnLock(l);

									if (exres && fib.fib_DirEntryType < 0) /* accept only files */
									{
										CurrentDir(origdir);

										if (NameFromLock(pe->pe_lock, t, PATH_SIZE) &&
										    AddPart(t, filename, PATH_SIZE))
										{
											D(PATH,bug("found <%s>\n", t));
											return (t);
										}

										/*
										 * Well, really. We found the file, but can't get the name,
										 * so fail. :-\
										 */
										free(t);
										return (NULL);
									}
								}
							}
							CurrentDir(origdir);
						}

						/*
						 * C: is done by the shell. Not in the real path.
						 */
						strcpy(t, "C:");
						stccpy(t + 2, filename, PATH_SIZE - 2);
				
						D(PATH,bug("testing C: internal path..\n"));

						if ( (l = Lock(t, ACCESS_READ)) )
						{
							D(PATH,bug("found\n"));
							UnLock(l);
							return (t);
						}

						free(t);
					}
					else
					{
						errormsg(ERR_NOMEM);
					}
				}
			}
		}
	}
	else
	{
		/*
		 * There's a relative/absolute path, so, return that..
		 */
		return (STRPTR) (filename);
	}
	return (NULL);
}


void path_free(STRPTR filename)
{
	if (filename)
	{
		free(filename);
	}
}

void path_update(struct ipc_newpath *np)
{
	struct CommandLineInterface *cli;

	/*
	 * This thing really should only be called from the main Ambient
	 * process.
	 */

	THREAD;
	ASSERT(np);

	D(PATH,bug("got update path..\n"));

	if ( (cli = Cli()) )
	{
		BPTR newpath;

		if ( (newpath = clone_path(np->pathlock, TRUE)) || !np->pathlock )
		{
			BPTR oldpath;

			oldpath = cli->cli_CommandDir;
			cli->cli_CommandDir = newpath;

			free_path(oldpath);

			D(PATH,bug("path updated\n"));
		}
	}
}


void path_clone(struct ipc_clonepath *cp)
{
	struct CommandLineInterface *cli;

	THREAD;
	ASSERT(cp);

	D(PATH,bug("got clone path..\n"));

	if ((cli = Cli()))
	{
		*cp->path = clone_path(cli->cli_CommandDir, FALSE);
	}
}
