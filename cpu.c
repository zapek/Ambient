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
 * $Id: cpu.c,v 1.13 2017/08/21 06:17:44 cyfm Exp $
 */

#include "ambient.h"

/* public */
#include <exec/system.h>
#include <proto/exec.h>

/* private */
#include "cpu.h"
#include "mui_func.h"
#include "vars.h"
#include "ambient_altivec.h"
#include "cpu_vec.h"


struct cpu_capabilities {
	ULONG l1_dcachelinesize;
};

static struct cpu_capabilities cpucap;

struct cpu_descr {
	ULONG pvr;
	ULONG mask;
	ULONG id;
	CONST_STRPTR name;
	ULONG flags;
};


static const struct cpu_descr cpuid[CPUID_NONE + 1] = {
	/*
	 * When adding an entry and there's already another
	 * with the same PVR version, make sure the one with
	 * the largest mask is added *before*.
	 */
	{0x00010000, 0xffff0000, CPUID_601,       "601",     0},
	{0x00030000, 0xffff0000, CPUID_603,       "603",     0},
	{0x00060000, 0xffff0000, CPUID_603e,      "603e",    0},
	{0x00071000, 0xfffff000, CPUID_603r,      "603r",    0},
	{0x00070000, 0xffff0000, CPUID_603ev,     "603ev",   0},
	{0x00040000, 0xffff0000, CPUID_604,       "604",     0},
	{0x00090000, 0xfffff000, CPUID_604e,      "604e",    0},
	{0x00090000, 0xffff0000, CPUID_604r,      "604r",    0},
	{0x000a0000, 0xffff0000, CPUID_604ev,     "604ev",   0},
	{0x00084202, 0xffffffff, CPUID_740_750_a, "740/750", 0},
	{0x00083000, 0xfffff000, CPUID_745_755,   "745/755", 0},
	{0x00080100, 0xfffffff0, CPUID_750CX_a,   "750CX",   0},
	{0x00082200, 0xfffffff0, CPUID_750CX_b,   "750CX",   0},
	{0x00082210, 0xfffffff0, CPUID_750CXe,    "750CXe",  CPUF_L2_256},
	{0x70000000, 0xffff0000, CPUID_750FX,     "750FX",   0},
	{0x70020102, 0xffffff0f, CPUID_750GL,     "750GL",   0},
	{0x70020000, 0xffff0000, CPUID_750GX,     "750GX",   0},
	{0x00080000, 0xffff0000, CPUID_740_750_b, "740/750", 0},
	{0x000c0000, 0xffff0000, CPUID_7400,      "7400",    0},
	{0x800c0000, 0xffff0000, CPUID_7410,      "7410",    0},
	{0x80000203, 0xffffffff, CPUID_7451,      "7451",    0},
	{0x80000000, 0xffff0000, CPUID_7450,      "7450",    0},
	{0x80010000, 0xffff0000, CPUID_7455,      "7455",    0},
	{0x80020100, 0xfffffff0, CPUID_7447,      "7447",    CPUF_L3_NONE | CPUF_L2_512},
	{0x80020000, 0xffff0000, CPUID_7457,      "7457",    0},
	{0x00810000, 0x7fff0000, CPUID_82xx,      "82xx",    0},
	{0x00390000, 0xffff0000, CPUID_970,       "970",     0},
	{0x80030000, 0xffff0000, CPUID_7447A,     "7447A",   0},
	{0x80040000, 0xffff0000, CPUID_7448,      "7448",    0},
	{0x003C0000, 0xffff0000, CPUID_970FX,     "970FX",   0},
	{0x00440000, 0xffff0000, CPUID_970MP,     "970MP",   0},
	{0x80810000, 0xffff0000, CPUID_5200,      "5200",    0},
	{0x80820000, 0xffff0000, CPUID_5200LE,    "5200LE",  0},
	{0, 0, 0, NULL, 0}
};


ULONG cpu_init(void)
{
	if (_var(nocachepretouch) || !NewGetSystemAttrs(&cpucap.l1_dcachelinesize, sizeof(cpucap.l1_dcachelinesize), SYSTEMINFOTYPE_PPC_DCACHEL1LINESIZE, TAG_DONE))
	{
		cpucap.l1_dcachelinesize = 0;
	}
	return (TRUE);
}


void cpu_cleanup(void)
{
	/* zZz */
}


ULONG cpu_id(ULONG version, ULONG revision)
{
	ULONG i;

	for (i = 0; cpuid[i].name; i++)
	{
		if ((((version << 16) | revision) & cpuid[i].mask) == cpuid[i].pvr)
		{
			return (cpuid[i].id);
		}
	}
	return (CPUID_NONE);
}


CONST_STRPTR cpu_name(ULONG id)
{
	if (id == CPUID_NONE)
	{
		return ("unknown");
	}
	else
	{
		ASSERT(id < CPUID_NONE);

		return (cpuid[id].name);
	}
}


ULONG cpu_flags(ULONG id)
{
	ASSERT(id <= CPUID_NONE);

	return (cpuid[id].flags);
}


void cpu_cachebuf(APTR obj, ULONG flags)
{
	if (flags & CACHEFLAGF_AVAILABLE)
	{
		set(obj, MA_AddText_Contents, "available");
	}
	if (flags & CACHEFLAGF_ACTIVE) /* that one doesn't work for L2.. laire should fix it */
	{
		set(obj, MA_AddText_Contents, "enabled");
	}
	if (flags & CACHEFLAGF_INST && !(flags & CACHEFLAGF_UNIFIED))
	{
		set(obj, MA_AddText_Contents, "instruction");
	}
	if (flags & CACHEFLAGF_DATA && !(flags & CACHEFLAGF_UNIFIED))
	{
		set(obj, MA_AddText_Contents, "data");
	}
	if (flags & CACHEFLAGF_INSTSNOOP)
	{
		set(obj, MA_AddText_Contents, "isnoop");
	}
	if (flags & CACHEFLAGF_DATASNOOP)
	{
		set(obj, MA_AddText_Contents, "dsnoop");
	}
	if (flags & CACHEFLAGF_UNIFIEDSNOOP)
	{
		set(obj, MA_AddText_Contents, "usnoop");
	}
	if (flags & CACHEFLAGF_INSTLOCKED)
	{
		set(obj, MA_AddText_Contents, "ilock");
	}
	if (flags & CACHEFLAGF_DATALOCKED)
	{
		set(obj, MA_AddText_Contents, "dlock");
	}
	if (flags & CACHEFLAGF_COPYBACK)
	{
		set(obj, MA_AddText_Contents, "copyback");
	}
	if (flags & CACHEFLAGF_UNIFIED)
	{
		set(obj, MA_AddText_Contents, "unified");
	}
}


#if USE_CPU_CACHEHINTS
void cpu_cache_zero(APTR ptr, ULONG size)
{
	/*
	 * Only perform to cache aligned data, at least with
	 * the size of the cache. Caller is responsible for
	 * determining if the memory is non-cacheable.
	 * TypeOfMem(ptr) can be used to determine this.
	 */
	LONG dcachelinesize = cpucap.l1_dcachelinesize;

	if (dcachelinesize && size >= dcachelinesize)
	{
		UBYTE *p = ptr;
		LONG s;

		p = (APTR)((IPTR)(p + dcachelinesize - 1) & -dcachelinesize);
		s = (UBYTE *)ptr + size - p;

		if (s >= dcachelinesize)
		{
			do
			{
				__asm volatile ("dcbz 0, %0\n" : : "r" (p));
				p += dcachelinesize;
			} while ((s -= dcachelinesize) >= dcachelinesize);
		}
	}
}


/*
 * blockcount: number of blocks (from 0 to 255, 0 = 256 blocks)
 * blocksize: number of 16-byte chunks (from 0 to 31, 0 = 32 * 16-byte chunks)
 * blockstride: 16-bit value from -32768 to 32767 (0 = 32768), denotes the address
 *   increment from the beginning of one block to the beginning of the next.
 */
void cpu_cache_stream_start(CONST_APTR ptr UNUSED, ULONG blockcount UNUSED, ULONG blocksize UNUSED, LONG stride UNUSED, ULONG flags UNUSED)
{
	#if USE_ALTIVEC
	if (use_altivec)
	{
		ULONG opts;
		ULONG channel = 0;

		opts = (blocksize << 24) | (blockcount << 16) | stride;

		if (flags & CCSSF_CHANNEL1)
		{
			channel = 1;
		}
		else if (flags & CCSSF_CHANNEL2)
		{
			channel = 2;
		}
		else if (flags & CCSSF_CHANNEL3)
		{
			channel = 3;
		}

		if (flags & CCSSF_WRITES)
		{
			if (flags & CCSSF_TRANSCIENT)
			{
				cpu_vec_dststt(ptr, opts, channel);
			}
			else
			{
				cpu_vec_dstst(ptr, opts, channel);
			}
		}
		else
		{
			if (flags & CCSSF_TRANSCIENT)
			{
				cpu_vec_dstt(ptr, opts, channel);
			}
			else
			{
				cpu_vec_dst(ptr, opts, channel);
			}
		}
	}
	#endif
	/* XXX: fallback to dtst etc.. ? */
}


void cpu_cache_stream_stop(ULONG channel UNUSED)
{
	#if USE_ALTIVEC
	if (use_altivec)
	{
		cpu_vec_dss(channel);
	}
	#endif
}

#endif


APTR cpu_cache_malloc(ULONG size)
{
	if (size)
	{
		ULONG dcachelinesize = cpucap.l1_dcachelinesize;

		if (dcachelinesize)
		{
			APTR *p, *q;

			size = ((size + dcachelinesize - 1) & -dcachelinesize) + dcachelinesize - 1 + sizeof(APTR);
		
			if ((q = malloc(size)))
			{
				p = q + 1;
				p = (APTR *)(((IPTR)p + dcachelinesize - 1) & -dcachelinesize);
				p[-1] = q;

				return (p);
			}
		}
		else
		{
			return (malloc(size));
		}
	}
	return (NULL);
}


void cpu_cache_free(APTR ptr)
{
	if (ptr)
	{
		APTR *p = ptr;

		free(p[-1]);
	}
}
