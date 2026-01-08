/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Cypyright 2001-2004 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2025 Ambient Open Source Team
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
 * $Id: lib.c,v 1.9.10.1 2025/01/06 00:28:00 cyfm Exp $
 */

#include "globals.h"

#if !USE_LEGACY
#include <libraries/query.h>
#endif
#include <proto/utility.h>

#include "lib.h"
#include "copyright.h"
#include "icon.h"


/*
 * Ah well, I'm tired of cut & past so define the stuff
 * and copy the file somewhere else.
 */
#define LIBNAME      "icon.library"
#define LIBCOPYRIGHT "© 2002-2004 David Gerber, © 2005-2025 Ambient Open Source Team"
#define LIBPRI 0
#define LIBBASE IconBase

#if USE_ICONLIB_JUMPTABLE
struct Library *IconBaseInt;
#endif

static const char verstr[]
#if __GNUC__ > 2
__attribute__((used))
#endif
= "$VER: " LIBNAME " " LVERTAG " " LIBCOPYRIGHT;

extern ULONG LibFuncTable[];

struct Library *LIB_Init(struct LibBase	*LIBBASE, BPTR SegList, struct ExecBase *SBase);

struct libinitstruct
{
	ULONG	LibSize;
	void	*FuncTable;
	void	*DataTable;
	void	(*InitFunc)(void);
};

static const struct libinitstruct libinit_struct =
{
	sizeof(struct LibBase),
	LibFuncTable,
	NULL,
	(void (*)(void))&LIB_Init
};


#if !USE_LEGACY
static const struct TagItem querytags[] = {
	{QUERYINFOATTR_NAME, (ULONG)LIBNAME},
	{QUERYINFOATTR_DESCRIPTION, (ULONG)"Library to handle icons and their metadata"},
	{QUERYINFOATTR_COPYRIGHT, (ULONG)LIBCOPYRIGHT},
	{QUERYINFOATTR_AUTHOR, (ULONG)"David Gerber, Ambient Open Source Team"},
	{QUERYINFOATTR_SUBTYPE, QUERYSUBTYPE_LIBRARY},
	{QUERYINFOATTR_CLASS, QUERYCLASS_NONE},
	{QUERYINFOATTR_SUBCLASS, QUERYSUBCLASS_NONE},
	{TAG_DONE, 0}
};
#endif

struct Resident libresident
#if __GNUC__ > 2
__attribute__((used))
#endif
= {
	RTC_MATCHWORD,
	(APTR) &libresident,
	(APTR) (&libresident + 1),
	RTF_PPC | RTF_EXTENDED | RTF_AUTOINIT | RTF_AFTERDOS,
	VERSION,
	NT_LIBRARY,
	0,
	LIBNAME,
	LIBNAME " " LVERTAG " "LIBCOPYRIGHT,
	(APTR) &libinit_struct,
	/* new fields */
	REVISION,
	#if !USE_LEGACY
	(APTR) querytags,
	#else
	NULL
	#endif
};

/*
 * To tell the loader that this is a new emulppc elf and not
 * one for the ppc.library.
 */
ULONG __abox__ = 1;


/*
 * Library functions (system)
 */
struct Library *LIB_Init(struct LibBase *LIBBASE, BPTR SegList, struct ExecBase *SBase)
{
	LIBBASE->SegList = SegList;
	LIBBASE->SBase = SBase;

	#if USE_ICONLIB_JUMPTABLE
	IconBaseInt = (struct Library *)LIBBASE;
	#endif

	if (lib_init(SBase))
	{
		LIBBASE->Lib.lib_Node.ln_Pri = LIBPRI;

		return (&LIBBASE->Lib);
	}

	lib_cleanup();

	FreeMem(((UBYTE *)LIBBASE) - LIBBASE->Lib.lib_NegSize,
		LIBBASE->Lib.lib_NegSize + LIBBASE->Lib.lib_PosSize);
	
	return (NULL);
}


/*
 * The following is needed because it's also called by LIB_Close() with
 * PPC args
 */
static BPTR libexpunge(struct LibBase *LIBBASE)
{
	BPTR MySegment;

	Forbid();

	if (LIBBASE->Lib.lib_OpenCnt)
	{
		LIBBASE->Lib.lib_Flags |= LIBF_DELEXP;
		Permit();
		return (NULL);
	}

	REMOVE((struct Node *)&LIBBASE->Lib);
	Permit();

	lib_cleanup();

	MySegment = LIBBASE->SegList;

	FreeMem(((UBYTE *)LIBBASE) - LIBBASE->Lib.lib_NegSize,
		LIBBASE->Lib.lib_NegSize + LIBBASE->Lib.lib_PosSize);

	return (MySegment);
}


BPTR LIB_Expunge(void)
{
	struct LibBase *LIBBASE = (struct LibBase *)REG_A6;

	return (libexpunge(LIBBASE));
}


static ULONG opened;

struct Library *LIB_Open(void)
{
	struct LibBase	*LIBBASE = (struct LibBase *)REG_A6;

	if (!opened)
	{
		opened = 1;
		
		if (!lib_open())
		{
			lib_cleanup();
			return (NULL);
		}
	}

	LIBBASE->Lib.lib_Flags &= ~LIBF_DELEXP;
	LIBBASE->Lib.lib_OpenCnt++;
	return (&LIBBASE->Lib);
}


BPTR LIB_Close(void)
{
	struct LibBase	*LIBBASE = (struct LibBase *)REG_A6;

	if ((--LIBBASE->Lib.lib_OpenCnt) == 0)
	{
		if (LIBBASE->Lib.lib_Flags & LIBF_DELEXP)
		{
			return (libexpunge(LIBBASE));
		}
	}
	return (0);
}


ULONG LIB_GetQueryAttr(void)
{
	#if !USE_LEGACY
	ULONG *data = (ULONG *)REG_A0;
	ULONG attr = REG_D0;

	if (data)
	{
		struct TagItem *ti;

		if ((ti = FindTagItem(attr, querytags)))
		{
			*data = ti->ti_Data;
			return (TRUE);
		}
	}
	#endif
	return (FALSE);
}

