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
 * $Id: getdirsize.c,v 1.9 2019/02/19 17:53:29 bitrocky Exp $
 */

#include "ambient.h"

/* public */
#include <dos/dos.h>
#include <dos/dosextens.h>

/* private */
#include "getdirsize.h"
#include "recurse.h"
#include "time_func.h"
#include "classes.h"
#include "methodstack.h"
#include "threads.h"
#include "typescanner.h"
#include "mui_func.h"


static ULONG adddirsize_infowin(APTR obj, CONST_STRPTR path UNUSED, LONG type, ULONG prot UNUSED, UQUAD size, APTR userdata, CONST_STRPTR comment UNUSED)
{
	ULONG curtime;
	struct dirsize *ds = (struct dirsize *)userdata;

	if (type == ST_FILE)
	{
		ds->totalsize += size;
		ds->numfiles++;
	}
	else if (type == ST_USERDIR)
	{
		ds->numdirs++;
	}
	else if (type == ST_LINKDIR || type == ST_LINKFILE)
	{
		ds->numhardlinks++;
	}
	else if (type == ST_SOFTLINK)
	{
		ds->numsoftlinks++;
	}

	curtime = timedm();

	if (curtime - ds->lasttime > 500)
	{
		if (threads_check_abort())
		{
			return (ABORTED);
		}
		methodstack_push_sync(obj, 7, MM_Infowin_UpdateSize, ds->numdirs, ds->numfiles, ds->numhardlinks, ds->numsoftlinks, &ds->totalsize, FALSE);
		ds->lasttime = curtime;
	}
	return (TRUE); /* XXX */
}

static ULONG adddirsize(APTR obj UNUSED, CONST_STRPTR path UNUSED, LONG type, ULONG prot UNUSED, UQUAD size, APTR userdata, CONST_STRPTR comment UNUSED)
{
	struct dirsize *ds = (struct dirsize *)userdata;

	if (type == ST_FILE)
	{
		ds->totalsize += size;
		ds->numfiles++;
	}
	else if (type == ST_USERDIR)
	{
		ds->numdirs++;
	}
	else if (type == ST_LINKDIR || type == ST_LINKFILE)
	{
		ds->numhardlinks++;
	}
	else if (type == ST_SOFTLINK)
	{
		ds->numsoftlinks++;
	}

	return (TRUE); /* XXX */
}


ULONG tr_getdirsize(APTR obj, CONST_STRPTR path)
{
	struct dirsize ds;
	ULONG rc;

	THREAD;
	CHECKOBJECT(obj);
	ASSERT(path);

	ds.lasttime     = timedm();
	ds.numhardlinks = 0;
	ds.numsoftlinks = 0;
	ds.numdirs      = 0;
	ds.numfiles     = 0;
	ds.totalsize    = 0;

	if ((rc = recurse(obj, path, "#?", NULL, NULL, adddirsize_infowin, &ds)) == TRUE)
	{
		methodstack_push_sync(obj, 7, MM_Infowin_UpdateSize, ds.numdirs, ds.numfiles, ds.numhardlinks, ds.numsoftlinks, &ds.totalsize, TRUE);
	}
	/* XXX */
	return (rc);
}

ULONG getdirsize(CONST_STRPTR path, struct dirsize *ds )
{
	ASSERT(path);

	ds->numhardlinks = 0;
	ds->numsoftlinks = 0;
	ds->numdirs      = 0;
	ds->numfiles     = 0;
	ds->totalsize    = 0;

	return recurse(NULL, path, "#?", NULL, NULL, adddirsize, ds);
}

ULONG tr_getdirsizes(APTR obj, struct MinList *l)
{
	ULONG rc = TRUE;
	struct typescannernode *tn, *nexttn;
	ULONG abort = FALSE;

	THREAD;
	CHECKOBJECT(obj);
	ASSERT(l);

	ITERATELISTSAFE(tn, nexttn, l)
	{
		if (threads_check_abort())
		{
			abort = TRUE;
			rc = ABORTED;
		}

		if (!abort)
		{
			struct dirsize ds;

			if (getdirsize( tn->path, &ds) == TRUE)
			{
				methodstack_push(tn->obj, 3, MUIM_Set, MA_Icon_FileSize, &(ds.totalsize));
				methodstack_push_sync(tn->obj, 1, MM_ListviewEntry_Redraw);
			}
			else
			{
				abort = TRUE;
				rc = ABORTED;
			}
		}
		free(tn);
	}

	free(l);

	return (rc);
}
