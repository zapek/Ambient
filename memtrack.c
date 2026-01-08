/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
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
 * $Id: memtrack.c,v 1.8 2013/10/28 11:15:22 geit Exp $
 */

#include "ambient.h"

#if USE_MEMTRACK

/* public */
#include <clib/alib_protos.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <exec/semaphores.h>

/* private */
#include "memtrack.h"
#include "segtracker.h"

/*
 * Some more options:
 */

/*
 * Checks if the pointer is valid on freeing, otherwise
 * complain and don't free.
 */
#define USE_MEMTRACK_MEMLIST_POINTERS 1

/*
 * Write a pattern after allocation.
 */
#define USE_MEMTRACK_WRITEPATTERN_ALLOC 1
#define MEMTRACK_PATTERN_ALLOC 0xbe /* value */

/*
 * Write a pattern before freeing.
 */
#define USE_MEMTRACK_WRITEPATTERN_FREE 1
#define MEMTRACK_PATTERN_FREE 0xde /* value */

/*
 * Do full consistency check every nth allocation per subsystem.
 */
#define USE_MEMTRACK_AUTOCHECK 1
#define MEMTRACK_AUTOCHECK_VAL 256

/*
 * Checks if allocation functions are properly freed with their
 * freeing functions and not the ones from another subsystem.
 */
#define USE_MEMTRACK_MEMLIST_CROSSCHECK 1

/*
 * Report memory allocation failures.
 */
#define USE_MEMTRACK_ALLOC_REPORT 1


#define MEMTRACK_WALL_BLOCKSIZE 16   /* bytes of one wall side, 32-bit aligned */
#define MEMTRACK_WALL_FRONTVAL 0xaa  /* value of the frontwall */
#define MEMTRACK_WALL_BACKVAL 0x55   /* value of the backwall */

#define PPCSTACKSCANSIZE 512

#define MTDB(x) { kprintf("Ambient memtrack: "); kprintf x ; }

static struct SignalSemaphore mallocsem[NUMMFUNC];

/*
 * malloc memory handling subsystem.
 */

static APTR mallocpool[NUMMFUNC];
#if USE_MEMTRACK_MEMLIST
static APTR mallocmwpool[NUMMFUNC];
static struct MinList mwlist[NUMMFUNC];

struct mwnode {
	struct MinNode n;
	APTR location;
	ULONG size;
	ULONG line;
	TEXT s[0];
};
#if USE_MEMTRACK_AUTOCHECK
static ULONG alloccnt[NUMMFUNC];
#endif
#endif

#if USE_MEMTRACK_RECORD
static ULONG recording;
static APTR recordpool;
static struct MinList recordlist;
static struct SignalSemaphore recordsem;

struct recnode {
	struct MinNode n;
	APTR location;
	ULONG size;
	ULONG subsys;
	ULONG line;
	TEXT s[0];
};
#endif


ULONG memtrack_init(void)
{
	ULONG i;

	for (i = 0; i < NUMMFUNC; i++)
		InitSemaphore(&mallocsem[i]);

	return (TRUE);
}


/*
 * Called to cleanup the memory subsystem.
 */
void memtrack_cleanup(void)
{
	ULONG i;

	#if USE_MEMTRACK_RECORD
	if (recordpool)
	{
		DeletePool(recordpool);
	}
	#endif

	for (i = 0; i < NUMMFUNC; i++)
	{
		#if USE_MEMTRACK_MEMLIST
		if (mallocmwpool[i])
		{
			DeletePool(mallocmwpool[i]);
		}
		#endif
		if (mallocpool[i])
		{
			DeletePool(mallocpool[i]);
		}
	}
}


/*
 * Allocates memory for malloc. Initializes the subsystem if needed.
 */
APTR memtrack_malloc(STRPTR s, STRPTR f, ULONG line, ULONG subsys, size_t size)
{
	ULONG *memblock;

	#if USE_MEMTRACK_MEMLIST
	if (!mallocmwpool[subsys])
	{
		ObtainSemaphore(&mallocsem[subsys]);

		if (!mallocmwpool[subsys])
		{
			NEWLIST(&mwlist[subsys]);

			mallocmwpool[subsys] = CreatePool(MEMF_SEM_PROTECTED | MEMF_ANY | MEMF_REVERSE, 65536, 1024);
		}

		ReleaseSemaphore(&mallocsem[subsys]);

		if (!mallocmwpool[subsys])
		{
			MTDB(("memlists couldn't be initialized, bailing out\n"));
			return (0);
		}
	}
	#endif

	if (!mallocpool[subsys])
	{
		ObtainSemaphore(&mallocsem[subsys]);

		if (!mallocpool[subsys])
		{
			mallocpool[subsys] = CreatePool(MEMF_ANY, 65536, 8192);
		}

		ReleaseSemaphore(&mallocsem[subsys]);

		if (!mallocpool[subsys])
		{
			MTDB(("couldn't initialize memory subsystem %lu, bailing out\n", subsys));
			return (0);
		}
	}

	size = (size + 3) & ~3; /* round to LONG */

	#if USE_MEMTRACK_AUTOCHECK
	if (alloccnt[subsys] == MEMTRACK_AUTOCHECK_VAL)
	{
		alloccnt[subsys] = 0;
		memcheck();
	}
	#endif

	ObtainSemaphore(&mallocsem[subsys]);

	#if USE_MEMTRACK_RECORD
	if (recording)
	{
		if (!recordpool)
		{
			ObtainSemaphore(&mallocsem[subsys]);

			if (!recordpool)
			{
				if ((recordpool = CreatePool(MEMF_ANY | MEMF_REVERSE, 65536, 1024)))
				{
					InitSemaphore(&recordsem);
					NEWLIST(&recordlist);
				}
				else
				{
					MTDB(("couldn't create recordpool, defering. might be incomplete\n"));
				}
			}

			ReleaseSemaphore(&mallocsem[subsys]);
		}
	}
	#endif

	if ((memblock = AllocPooled(mallocpool[subsys], size + MEMTRACK_WALL_BLOCKSIZE * 2 + sizeof(ULONG *))))
	{
		memset(memblock, MEMTRACK_WALL_FRONTVAL, MEMTRACK_WALL_BLOCKSIZE);
		memblock += MEMTRACK_WALL_BLOCKSIZE / 4;
		*memblock = size + MEMTRACK_WALL_BLOCKSIZE * 2 + sizeof(ULONG *);
		memblock++;
		memset((UBYTE *)memblock + size, MEMTRACK_WALL_BACKVAL, MEMTRACK_WALL_BLOCKSIZE);
		#if USE_MEMTRACK_MEMLIST
		{
			struct mwnode *mwn;

			if ((mwn = AllocVecPooled(mallocmwpool[subsys], sizeof(*mwn) + strlen(s) + strlen(f) + 2)))
			{
				mwn->location = memblock;
				mwn->size = size;
				mwn->line = line;
				sprintf(mwn->s, "%s %s", s, f);
				ADDTAIL(&mwlist[subsys], mwn);
			}
			else
			{
				MTDB(("not enough memory to add memlist entry, expect bogus memstate results\n"));
			}
		}
		#endif
	
		#if USE_MEMTRACK_WRITEPATTERN_ALLOC
		memset(memblock, MEMTRACK_PATTERN_ALLOC, size);
		#endif

		#if USE_MEMTRACK_RECORD
		if (recording && recordpool)
		{
			struct recnode *rn;

			ObtainSemaphore(&recordsem);

			if ((rn = AllocVecPooled(recordpool, sizeof(*rn) + strlen(s) + strlen(f) + 2))) /* XXX: optimize.. see above */
			{
				rn->location = memblock;
				rn->size = size;
				rn->subsys = subsys;
				rn->line = line;
				sprintf(rn->s, "%s %s", s, f);
				ADDTAIL(&recordlist, rn);
			}
			else
			{
				MTDB(("not enough memory to add record, expect bogus results\n"));
			}

			ReleaseSemaphore(&recordsem);
		}
		#endif
	}
	ReleaseSemaphore(&mallocsem[subsys]);
	
	#if USE_MEMTRACK_ALLOC_REPORT
	if (!memblock)
	{
		MTDB(("memory allocation failed for size: %lu\n", (ULONG)size));
	}
	#endif
	return (memblock);
}


__inline__ static ULONG check_front(UBYTE *p)
{
	ULONG i;
	ULONG freeit = TRUE;

	for (i = 1; i <= MEMTRACK_WALL_BLOCKSIZE; i++)
	{
		if (*(p - i) != MEMTRACK_WALL_FRONTVAL)
		{
			MTDB(("frontwall byte -%ld trashed (addr: %p, val 0x%lx)\n", i, p - 1, (ULONG)*(p - i)));
			showppcstackhistory((ULONG *) __builtin_frame_address(0),
								(ULONG *) ((UBYTE *)__builtin_frame_address(0) + PPCSTACKSCANSIZE)); /* XXX: print s as well */

			freeit = FALSE;
			break;
		}
	}
	return (freeit);
}


__inline__ static ULONG check_back(UBYTE *p)
{
	ULONG i;
	ULONG freeit = TRUE;

	for (i = 0; i < MEMTRACK_WALL_BLOCKSIZE; i++)
	{
		if (*(p + i) != MEMTRACK_WALL_BACKVAL)
		{
			MTDB(("backwall byte %ld trashed (addr: %p, val 0x%lx)\n", i, p + i, (ULONG)*(p + i)));
			showppcstackhistory((ULONG *) __builtin_frame_address(0),
								(ULONG *) ((UBYTE *)__builtin_frame_address(0) + PPCSTACKSCANSIZE)); /* XXX: print s as well */
			freeit = FALSE;
			break;
		}
	}
	return (freeit);
}


#if USE_MEMTRACK_MEMLIST
#define MEMLIST_LOCKMEM ({ ULONG i; for (i = 0; i < NUMMFUNC; i++) ObtainSemaphore(&mallocsem[i]); })
#define MEMLIST_UNLOCKMEM ({ ULONG i; for (i = 0; i < NUMMFUNC; i++) ReleaseSemaphore(&mallocsem[i]); })

void memcheck(void)
{
	ULONG i;
	struct mwnode *mwn;
	ULONG *memblock;

	MEMLIST_LOCKMEM;

	for (i = 0; i < NUMMFUNC; i++)
	{
		ITERATELIST(mwn, &mwlist[i])
		{
			UBYTE *p;
			memblock = mwn->location;
			memblock--;
			p = (UBYTE *)memblock;

			if ((mwn->size + MEMTRACK_WALL_BLOCKSIZE * 2 + sizeof(ULONG *)) != *memblock)
			{
				MTDB(("checkmem: size trashed for allocation %p (type: %ld), original is %ld, current is %ld\n", mwn->location, i, mwn->size, *memblock)); /* XXX: print s as well */
				continue;
			}

			check_front(p);
			p += *memblock - MEMTRACK_WALL_BLOCKSIZE * 2;
			check_back(p);
		}
	}

	MEMLIST_UNLOCKMEM;
}

void memstats(ULONG mode)
{
	ULONG i;
	struct mwnode *mwn;
	ULONG size, cnt;

	memcheck();

	MTDB(("memory statistics:\n"));

	MEMLIST_LOCKMEM;

	for (i = 0; i < NUMMFUNC; i++)
	{
		size = 0;
		cnt = 0;
		MTDB(("    type: %lu\n", i));

		ITERATELIST(mwn, &mwlist[i])
		{
			if (mode == MEMSTATS_ALL)
			{
				MTDB(("        entry: %p, size: %lu, %s[%lu]\n", mwn->location, mwn->size, mwn->s, mwn->line));
			}
			size += mwn->size;
			cnt++;
		}
		MTDB(("        total size: %lu in %lu allocations\n", size, cnt));
	}
	
	MTDB(("    recording: %s\nend of memory statistics\n", recording ? "yes" : "no"));

	MEMLIST_UNLOCKMEM;
}
#endif


#if USE_MEMTRACK_RECORD
void memtrack_record_start(void)
{
	MTDB(("starting recording..\n"));
	NEWLIST(&recordlist);
	recording = TRUE;
}


void memtrack_record_stop(void)
{
	struct recnode *rn;

	if (recording)
	{
		MTDB(("stopping recording\n"));
		recording = FALSE;

		if (recordpool)
		{
			ObtainSemaphore(&recordsem);

			MTDB(("remaining allocations so far:\n"));

			ITERATELIST(rn, &recordlist)
			{
				MTDB(("    entry: %p, type: %lu, size: %lu, %s[%lu]\n", rn->location, rn->subsys, rn->size, rn->s, rn->line));
			}
			FlushPool(recordpool);
		
			ReleaseSemaphore(&recordsem);
		}
	}
	else
	{
		MTDB(("there's nothing being recorded\n"));
	}
}
#endif

#if USE_MEMTRACK_MEMLIST_CROSSCHECK
__inline__ static ULONG memtrack_crosscheck(ULONG line UNUSED, ULONG subsys, ULONG *ptr)
{
	struct mwnode *mwn;
	ULONG i;

	for (i = 0; i < MFUNC_MAXSYS; i++)
	{
		if (i == subsys)
		{
			continue;
		}

		ITERATELIST(mwn, &mwlist[i])
		{
			if (mwn->location == ptr)
			{
				MTDB(("ptr %p  was allocated with subsys %lu but freed with subsys %lu, size: %lu, %s[%lu]\n", ptr, subsys, i, mwn->size, mwn->s, mwn->line));
				return (TRUE);
			}
		}
	}
	return (FALSE);
}
#endif

/*
 * Frees memory for icons. The subsystem has to be
 * initialized.
 */
void memtrack_free(STRPTR s, STRPTR f, ULONG line, ULONG subsys, APTR ptr)
{
	ULONG *memblock = (ULONG *)ptr;

	ObtainSemaphore(&mallocsem[subsys]);

	if (memblock--)
	{
		UBYTE *p = (UBYTE *)memblock;
		ULONG freeit1 = TRUE;
		ULONG freeit2 = TRUE;
		ULONG size = *memblock;
		#if USE_MEMTRACK_MEMLIST_POINTERS
		ULONG found = FALSE;
		#endif

		#if USE_MEMTRACK_MEMLIST
		{
			struct mwnode *mwn;

			ITERATELIST(mwn, &mwlist[subsys])
			{
				if (mwn->location == (ULONG *)ptr)
				{
					REMOVE(mwn);
					FreeVecPooled(mallocmwpool[subsys], mwn);
					#if USE_MEMTRACK_MEMLIST_POINTERS
					found = TRUE;
					#endif
					break;
				}
			}
			#if USE_MEMTRACK_MEMLIST_POINTERS
			if (!found)
			{
				#if USE_MEMTRACK_MEMLIST_CROSSCHECK
				if (!memtrack_crosscheck(line, subsys, ptr))
				#endif
				{
					MTDB(("trying to free unallocated pointer at %p, subsys: %lu, %s %s[%lu]\n", ptr, subsys, s, f, line));
				}
			}
			#endif
		}
		#endif
		
		#if USE_MEMTRACK_RECORD
		if (recording && recordpool)
		{
			struct recnode *rn;

			ObtainSemaphore(&recordsem);

			ITERATELIST(rn, &recordlist)
			{
				if (rn->location == (ULONG *)ptr)
				{
					REMOVE(rn);
					FreeVecPooled(recordpool, rn);
					break;
				}
			}

			ReleaseSemaphore(&recordsem);
		}
		#endif

		freeit1 = check_front(p);

		p += *memblock - MEMTRACK_WALL_BLOCKSIZE * 2;

		freeit2 = check_back(p);

		#if USE_MEMTRACK_MEMLIST_POINTERS
		if (found && freeit1 && freeit2) /* a memleak is better than a crash at that point */
		#else
		if (freeit1 && freeit2) /* a memleak is better than a crash at that point */
		#endif
		{
			memblock -= MEMTRACK_WALL_BLOCKSIZE / 4;
			#if USE_MEMTRACK_WRITEPATTERN_FREE
			memset(memblock, MEMTRACK_PATTERN_FREE, size);
			#endif
			FreePooled(mallocpool[subsys], memblock, size);
		}
		else if (!freeit1 || !freeit2)
		{
			MTDB(("Memory deallocation ommited: %p, subsys: %lu, %s %s[%lu]\n", ptr, subsys, s, f, line));
		}


	}
	ReleaseSemaphore(&mallocsem[subsys]);
}

#endif /* USE_MEMTRACK */
