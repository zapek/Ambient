/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2007 Ambient Open Source Team
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
 * $Id: soundlib.c,v 1.8 2017/08/11 23:32:05 cyfm Exp $
 */

#include "ambient.h"

#if USE_SOUNDLIB

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
#include "classes.h"
#include "soundlib.h"
#include "sound.h"
#include "copyright.h"
#include "smartreq.h"


struct Library *ASoundLibBase;

static BPTR libexpunge(void);
static struct Library * LIB_Open(void);
static BPTR LIB_Close(void);
static BPTR LIB_Expunge(void);
static ULONG LIB_GetQueryAttr(void);

static APTR LIB_ASound_Create(ULONG mode, struct TagItem *tags);
static void LIB_ASound_Delete(APTR ctx);
static ULONG LIB_ASound_PlaySync(APTR ctx, APTR buf, ULONG len);
static ULONG LIB_ASound_PlayAsync(APTR ctx, APTR buf, ULONG len);
static ULONG LIB_ASound_Wait(APTR ctx);
static void LIB_ASound_Abort(APTR ctx);
static APTR LIB_ASound_GetAttr(APTR ctx, ULONG attr);


static const ULONG libfunctable[] =
{
	FUNCARRAY_BEGIN,
	FUNCARRAY_32BIT_NATIVE,
	(ULONG)&LIB_Open,
	(ULONG)&LIB_Close,
	(ULONG)&LIB_Expunge,
	(ULONG)&LIB_GetQueryAttr,
	0xffffffff,
	FUNCARRAY_32BIT_SYSTEMV,
	(ULONG)&LIB_ASound_Create,
	(ULONG)&LIB_ASound_Delete,
	(ULONG)&LIB_ASound_PlaySync,
	(ULONG)&LIB_ASound_PlayAsync,
	(ULONG)&LIB_ASound_Wait,
	(ULONG)&LIB_ASound_Abort,
	(ULONG)&LIB_ASound_GetAttr,
	0xffffffff,
	FUNCARRAY_END
};


static const TEXT libname[] = "ambient_sound.library";
static const TEXT idstring[] = "ambient_sound.library 1.0 (28.2.2004) © 2004-2005 by David Gerber, © 2005-2006 Ambient Open Source Team";

#if !USE_LEGACY
static const struct TagItem querytags[] = {
	{QUERYINFOATTR_NAME, (ULONG)libname},
	{QUERYINFOATTR_DESCRIPTION, (ULONG)"High level library to play sound efficiently"},
	{QUERYINFOATTR_COPYRIGHT, (ULONG)"© 2004-2005 by David Gerber, © 2005-2006 Ambient Open Source Team"},
	{QUERYINFOATTR_AUTHOR, (ULONG)"David Gerber. Ambient Open Source Team"},
	{QUERYINFOATTR_SUBTYPE, QUERYSUBTYPE_LIBRARY},
	{QUERYINFOATTR_CLASS, QUERYCLASS_NONE},
	{QUERYINFOATTR_SUBCLASS, QUERYSUBCLASS_NONE},
	{TAG_DONE, 0}
};
#endif


ULONG soundlib_init(void)
{
	if ( (ASoundLibBase = NewCreateLibraryTags(
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
		LIBTAG_VERSION, 1,
		LIBTAG_REVISION, 0,
		LIBTAG_IDSTRING, idstring,
		LIBTAG_PUBLIC, TRUE,
	TAG_DONE)) )
	{
		return (TRUE);
	}
	return (FALSE);
}


void soundlib_cleanup(void)
{
	if (ASoundLibBase)
	{
		if (!libexpunge())
		{
			PDB(("should not happen (tm)\n"));
		}
	}
}


static BPTR libexpunge(void)
{
	Forbid();
	if (ASoundLibBase->lib_OpenCnt)
	{
		ASoundLibBase->lib_Flags |= LIBF_DELEXP;
		Permit();
		return (0);
	}

	REMOVE(&ASoundLibBase->lib_Node);
	Permit();

	FreeMem(((UBYTE *) ASoundLibBase) - ASoundLibBase->lib_NegSize,
	        ASoundLibBase->lib_NegSize + ASoundLibBase->lib_PosSize);

	return (TRUE);
}


static struct Library * LIB_Open(void)
{
	ASoundLibBase->lib_Flags &= ~LIBF_DELEXP;
	ASoundLibBase->lib_OpenCnt++;

	return ((struct Library *)ASoundLibBase);
}


static BPTR LIB_Expunge(void)
{
	return (0); /* do not let anything expunge us, except ourself */
}


static BPTR LIB_Close(void)
{
	if ((--ASoundLibBase->lib_OpenCnt) == 0)
	{
		if (ASoundLibBase->lib_Flags & LIBF_DELEXP)
		{
			return (0); /* same here */
		}
	}
	return (0);
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


static APTR LIB_ASound_Create(ULONG mode, struct TagItem *tags)
{
	return (v_sound_create(mode, tags));
}


static void LIB_ASound_Delete(APTR ctx)
{
	sound_delete(ctx);
}


static ULONG LIB_ASound_PlaySync(APTR ctx, APTR buf, ULONG len)
{
	return (sound_play_sync(ctx, buf, len));
}


static ULONG LIB_ASound_PlayAsync(APTR ctx, APTR buf, ULONG len)
{
	return (sound_play_async(ctx, buf, len));
}


static ULONG LIB_ASound_Wait(APTR ctx)
{
	return (sound_wait(ctx));
}


static void LIB_ASound_Abort(APTR ctx)
{
	sound_abort(ctx);
}


static APTR LIB_ASound_GetAttr(APTR ctx, ULONG attr)
{
	return (sound_getattr(ctx, attr));
}


ULONG preclose_soundlib(void)
{
	if (ASoundLibBase && ASoundLibBase->lib_OpenCnt)
	{
		smartreq_request(NULL, NULL, NULL, 0, 0, "Ok", MV_Notification_Warning, "ambient_sound.library's opencount is %ld.\nClose the processes using it.", ASoundLibBase->lib_OpenCnt);
		return (FALSE);
	}
	return (TRUE);
}

#endif /* USE_SOUNDLIB */
