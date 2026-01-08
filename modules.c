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
 * $Id: modules.c,v 1.6 2006/08/08 13:31:35 fab Exp $
 */

#include "ambient.h"

/* public */
#include <exec/resident.h>
#include <dos/dos.h>
#include <proto/dos.h>

/* private */
#include "modules.h"


struct MinList moduleslist;


struct LibInitStruct
{
	ULONG	LibSize;
	void	*FuncTable;
	void	*DataTable;
	void	(*InitFunc)(void);
};


ULONG modules_init(void)
{
	NEWLIST(&moduleslist);

	return (TRUE);
}


void modules_cleanup(void)
{
	struct Library *module, *nextmodule;

	ITERATELISTSAFE(module, nextmodule, &moduleslist)
	{
		module_close(module);
	}

	#ifdef DEBUG
	ITERATELIST(module, &moduleslist)
	{
		PDB(("failed to remove <%s>\n", module->lib_Node.ln_Name));
	}
	#endif
}


static struct Resident *scanresident(BPTR seglist)
{
	struct Resident *resident;
	ULONG *segment;
	UWORD *buffer, *endbuf;

	/* scan for RomTag */
	while (seglist)
	{
		segment = (ULONG *)BADDR(seglist);
		buffer = (UWORD *)&segment[1];
		endbuf = (UWORD *)&segment[(segment[-1] - sizeof(struct Resident)) / sizeof(ULONG)];

		while (buffer < endbuf)
		{
			if (*buffer == RTC_MATCHWORD)
			{
				resident = (struct Resident *)buffer;

				if (resident->rt_MatchTag == resident)
				{
					return (resident);
				}
			}
			buffer++;
		}
		seglist = (BPTR)segment[0];
	}
	return (NULL);
}


struct Library *module_open(CONST_STRPTR name, ULONG version)
{
	struct Library *module = NULL;
	BPTR seglist;

	if ((seglist = LoadSeg(name)))
	{
		struct Resident *resident;

		if ((resident = scanresident(seglist)))
		{
			if (resident->rt_Version >= version && !strcmp(resident->rt_Name, FilePart(name)))
			{
				struct Library *library = NULL;

				if (resident->rt_Flags & RTF_PPC)
				{
					struct LibInitStruct *init = resident->rt_Init;

					if (resident->rt_Flags & RTF_AUTOINIT)
					{
						library = NewCreateLibraryTags(LIBTAG_FUNCTIONINIT, (ULONG)init->FuncTable,
						                               LIBTAG_STRUCTINIT,   (ULONG)init->DataTable,
						                               LIBTAG_LIBRARYINIT,  (ULONG)init->InitFunc,
						                               LIBTAG_BASESIZE,     (ULONG)init->LibSize,
						                               LIBTAG_MACHINE,      MACHINE_PPC,
						                               LIBTAG_SEGLIST,      (ULONG)seglist,
						                               LIBTAG_TYPE,         (ULONG)resident->rt_Type,
						                               LIBTAG_NAME,         (ULONG)resident->rt_Name,
						                               LIBTAG_FLAGS,        LIBF_SUMUSED | LIBF_CHANGED,
						                               LIBTAG_IDSTRING,     (ULONG)resident->rt_IdString,
						                               LIBTAG_VERSION,      (ULONG)resident->rt_Version,
						                               (resident->rt_Flags & RTF_EXTENDED) ? LIBTAG_REVISION : TAG_IGNORE, (ULONG)resident->rt_Revision,
						                               LIBTAG_PUBLIC,       FALSE,
						                               TAG_DONE);
					}
					else
					{
						struct Library *(*initmodule)(struct Library *, BPTR, struct ExecBase *) = (APTR)resident->rt_Init;

						if (initmodule)
						{
							library = (*initmodule)(NULL, seglist, SysBase);
						}
					}
				}
				else
				{
					PDB(("no 68k support\n"));
				}

				if (library)
				{
					ENQUEUE(&moduleslist, library); /* expunge will remove the lib from that list */

					if (!(module = LP0(6, struct Library *, Open, , library, 0, 0, 0, 0, 0, 0)))
					{
						seglist = LP0(18, BPTR, Expunge,  , library, 0, 0, 0, 0, 0, 0);
					}
				}
			}
		}

		if (!module)
		{
			UnLoadSeg(seglist);
		}
	}
	return (module);
}


void module_close(struct Library *module)
{
	if (module)
	{
		BPTR seglist;

		if (!(seglist = LP0(12, BPTR , Close, , module, 0, 0, 0, 0, 0, 0)))
		{
			seglist = LP0(18, BPTR , Expunge, , module, 0, 0, 0, 0, 0, 0);
		}

		if (seglist)
		{
			UnLoadSeg(seglist);
		}
	}
}
