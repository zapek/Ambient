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
 * $Id: recurse.c,v 1.9 2019/02/19 21:29:48 bitrocky Exp $
 */

#include "ambient.h"

/* public */
#include <exec/memory.h>
#include <dos/exall.h>
#include <proto/dos.h>
#include <proto/exec.h>

/* private */
#include "recurse.h"
#include "mui_func.h"
#include "methodstack.h"
#include "exdir.h"
#include "threads.h"

struct dirnode {
	struct MinNode n;
	LONG type;
	ULONG prot;
	UQUAD size;
	STRPTR comment;
	UBYTE name[0];
};

struct dirlist {
	struct MinList l;
	struct dirnode *cdn; /* node to resume scanning from */
	struct dirlist *pdl; /* previous dirlist */
};


static ULONG addfilefunc(APTR obj, CONST_STRPTR path, struct ExAllData *ead, APTR userdata) /* XXX: we abuse 'obj'.. not smart */
{
	ULONG pathlen = strlen(path) + 1 + strlen(ead->ed_Name) + 1;
	struct dirnode *dn;

	if ( (dn = AllocVecPooled(obj, sizeof(*dn) + pathlen)) )
	{
		dn->comment = NULL;
		if (ead->ed_Comment && ead->ed_Comment[0])
		{
			if (!(dn->comment = AllocVecPooled(obj, strlen(ead->ed_Comment)+1)))
			{
				FreeVecPooled(obj, dn);
				return (FALSE);
			}
			else{
				strcpy(dn->comment, ead->ed_Comment);
			}
		}
		strcpy(dn->name, path);
		AddPart(dn->name, ead->ed_Name, pathlen);
		dn->type = ead->ed_Type;
		dn->prot = ead->ed_Prot;
		#if USE_LEGACY
		dn->size = ead->ed_Size;
		#else
		dn->size = ead->ed_Size64;
		#endif
		//dprintf("adding node name %p, %s\n", dn, dn->name);

		ADDTAIL(&((struct dirlist *)userdata)->l, dn);
		return (TRUE);
	}
	return (FALSE);
}

#if USE_LEGACY
#define RECURSETYPE ED_PROTECTION
#else
#define RECURSETYPE ED_SIZE64
#endif

ULONG recurse(APTR obj, CONST_STRPTR path, CONST_STRPTR pattern, ULONG (*enterdir)(APTR obj, CONST_STRPTR path, APTR userdata), ULONG (*exitdir)(APTR obj, CONST_STRPTR path, APTR userdata), ULONG (*func)(APTR obj, CONST_STRPTR path, LONG type, ULONG prot, UQUAD size, APTR userdata, CONST_STRPTR comment), APTR userdata)
{
	APTR pool;
	struct dirlist mdl;  /* main dirlist */
	struct dirlist *cdl; /* current dirlist */
	struct dirlist *ndl; /* new dirlist */
	struct dirnode *cdn; /* current node */
	ULONG rc = FALSE;
	BPTR l;

	THREAD;

	if ( (path && *path) )
	{
		if ( (l = Lock(path, ACCESS_READ)) )
		{
			if ( (pool = CreatePool(MEMF_ANY, 2048, 1024)) )
			{
				rc = TRUE;
				cdl = &mdl;
				cdl->pdl = NULL;
				NEWLIST(cdl);

				D(RECURSE,bug("path <%s> locked\n", path));

				/* current path */
				if (enterdir)
				{
					rc = enterdir(obj, path, userdata);

					if (rc != TRUE) /* taken for FALSE and RECURSE_SKIP */
					{
						D(RECURSE,bug("enterdir %s, exiting..\n", (rc == FALSE) ? "failed" : "skipped"));
						goto out;
					}
				}

				rc = exdir(pool, path, pattern, RECURSETYPE, 0, addfilefunc, NULL, NULL, cdl);
				if (rc != TRUE)
				{
					D(RECURSE,bug("exdir() failed with rc = %d, exiting..\n", rc));
					goto out;
				}

				cdn = FIRSTNODE(cdl);

				while (cdn && (NEXTNODE(cdn)))
				{
					if (cdn->type == ST_USERDIR) /* XXX: what about softlinks and hardlinks ? */
					{
						D(RECURSE,bug("-> (dir: <%s>)\n", cdn->name));
						if (enterdir)
						{
							if ((rc = enterdir(obj, cdn->name, userdata)) != TRUE)
							{
								D(RECURSE,bug("enterdir failed, exiting..\n"));
								goto out;
							}
							else if (rc == RECURSE_SKIP)
							{
								D(RECURSE,bug("skipping directory (RECURSE_SKIP)\n"));
								goto rescan;
							}
						}
						cdl->cdn = cdn; /* keep current node */

						if ( (ndl = AllocPooled(pool, sizeof(*ndl))) )
						{
							NEWLIST(ndl);
							ndl->pdl = cdl; /* record parent */
							cdl = ndl;      /* enter subdir context */
						}
						else
						{
							rc = FALSE;
							break;
						}

						D(RECURSE,bug("scanning <%s>..\n", cdl->pdl->cdn->name));
						rc = exdir(pool, cdl->pdl->cdn->name, pattern, RECURSETYPE, 0, addfilefunc, NULL, NULL, cdl);
						if ( rc != TRUE )
						{
							D(RECURSE,bug("exdir() failed with rc = %d, exiting..\n", rc));
							goto out;
						}

						if (ISLISTEMPTY(cdl))
						{
							/* empty dir */
							D(RECURSE,bug("dir <%s> is empty\n", cdn->name));
							cdl = cdl->pdl;
							cdn = cdl->cdn;
	
							if (exitdir)
							{
								if ((rc = exitdir(obj, cdn->name, userdata)) != TRUE)
								{
									D(RECURSE,bug("exitdir failed, exiting..\n"));
									goto out;
								}
							}

							if (!enterdir && !exitdir)
							{
								if ((rc = func(obj, cdn->name, cdn->type, cdn->prot, cdn->size, userdata, cdn->comment)) != TRUE)
								{
									D(RECURSE,bug("func (exit) failed for <%s>. exiting..\n", cdn->name));
									goto out;
								}
							}
							goto rescan;
						}
						else
						{
							cdn = FIRSTNODE(cdl);
							D(RECURSE,bug("processing dir entries of list %p\n", cdl));
							continue;
						}
			        }

					if ((rc = func(obj, cdn->name, cdn->type, cdn->prot, cdn->size, userdata, cdn->comment)) != TRUE)
					{
						D(RECURSE,bug("func failed for <%s>, exiting..\n", cdn->name));
						goto out;
					}
rescan:
					while ((cdn = NEXTNODE(cdn)) && !NEXTNODE(cdn))
					{
						D(RECURSE,bug("<- (dir)\n"));
						/* end of list.. getting back to the parent */
						if (cdl->pdl)
						{
							struct dirlist *fdl = cdl;
							struct dirnode *fdn, *nextfdn;

							cdl = cdl->pdl;
							cdn = cdl->cdn;

							if (exitdir)
							{
								if ((rc = exitdir(obj, cdn->name, userdata)) != TRUE)
								{
									D(RECURSE,bug("exitdir failed. exiting..\n"));
									goto out;
								}
							}

							if (!enterdir && !exitdir)
							{
								if ((rc = func(obj, cdn->name, cdn->type, cdn->prot, cdn->size, userdata, cdn->comment)) != TRUE)
								{
									D(RECURSE,bug("func (exit) failed for <%s>. exiting..\n", cdn->name));
									goto out;
								}
							}

							/* free every node and the list */
							ITERATELISTSAFE(fdn, nextfdn, fdl)
							{
								if (fdn->comment) FreeVecPooled(pool, fdn->comment);
								FreeVecPooled(pool, fdn);
							}
							FreePooled(pool, fdl, sizeof(*fdl));
							goto rescan; /* check if we are the last again */
						}
						else
						{
							D(RECURSE,bug("we're done\n"));
							break;
						}
					}
				}
out:
				if (exitdir)
				{
					rc = exitdir(obj, path, userdata);
					D(RECURSE,bug("final exitdir returned %ld\n", rc));
				}

				DeletePool(pool);
			}
			UnLock(l);
		}
	}
	return (rc); /* XXX: should be ok but check twice */
}
