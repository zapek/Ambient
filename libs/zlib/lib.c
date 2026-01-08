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
 * $Id: lib.c,v 1.4 2017/07/29 16:26:49 piru Exp $
 */

#include "globals.h"

#if !USE_LEGACY
#include <libraries/query.h>
#endif
#include <proto/utility.h>

#include "lib.h"
#include "copyright.h"
#include "z.h"

/*
 * Ah well, I'm tired of cut & past so define the stuff
 * and copy the file somewhere else.
 */
#define LIBNAME      "z.alib"
#define LIBCOPYRIGHT "© 2002-2004 David Gerber, © 2005-2006 Ambient Open Source Team"
#define LIBPRI 0
#define LIBBASE ZLibBase

static const char verstr[]
#if __GNUC__ > 2
__attribute__((used))
#endif
= "$VER: " LIBNAME " " LVERTAG " " LIBCOPYRIGHT;

extern ULONG LibFuncTable[];

struct Library*	LIB_Init(struct LibBase	*LIBBASE, BPTR SegList, struct ExecBase *SBase);

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
struct Library*	LIB_Init(struct LibBase *LIBBASE, BPTR SegList, struct ExecBase *SBase)
{
	LIBBASE->SegList = SegList;
	LIBBASE->SBase = SBase;

	if (lib_init(SBase))
	{
		LIBBASE->Lib.lib_Node.ln_Pri = LIBPRI;

		return (&LIBBASE->Lib);
	}

	lib_cleanup();

	FreeMem((APTR)((ULONG)(LIBBASE) - (ULONG)(LIBBASE->Lib.lib_NegSize)),
		LIBBASE->Lib.lib_NegSize + LIBBASE->Lib.lib_PosSize);
	
	return (0);
}


/*
 * The following is needed because it's also called by LIB_Close() with
 * PPC args
 */
static ULONG libexpunge(struct LibBase *LIBBASE)
{
	BPTR MySegment;

	MySegment =	LIBBASE->SegList;

	if (LIBBASE->Lib.lib_OpenCnt)
	{
		LIBBASE->Lib.lib_Flags |= LIBF_DELEXP;
		return (NULL);
	}

	Forbid();
	Remove((struct Node *)&LIBBASE->Lib);
	Permit();

	lib_cleanup();

	FreeMem((APTR)((ULONG)(LIBBASE) - (ULONG)(LIBBASE->Lib.lib_NegSize)),
		LIBBASE->Lib.lib_NegSize + LIBBASE->Lib.lib_PosSize);

	return ((ULONG)MySegment);
}


ULONG LIB_Expunge(void)
{
	struct LibBase *LIBBASE = (struct LibBase *)REG_A6;

	return (libexpunge(LIBBASE));
}


static ULONG opened;

struct Library * LIB_Open(void)
{
	struct LibBase	*LIBBASE = (struct LibBase *)REG_A6;

	if (!opened)
	{
		opened = 1;
		
		if (!lib_open())
		{
			lib_cleanup();
			return (0);
		}
	}

	LIBBASE->Lib.lib_Flags &= ~LIBF_DELEXP;
	LIBBASE->Lib.lib_OpenCnt++;
	return (&LIBBASE->Lib);
}


ULONG LIB_Close(void)
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
		switch (attr)
		{
			case QUERYINFOATTR_NAME:
				*data = (ULONG)LIBNAME;
				return (TRUE);

			case QUERYINFOATTR_DESCRIPTION:
				*data = (ULONG)"Ambient private library. Don't touch";
				return (TRUE);

			case QUERYINFOATTR_COPYRIGHT:
				*data = (ULONG)LIBCOPYRIGHT;
				return (TRUE);

			case QUERYINFOATTR_AUTHOR:
				*data = (ULONG)"David Gerber";
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

