/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * © 2001-2004 by David Gerber <zapek@morphos.net>
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
 * $Id: freelist.c,v 1.5 2006/04/12 14:01:58 fab Exp $
 */

#include "globals.h"

/* public */
#include <clib/alib_protos.h>
#include <exec/memory.h>
#include <workbench/workbench.h>
#include <proto/exec.h>

/* private */
#include "icon_internal.h"
#include "clib/icon_protos.h"
#include "freelist.h"
#include <macros/vapor.h>


#define DB_FREELIST 0


/*
 * Support function. Builds up a FreeList.
 * A FreeList contains a list containing
 * MemList nodes. Each MemList contains
 * FREELIST_MEMLISTENTRIES MemEntries.
 * If NULL is supplied as parameter, it allocates
 * a FreeList structure and puts it as first
 * member into the first MemList, first MemEntry.
 */
struct FreeList * create_freelist(struct FreeList *freelist)
{
	struct MemList *ml;
	struct FreeList *fl;
	ULONG allocated = FALSE;

	D(FREELIST,bug("freelist 0x%lx\n", (ULONG)freelist));
	
	if (freelist)
	{
		fl = freelist;
		D(FREELIST,bug("called to update freelist 0x%lx\n", (ULONG)freelist));
	}
	else
	{
		if (!(fl = AllocMem(sizeof(struct FreeList), MEMF_PUBLIC)))
		{
			return (NULL);
		}
		allocated = TRUE;
		D(FREELIST,bug("called to create freelist 0x%lx\n", (ULONG)fl));
	}

	NEWLIST(&fl->fl_MemList);

	if ((ml = AllocMem(sizeof(struct MemList) + (FREELIST_MEMLISTENTRIES - 1) * sizeof(struct MemEntry), MEMF_PUBLIC | MEMF_CLEAR)))
	{
		fl->fl_NumFree = FREELIST_MEMLISTENTRIES;
		ml->ml_NumEntries = FREELIST_MEMLISTENTRIES;
		ADDTAIL(&fl->fl_MemList, ml);
		return (fl);
	}
	D(FREELIST,bug("ouch.. failed..\n"));
	if (allocated)
	{
		FreeMem(fl, sizeof(struct FreeList));
	}
	return (NULL);
}


/*****i icon.library/FreeAlloc ***********************************************
*
*   NAME
*	FreeAlloc - allocate memory and add it to a free list.
*
*   SYNOPSIS
*	status = FreeAlloc( free, len, type )
*	  D0                  A0   A1   A2
*
*	APTR FreeAlloc(struct FreeList *, ULONG, ULONG);
*
*   FUNCTION
*	This routine allocates the amount of memory specified and
*	then adds it to the free list.  The free list will be extended
*	(if required).  If there is not enough memory to complete the call,
*	a null is returned.
*
*	Note that FreeAlloc not only allocates the requested memory
*	but also records the memory in the free list.
*
*   INPUTS
*	free -- a pointer to a FreeList structure
*	len -- the length of the memory to be recorded
*	type -- the type of memory to be allocated
*
*   RESULTS
*	memory -- a pointer to the newly allocated memory chunk
*	          or zero if the call failed.
*
*   SEE ALSO
*	AllocEntry, FreeEntry, FreeFreeList
*
******************************************************************************
*/
APTR FreeAlloc(struct FreeList *freelist, ULONG size, ULONG flags)
{
	APTR mem;
	
	D(FREELIST,bug("called: freelist 0x%lx size %ld flags 0x%lx\n", (ULONG)freelist, size, flags));

	if ((mem = AllocMem(size, flags)))
	{
		D(FREELIST,bug("mem 0x%lx\n", (ULONG)mem));
		if (AddFreeList(freelist, mem, size))
		{
			return (mem);
		}
		else
		{
			D(FREELIST,bug("called: failed\n"));
			FreeMem(mem, size);
		}
	}
	return (NULL);
}


/*icon.library/AddFreeList                             icon.library/AddFreeList

   NAME
	   AddFreeList - add memory to a free list.

   SYNOPSIS
	   status = AddFreeList(free, mem, len)
		 D0                  A0    A1   A2

	BOOL AddFreeList(struct FreeList *, APTR, ULONG);

   FUNCTION
	This routine adds the specified memory to the free list.
	The free list will be extended (if required).  If there
	is not enough memory to complete the call, a null is returned.

	Note that AddFreeList does NOT allocate the requested memory.
	It only records the memory in the free list.

   INPUTS
	free -- a pointer to a FreeList structure
	mem -- the base of the memory to be recorded
	len -- the length of the memory to be recorded

   RESULTS
	status -- TRUE if the call succeeded else FALSE;

   SEE ALSO
	AllocEntry(), FreeEntry(), FreeFreeList()

   BUGS
	None
*/
BOOL AddFreeList(struct FreeList *freelist, APTR mem, unsigned long size)
{
	struct MemList *ml;
	
	D(FREELIST,bug("called for allocating size %ld, freelist 0x%lx NumFree %ld\n", size, (ULONG)freelist, (ULONG)freelist->fl_NumFree));

	ml = LASTNODE(&freelist->fl_MemList);

	if (!freelist->fl_NumFree) /* number of entries free in the *LAST* node */
	{
		D(FREELIST,bug("allocating a new one\n"));
		/* allocate a new entry */
		if (!(ml = AllocMem(sizeof(struct MemList) + (FREELIST_MEMLISTENTRIES - 1) * sizeof(struct MemEntry), MEMF_PUBLIC | MEMF_CLEAR)))
		{
			D(FREELIST,bug("failed\n"));
			return (FALSE);
		}
		ml->ml_NumEntries = FREELIST_MEMLISTENTRIES;
		freelist->fl_NumFree = FREELIST_MEMLISTENTRIES;
		ADDTAIL(&freelist->fl_MemList, ml);
	}
	D(FREELIST,bug("inserting MEMLISTENTRIES %ld fl_NumFree %ld\n", (ULONG)FREELIST_MEMLISTENTRIES, (ULONG)freelist->fl_NumFree));
	/* insert the arguments */
	ml->ml_ME[FREELIST_MEMLISTENTRIES - freelist->fl_NumFree].me_Un.meu_Addr = mem;
	ml->ml_ME[FREELIST_MEMLISTENTRIES - freelist->fl_NumFree].me_Length = size;
	freelist->fl_NumFree--;
	D(FREELIST,bug("done inserting..\n"));

	return (TRUE);
}


void FreeFreeList(struct FreeList *freelist)
{
	struct MemList *ml1, *ml2, *ml3;

	D(FREELIST,bug("freeing freelist 0x%lx\n", (ULONG)freelist));

	ml1 = LASTNODE(&freelist->fl_MemList);
	ml2 = PREVNODE(ml1);
	while (ml2)
	{
		D(FREELIST,bug("freeing..\n"));
		ml3 = PREVNODE(ml2);
		FreeEntry(ml1);
		ml1 = ml2;
		ml2 = ml3;
	}
}

