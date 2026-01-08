/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2006 Ambient Open Source Team
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
 * $Id: wbstartlib.c,v 1.9 2017/08/11 23:32:05 cyfm Exp $
 */

#include "ambient.h"

#if USE_WBSTARTLIB

/* public */
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#if !USE_LEGACY
#include <libraries/query.h>
#endif
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

/* private */
#include "wbstartlib.h"
#include "wbstart.h"
#include "mui_func.h"
#include "copyright.h"
#include "smartreq.h"


struct Library *WBStartLibBase;

static BPTR libexpunge(void);
static struct Library * LIB_Open(void);
static BPTR LIB_Close(void);
static BPTR LIB_Expunge(void);
static ULONG LIB_GetQueryAttr(void);
static ULONG LIB_ArexxMustDie(void);
static ULONG LIB_WBStartTagList(void);


static const ULONG libfunctable[] =
{
	FUNCARRAY_BEGIN,
	FUNCARRAY_32BIT_NATIVE,
	(ULONG)&LIB_Open,
	(ULONG)&LIB_Close,
	(ULONG)&LIB_Expunge,
	(ULONG)&LIB_GetQueryAttr,
	(ULONG)&LIB_ArexxMustDie,
	(ULONG)&LIB_WBStartTagList,
	0xffffffff,
	FUNCARRAY_END
};

extern struct ExecBase *SysBase;

static ULONG flush_wbstartlib(void)
{
	ULONG retval = FALSE;
	struct Library *lib;
	
	/*
	 * Check if another alien library already exists
	 * and try to flush it.
	 */
	Forbid();
	if ( (lib = (struct Library *)FindName(&SysBase->LibList, "wbstart.library")) )
	{
		if (!lib->lib_OpenCnt)
		{
			RemLibrary(lib);

			if (!FindName(&SysBase->LibList, "wbstart.library"))
			{
				retval = TRUE;
			}
		}
	}
	else
	{
		retval = TRUE;
	}
	Permit();

	return (retval);
}


static const TEXT libname[] = "wbstart.library";
static const TEXT idstring[] = "wbstart.library 50.2 (10.4.2006) © 2002-2005 by David Gerber, © 2005-2006 Ambient Open Source Team";

#if !USE_LEGACY
static const struct TagItem querytags[] = {
	{QUERYINFOATTR_NAME, (ULONG)libname},
	{QUERYINFOATTR_DESCRIPTION, (ULONG)"Library to launch programs from the WB, CLI, ARexx, etc.."},
	{QUERYINFOATTR_COPYRIGHT, (ULONG)"© 2002-2005 by David Gerber, © 2005-2006 by Ambient Open Source Team"},
	{QUERYINFOATTR_AUTHOR, (ULONG)"David Gerber, Ambient Open Source Team"},
	{QUERYINFOATTR_SUBTYPE, QUERYSUBTYPE_LIBRARY},
	{QUERYINFOATTR_CLASS, QUERYCLASS_NONE},
	{QUERYINFOATTR_SUBCLASS, QUERYSUBCLASS_NONE},
	{TAG_DONE, 0}
};
#endif

ULONG wbstartlib_init(void)
{
	/*
	 * wbstart.library 2.2 is bugged and needs
	 * to be flushed twice.
	 */
	flush_wbstartlib();

	if (flush_wbstartlib())
	{
		WBStartLibBase = NewCreateLibraryTags(
			LIBTAG_FUNCTIONINIT, libfunctable,
			LIBTAG_BASESIZE, sizeof(struct Library),
			LIBTAG_TYPE, NT_LIBRARY,
			LIBTAG_NAME, libname,
			LIBTAG_FLAGS, LIBF_CHANGED | LIBF_SUMUSED
			#if USE_LEGACY
			,
			#else 
			| LIBF_QUERYINFO,
			#endif
			LIBTAG_VERSION, 50,
			LIBTAG_REVISION, 2,
			LIBTAG_IDSTRING, idstring,
			LIBTAG_PUBLIC, TRUE,
			TAG_DONE);
	}
	/* XXX: maybe add some debugging to know what happens.. */

	return (TRUE); /* we don't fail if some lib is already here. better than nothing */
}


void wbstartlib_cleanup(void)
{
	if (WBStartLibBase)
	{
		if (!libexpunge())
		{
			PDB(("should not happen (tm)\n")); /* XXX: actually it can but well .. */
		}
	}
}


static BPTR libexpunge(void)
{
	Forbid();
	if (WBStartLibBase->lib_OpenCnt)
	{
		WBStartLibBase->lib_Flags |= LIBF_DELEXP;
		Permit();
		return (0);
	}

	REMOVE(&WBStartLibBase->lib_Node);
	Permit();

	FreeMem(((UBYTE *)WBStartLibBase) - WBStartLibBase->lib_NegSize,
	        WBStartLibBase->lib_NegSize + WBStartLibBase->lib_PosSize);

	return (TRUE);
}


static ULONG LIB_GetQueryAttr(void)
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


static struct Library * LIB_Open(void)
{
	WBStartLibBase->lib_Flags &= ~LIBF_DELEXP;
	WBStartLibBase->lib_OpenCnt++;

	return ((struct Library *)WBStartLibBase);
}


static BPTR LIB_Expunge(void)
{
	return (0); /* do not let anything expunge us, except ourself */
}


static BPTR LIB_Close(void)
{
	if ((--WBStartLibBase->lib_OpenCnt) == 0)
	{
		if (WBStartLibBase->lib_Flags & LIBF_DELEXP)
		{
			return (0); /* same here */
		}
	}
	return (0);
}


static ULONG LIB_ArexxMustDie(void)
{
	/*
	 * That entry is really STUPID.
	 */
	return (0);
}


static ULONG LIB_WBStartTagList(void)
{
	STRPTR name = NULL;
	BPTR newlock, dirlock = (BPTR)NULL;
	ULONG has_dirlock = FALSE;
	ULONG stack = M68K_STACKSIZE;
	ULONG argcnt = 0;
	struct WBArg *wba = NULL;
	ULONG rc = RETURN_FAIL;
	BYTE pri = 0;

	FORTAG((struct TagItem *)REG_A0)
	{
		case WBStart_Name:
			name = (STRPTR)tag->ti_Data;
			break;

		case WBStart_DirectoryName:
			{
				STRPTR dirname = (STRPTR)tag->ti_Data;
				if ( (newlock = Lock(dirname ? dirname : (STRPTR)"", ACCESS_READ)) )
				{
					UnLock(dirlock);
					dirlock = newlock;
					has_dirlock = TRUE;
				}
			}
			break;

		case WBStart_DirectoryLock:
			if ( (newlock = DupLock((BPTR)tag->ti_Data)) )
			{
				UnLock(dirlock);
				dirlock = newlock;
				has_dirlock = TRUE;
			}
			break;

		case WBStart_Stack:
			stack = tag->ti_Data;
			break;

		case WBStart_Priority:
			pri = (BYTE)tag->ti_Data;
			break;

		case WBStart_ArgumentCount:
			DB(("Argument count:%d\n", tag->ti_Data));
			argcnt = tag->ti_Data;
			break;

		case WBStart_ArgumentList:
			wba = (struct WBArg *)tag->ti_Data;
			break;

		#ifdef DEBUG
		default:
			PDB(("unknown tag..\n"));
			break;
		#endif
	}
	NEXTTAG

	if (name && ((!wba || argcnt)))
	{
		/*
		 * Fred Fish's wbrun doesn't supply WBStart_DirectoryName
		 * nor WBStart_DirectoryLock so in that case we use the
		 * currentdir.
		 */

		if (!has_dirlock)
		{
			struct Process *pr = (struct Process *)FindTask(NULL);

			dirlock = pr->pr_CurrentDir;
		}

		if (wbstart(name,
			WBSTARTTAG_FromLib, TRUE,
			WBSTARTTAG_DirLock, dirlock,
			WBSTARTTAG_Priority, pri,
			WBSTARTTAG_Stack, stack,
			WBSTARTTAG_WBArgs, wba,
			WBSTARTTAG_WBArgsCount, argcnt,
		TAG_DONE))
		{
			rc = RETURN_OK;
		}
	}
	return (rc);
}


ULONG preclose_wbstartlib(void)
{
	if (WBStartLibBase && WBStartLibBase->lib_OpenCnt)
	{
		smartreq_request(NULL, NULL, NULL, 0, 0, "Ok", MV_Notification_Warning, "wbstart.library's opencount is %ld.\nClose the processes using it.", WBStartLibBase->lib_OpenCnt);
		return (FALSE);
	}
	return (TRUE);
}

#endif
