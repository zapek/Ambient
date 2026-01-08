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
 * $Id: lib.c,v 1.8 2022/01/06 16:53:16 piru Exp $
 */

#include "globals.h"

#if !USE_LEGACY
#include <libraries/query.h>
#endif

#include "lib.h"
#include "copyright.h"
#include "wb.h"

/*
 * Ah well, I'm tired of cut & past so define the stuff
 * and copy the file somewhere else.
 */
#define LIBNAME      "workbench.library"
#define LIBCOPYRIGHT "© 2001-2004 David Gerber, © 2006-2016 Ambient Open Source Team"
#define LIBPRI 0
#define LIBBASE WorkbenchBase

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


static const struct Resident libresident
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
	(APTR)1,
};

/*
 * To tell the loader that this is a new emulppc elf and not
 * one for the ppc.library.
 */
const ULONG __abox__ = 1;


/*
 * Library functions (system)
 */
struct Library *LIB_Init(struct LibBase *LIBBASE, BPTR SegList, struct ExecBase *SBase)
{
	LIBBASE->SegList = SegList;
	LIBBASE->SBase = SBase;

	if (lib_init(SBase, LIBBASE))
	{
		struct ExecBase *SysBase = SBase;
		InitSemaphore(&LIBBASE->appwinsem);
		InitSemaphore(&LIBBASE->basesem);

		LIBBASE->Lib.lib_Node.ln_Pri = LIBPRI;
		return (&LIBBASE->Lib);
	}

	lib_cleanup(LIBBASE);

	FreeMem(((UBYTE *)LIBBASE) - LIBBASE->Lib.lib_NegSize,
		LIBBASE->Lib.lib_NegSize + LIBBASE->Lib.lib_PosSize);

	return (NULL);
}


/*
 * The following is needed because it's also called by LIB_Close() with
 * PPC args
 */

extern ULONG childrenbusy;

static BPTR libexpunge(struct LibBase *LIBBASE)
{
	BPTR MySegment;

	Forbid();
	if (LIBBASE->Lib.lib_OpenCnt || childrenbusy)
	{
		LIBBASE->Lib.lib_Flags |= LIBF_DELEXP;
		Permit();
		return (NULL);
	}

	REMOVE((struct Node *)&LIBBASE->Lib);
	Permit();

	lib_cleanup(LIBBASE);

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



struct Library *LIB_Open(void)
{
	struct LibBase	*base = (struct LibBase *)REG_A6;

	base->Lib.lib_Flags &= ~LIBF_DELEXP;
	base->Lib.lib_OpenCnt++;

	if (base->Initialized == 0)
	{
		ObtainSemaphore(&base->basesem);

		if (base->Initialized == 0)
		{
			base->Initialized = lib_open(base);
		}

		ReleaseSemaphore(&base->basesem);
	}

	return (&base->Lib);
}


BPTR LIB_Close(void)
{
	struct LibBase	*base = (struct LibBase *)REG_A6;

	if ((--base->Lib.lib_OpenCnt) == 0)
	{
		ObtainSemaphore(&base->basesem);

		if (base->Lib.lib_OpenCnt == 0 && base->Initialized)
		{
			base->Initialized = 0;
			lib_close(base);
		}

		ReleaseSemaphore(&base->basesem);

		if (base->Lib.lib_Flags & LIBF_DELEXP)
		{
			return (libexpunge(base));
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
		switch (attr)
		{
			case QUERYINFOATTR_NAME:
				*data = (ULONG)LIBNAME;
				return (TRUE);

			case QUERYINFOATTR_DESCRIPTION:
				*data = (ULONG)"Library to handle some desktop functions";
				return (TRUE);

			case QUERYINFOATTR_COPYRIGHT:
				*data = (ULONG)LIBCOPYRIGHT;
				return (TRUE);

			case QUERYINFOATTR_AUTHOR:
				*data = (ULONG)"David Gerber, Ambient Open Source Team";
				return (TRUE);

			case QUERYINFOATTR_SUBTYPE:
				*data = QUERYSUBTYPE_LIBRARY;
				return (TRUE);

			case QUERYINFOATTR_CLASS:
				*data = QUERYCLASS_NONE;
				return (TRUE);

			case QUERYINFOATTR_SUBCLASS:
				*data = QUERYSUBCLASS_NONE;
				return (TRUE);
		}
	}
	#endif
	return (FALSE);
}

