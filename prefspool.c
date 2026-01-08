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
 * $Id: prefspool.c,v 1.9 2017/08/11 23:32:06 cyfm Exp $
 */

#include "ambient.h"

/* public */
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>
#if USE_SHARED_LIBZ
#include <proto/z.h>
#endif

/* private */
#include "copyright.h"
#include "prefspool.h"
#include "file_io.h"
#include "prefs.h"
#include "atomic.h"
#include "file_func.h"
#include "vars.h"
#include "smartreq.h"
#if !USE_SHARED_LIBZ
#include "libs/zlib/ppcinline/z.h"

extern struct Library *ZLibBase;
#endif

#define IOBUFFERSIZE 2048

#define PREFSPOOL_PUDDLESIZE 2048
#define PREFSPOOL_THRESHSIZE 1024

#define MAXOLDDATALENGTH 2048 /* maximum data size of a prefsnode (probably a string). only used for old prefs */
#define MAXDATALENGTH (1024 * 1024 * 32) /* someone storing more than 32MB in there is probably insane */
#define MAXRECURSIONLEVEL 128

static ULONG guid;

struct prefspool_ctx {
	struct MinList l;
	APTR pool;
	ULONG uid; /* unique id */
};


struct prefsnode {
	struct MinNode n;
	ULONG id;
	ULONG size;
	UBYTE data[0];
};


/* XXX: add per-pool semaphores to prevent concurrent access from different tasks ? */


/*
 * Creates a prefspool. If 'uid' is not specified it
 * picks up an internal one which is unique. This function
 * returns a prefs context.
 */
APTR prefspool_create(ULONG uid)
{
	struct prefspool_ctx *ct;

	if( (ct = malloc(sizeof(*ct))) )
	{
		if ( (ct->pool = CreatePool(MEMF_ANY, PREFSPOOL_PUDDLESIZE, PREFSPOOL_THRESHSIZE)) )
		{
			NEWLIST(&ct->l);
			ct->uid = uid ? uid : atomic_add(&guid, 1);

			D(PREFSPOOL,bug("created prefspool %p, id: %ld\n", ct, ct->uid));

			return (ct);
		}
		free(ct);
	}
	return (NULL);
}


/*
 * Deletes a prefspool.
 */
void prefspool_delete(APTR ctx)
{
	struct prefspool_ctx *ct = ctx;

	ASSERT(ct);
	ASSERT(ct->pool);

	D(PREFSPOOL,bug("deleting prefspool %p, id: %ld\n", ct, ct->uid));

	DeletePool(ct->pool);
	free(ct);
}


/*
 * Flushes all entries of a prefspool.
 */
void prefspool_flush(APTR ctx)
{
	struct prefspool_ctx *ct = ctx;

	ASSERT(ct);
	ASSERT(ct->pool);

	D(PREFSPOOL,bug("flushing prefspool %p, id: %ld\n", ct, ct->uid));

	FlushPool(ct->pool);

	NEWLIST(&ct->l);
}


/*
 * Returns the 'uid' of the prefspool.
 */
ULONG prefspool_uid(APTR ctx)
{
	struct prefspool_ctx *ct = ctx;

	ASSERT(ct);

	return (ct->uid);
}


/*
 * Finds an item.
 */
static struct prefsnode * ppool_item_find(struct prefspool_ctx *ct, struct prefsnode *n, ULONG id)
{
	struct prefsnode *rn;
	struct MinList *l;
	ASSERT(ct);

	if (n)
	{
		if (n->id & DSF_LISTPOOL)
		{
			l = (struct MinList *)&n->data;
		}
		else
		{
			PDB(("wrong item\n"));
			return (NULL);
		}
	}
	else
	{
		l = &ct->l;
	}

	ITERATELIST(rn, l)
	{
		if (rn->id == id)
		{
			//D(PREFSPOOL,bug("ctx: %p, found 0x%lx\n", ct, id));
			return (rn);
		}
	}
	return (NULL);
}


/*
 * Removes an item and all the childs if it's a listpool. Pass 0 as
 * reclevel.
 */
static void ppool_item_remove(struct prefspool_ctx *ct, struct prefsnode *n, ULONG reclevel)
{
	ASSERT(ct);
	ASSERT(n);

	if (reclevel < MAXRECURSIONLEVEL)
	{
		if (n->id & DSF_LISTPOOL)
		{
			struct prefsnode *nc, *nextnc;

			ITERATELISTSAFE(nc, nextnc, &n->data)
			{
				if (nc->id & DSF_LISTPOOL)
				{
					ppool_item_remove(ct, nc, reclevel + 1);
				}
				else
				{
					FreePooled(ct->pool, nc, sizeof(*nc) + nc->size);
				}
			}
			REMOVE(n);
			FreePooled(ct->pool, n, sizeof(*n) + sizeof(struct MinList));
		}
		else
		{
			REMOVE(n);
			FreePooled(ct->pool, n, sizeof(*n) + n->size);
		}
	}
	else
	{
		PDB(("too deep recursion level\n"));
	}
}


/*
 * Adds a prefs item. 'pitem' can be:
 * - NULL: just add a normal item
 * - ptr to listpool: add the item within the listpool
 * - ptr to item: error
 *
 * Returns a pointer to the item if successful. Data is ignored for listpools.
 *
 * Note: if 'data' is NULL, it isn't copied. This allows to fill it in later, eg. during
 * I/O.
 */
APTR prefspool_item_add(APTR ctx, APTR pitem, ULONG id, CONST_APTR data, ULONG size)
{
	struct prefspool_ctx *ct = ctx;
	struct prefsnode *n;

	ASSERT(ct);
	ASSERT(ct->pool);

	if ( (n = ppool_item_find(ct, pitem, id)) )
	{
		if (!(id & DSF_LISTPOOL) && !(n->id & DSF_LISTPOOL) && (size == n->size))
		{
			/* same size, just copy over */
			if (data)
			{
				memcpy(n->data, data, size);
			}
			return (n);
		}
		else
		{
			/* different, remove it */
			ppool_item_remove(ct, n, 0);
		}
	}

	if (id & DSF_LISTPOOL)
	{
		if ( (n = AllocPooled(ct->pool, sizeof(*n) + sizeof(struct MinList))) )
		{
			n->id = id;
			n->size = 0;
			NEWLIST(&n->data);
		}
	}
	else
	{
		if ( (n = AllocPooled(ct->pool, sizeof(*n) + size)) )
		{
			n->id = id;
			n->size = size;
			if (data)
			{
				memcpy(n->data, data, size);
			}
		}
	}

	if (n)
	{
		if (pitem)
		{
			struct prefsnode *pn = pitem;

			if (pn->id & DSF_LISTPOOL)
			{
				ADDTAIL(&pn->data, n);
				pn->size++;
			}
			else
			{
				PDB(("illegal pitem 0x%lx!\n", pn->id));
				FreePooled(ct->pool, n, sizeof(*n) + (id & DSF_LISTPOOL) ? sizeof(struct MinList) : n->size);
				n = NULL;
			}
		}
		else
		{
			ADDTAIL(&ct->l, n);
		}
	}
	return (n);
}


/*
 * Removes a prefs item. 'pitem' can be:
 * - NULL: just remove a normal item
 * - ptr to listpool: removes the item within the listpool
 * - ptr to item: error
 */
void prefspool_item_remove(APTR ctx, APTR pitem, ULONG id)
{
	struct prefspool_ctx *ct = ctx;
	struct prefsnode *n;

	ASSERT(ct);

	if (pitem)
	{
		if (((struct prefsnode *)pitem)->id & DSF_LISTPOOL)
		{
			if ( (n = ppool_item_find(ct, pitem, id)) )
			{
				ppool_item_remove(ct, n, 0);
			}
		}
		else
		{
			PDB(("wrong pitem\n"));
		}
	}
	else
	{
		if ( (n = ppool_item_find(ct, NULL, id)) )
		{
			ppool_item_remove(ct, n, 0);
		}
	}
}


/*
 * Gets a prefs item. 'pitem' can be:
 * - NULL: just get a normal item
 * - ptr to listpool: get the item within the listpool
 * - ptr to item: error
 *
 * Returns a pointer to the item if successful, data in 'p' if 'p' is not
 * NULL and size in 'size' if 'size' is not NULL
 */
APTR prefspool_item_get(APTR ctx, APTR pitem, ULONG id, APTR *p, ULONG *size)
{
	struct prefspool_ctx *ct = ctx;
	struct prefsnode *n = NULL;

	ASSERT(ctx);

	if (pitem)
	{
		if (((struct prefsnode *)pitem)->id & DSF_LISTPOOL)
		{
			n = ppool_item_find(ct, pitem, id);
		}
		else
		{
			PDB(("wrong pitem\n"));
		}
	}
	else
	{
		n = ppool_item_find(ct, NULL, id);
	}

	if (n)
	{
		if (p)
		{
			*p = n->data;
		}

		if (size)
		{
			*size = n->size;
		}
	}
	return (n);
}


#define PREFS_READLONG(x) ({ if (!file_read(fh, &x, sizeof(x))) { rc = PREFSPOOL_IO_ERROR; *ioerr = IoErr(); goto fail; } })
#define PREFS_WRITELONG(x) ({ if (!file_write(fh, &x, sizeof(x))) { rc = PREFSPOOL_IO_ERROR; *ioerr = IoErr(); goto fail; } })

static LONG prefspool_oldread(struct prefspool_ctx *ct, APTR fh, LONG *ioerr)
{
	ULONG id;
	ULONG size;
	ULONG numnodes;
	struct prefsnode *pl, *pn, *n;
	LONG rc = PREFSPOOL_IO_INCONSISTENCY;

	ASSERT(ct);
	ASSERT(fh);

	D(PREFSIO,bug("ctx: %p\n", ct));


	PREFS_READLONG(numnodes);

	D(PREFSIO,bug("number of nodes to load: %ld\n", numnodes));

	while (numnodes)
	{
		PREFS_READLONG(id);
		PREFS_READLONG(size);

		D(PREFSIO,bug("got id: 0x%lx, size: %ld\n", id, size));

		if (!id || size > MAXOLDDATALENGTH)
		{
			PDB(("urgl. prefs item error: id: %ld, size: %ld\n", id, size));
			rc = PREFSPOOL_IO_INCONSISTENCY;
			goto fail;
		}

		if (id & DSF_LISTPOOL)
		{
			ULONG cnt = 1; /* it starts at 1 for historical reasons.. you don't want to know :) */
			ULONG itemcnt;
			ULONG itemsize;
			ULONG attrid;
			ULONG attrsize;

			D(PREFSIO,bug("reading listpool id 0x%lx.. number of items: %ld\n", id, size));

			if ( (pl = prefspool_item_add(ct, NULL, id, NULL, 0)) )
			{
				while (size) /* for each item.. */
				{
					PREFS_READLONG(itemcnt);
					PREFS_READLONG(itemsize);

					if (cnt == itemcnt)
					{
						if (itemsize)
						{
							if ( (pn = prefspool_item_add(ct, pl, (cnt - 1) | DSF_LISTPOOL, NULL, 0)) )
							{
								while (itemsize) /* for each attribute */
								{
									PREFS_READLONG(attrid);
									PREFS_READLONG(attrsize);

									D(PREFSIO,bug("got item (reverse: %ld) attribute id 0x%lx, size: %ld\n", itemsize, attrid, attrsize));

									if (attrid && attrsize)
									{
										if ( (n = prefspool_item_add(ct, pn, attrid, NULL, attrsize)) )
	                                    {
											if (!file_read(fh, n->data, attrsize))
											{
												PDB(("argh, prefslistpool read failed\n"));
												rc = PREFSPOOL_IO_ERROR;
												*ioerr = IoErr();
												goto fail;
											}
										}
										else
										{
											PDB(("out of memory\n"));
											rc = PREFSPOOL_IO_OUTOFMEM;
											goto fail;
										}
									}
									else
									{
										PDB(("wrong ID or size for listpool attribute\n"));
										rc = PREFSPOOL_IO_INCONSISTENCY;
										goto fail;
									}
									itemsize--;
								}
							}
							else
							{
								PDB(("out of memory\n"));
								rc = PREFSPOOL_IO_OUTOFMEM;
								goto fail;
							}
						}
						else
						{
							PDB(("prefs error, itemsize is 0\n"));
							rc = PREFSPOOL_IO_INCONSISTENCY;
							goto fail;
						}
					}
					else
					{
						PDB(("inconsistency check, item index is wrong, got: %ld, expected: %ld\n", itemcnt, cnt));
						rc = PREFSPOOL_IO_INCONSISTENCY;
						goto fail;
					}
					size--;
					cnt++;
				}
			}
			else
			{
				PDB(("out of memory\n"));
				rc = PREFSPOOL_IO_OUTOFMEM;
				goto fail;
			}
		}
		else
		{
			if ( (pn = prefspool_item_add(ct, NULL, id, NULL, size)) )
			{
				if (!file_read(fh, pn->data, size))
				{
					PDB(("read failed\n"));
					rc = PREFSPOOL_IO_ERROR;
					*ioerr = IoErr();
					goto fail;
				}
			}
			else
			{
				PDB(("out of memory\n"));
				rc = PREFSPOOL_IO_OUTOFMEM;
				goto fail;
			}
		}
		numnodes--;
	}

	if (!numnodes)
	{
		rc = PREFSPOOL_IO_OK;
	}
	fail: /* XXX: hm.. perhaps we should retry with the backup ? */

	return (rc);
}


#define CALC_CRC(var) if (crc) *crc = crc32(*crc, (UBYTE *)&var, sizeof(var))

static LONG ppool_read(struct prefspool_ctx *ct, APTR fh, struct prefsnode *pitem, struct MinList *l UNUSED, ULONG reclevel, ULONG *crc, LONG *ioerr)
{
	LONG rc = PREFSPOOL_IO_OK;

	if (reclevel < MAXRECURSIONLEVEL)
	{
		ULONG numnodes;
		ULONG id;
		ULONG size;
		struct prefsnode *n;

		PREFS_READLONG(numnodes);
		CALC_CRC(numnodes);

		D(PREFSIO,bug("ctx: %p, number of nodes to load: %ld\n", ct, numnodes));

		while (numnodes)
		{
			PREFS_READLONG(id);
			CALC_CRC(id);

			D(PREFSIO,bug("ctx: %p, id: 0x%lx\n", ct, id));

			if (id & DSF_LISTPOOL)
			{
				D(PREFSIO,bug("ctx: %p, adding listpool id 0x%lx..\n", ct, id));

				if ( (n = prefspool_item_add(ct, pitem, id, NULL, 0)) )
				{
					/* XXX: I think that is going to barf if the prefspool has no child! ppool_read() should check 'pitem' and rewind if it's wrong.. but.. how could it know? we have to check at write times! */
					rc = ppool_read(ct, fh, n, (struct MinList *)&n->data, reclevel + 1, crc, ioerr);

					if (rc != PREFSPOOL_IO_OK) break;
				}
				else
				{
					PDB(("out of memory\n"));
					rc = PREFSPOOL_IO_OUTOFMEM;
					break;
				}
			}
			else
			{
				PREFS_READLONG(size);
				CALC_CRC(size);

				D(PREFSIO,bug("ctx: %p, size: %lu\n", ct, size));

				if (size > MAXDATALENGTH)
				{
					PDB(("size bigger than %lu bytes, must be fucked\n", (ULONG)MAXDATALENGTH));
					rc = PREFSPOOL_IO_INCONSISTENCY;
					break;
				}

				if ( (n = prefspool_item_add(ct, pitem, id, NULL, size)) )
				{
					if (file_read(fh, n->data, size))
					{
						if (crc)
						{
							*crc = crc32(*crc, n->data, size);
						}
					}
					else
					{
						PDB(("read failed\n"));
						rc = PREFSPOOL_IO_ERROR;
						*ioerr = IoErr();
						break;
					}
				}
				else
				{
					PDB(("out of memory\n"));
					rc = PREFSPOOL_IO_OUTOFMEM;
					break;
				}
			}
			numnodes--;
		}
	}
	else
	{
		PDB(("too many recursions\n"));
		rc = PREFSPOOL_IO_INCONSISTENCY;
	}
	fail:
	return (rc);
}

#define PIDF_CRC32 (1 << 0UL)

LONG prefspool_read(APTR ctx, CONST_STRPTR filename, ULONG prefsid, ULONG report)
{
	struct prefspool_ctx *ct = ctx;
	ULONG v;
	APTR fh;
	LONG rc = PREFSPOOL_IO_MISSING;
	LONG _ioerr = 0;
	LONG *ioerr = &_ioerr;
	#if USE_SAFE_CONFIG
	TEXT bakname[PATH_SIZE];
	ULONG try_backup = FALSE;
	#endif

	ASSERT(ct);
	ASSERT(filename);
	ASSERT(prefsid);

	D(PREFSIO,bug("ctx: %p, opening file <%s>, id: 0x%lx\n", ctx, filename, prefsid));

	retry:

	if ( (fh = file_open(filename, MODE_OLDFILE)) )
	{
		PREFS_READLONG(v);

		D(PREFSIO,bug("ctx: %p, got id: 0x%lx\n", ctx, v));

		if ((v & 0xffffff00) == (prefsid & 0xffffff00))
		{
			if ((v & 0xff) == 0)
			{
				/*
				 * Old prefs version.
				 */
				D(PREFSIO,bug("ctx: %p, old preference format\n", ct));
				rc = prefspool_oldread(ct, fh, ioerr);
			}
			else if ((v & 0xff) == 1)
			{
				ULONG crc = 0;

				/*
				 * Current version.
				 */
				D(PREFSIO,bug("ctx: %p, preference format 1\n", ct));

				PREFS_READLONG(v);

				D(PREFSIO,bug("ctx: %p, flags: 0x%lx\n", ct, v));

				if (v & PIDF_CRC32)
				{
					crc = crc32(crc, (UBYTE *)&prefsid, sizeof(prefsid));
					crc = crc32(crc, (UBYTE *)&v, sizeof(v));

					if ((rc = ppool_read(ct, fh, NULL, &ct->l, 0, &crc, ioerr)) == PREFSPOOL_IO_OK)
					{
						PREFS_READLONG(v);

						if ((v != crc) && !_var(prefsio_ignore_crc))
						{
							D(PREFSIO,bug("ctx: %p, wrong checksum, expected 0x%lx and got 0x%lx\n", ct, crc, v));
							rc = PREFSPOOL_IO_INCONSISTENCY;
						}
					}
				}
				else
				{
					rc = ppool_read(ct, fh, NULL, &ct->l, 0, NULL, ioerr);
				}
			}
			else
			{
				D(PREFSIO,bug("ctx: %p, unknown prefsid format %ld\n", ct, v & 0xff));
				rc = PREFSPOOL_IO_INCONSISTENCY;
			}
		}
		else
		{
			D(PREFSIO,bug("ctx: %p, wrong prefsid, got 0x%lx instead of the expected 0x%lx\n", ct, v, prefsid))
			rc = PREFSPOOL_IO_INCONSISTENCY;
		}
	}
	else
	{
		D(PREFSIO,bug("ctx: %p, no prefs file\n", ct));
	}
	fail:
	if (fh)
	{
		file_close(fh);
	}

	if (rc != PREFSPOOL_IO_OK)
	{
		if (!_var(prefsio_partial))
		{
			/*
			 * Remove all the prefsnodes as there's a high
			 * chance some of them are corrupted and no way to
			 * tell for sure.
			 */
			prefspool_flush(ct);
		}

		if ((rc != PREFSPOOL_IO_MISSING) && report)
		{
			switch (rc)
			{
				case PREFSPOOL_IO_ERROR:
					SetIoErr(*ioerr);
					smartreq_info("Ambient prefs", MV_Notification_Error, "I/O error while reading\n%s.\n%S (#%N)", filename);
					break;

				case PREFSPOOL_IO_OUTOFMEM:
					smartreq_info("Ambient prefs", MV_Notification_Error, "Out of memory while reading\n%s.\nEither you're really low on memory or the preference\nfile might be broken somehow.", filename);
					break;

				case PREFSPOOL_IO_INCONSISTENCY:
					smartreq_info("Ambient prefs", MV_Notification_Error, "Inconsistency detected while reading\n%s.\nThe file is corrupt.", filename);
					break;

				default:
					smartreq_info("Ambient prefs", MV_Notification_Warning, "The programmer forgot to add a proper error message\nfor the failure to read\n%s.\nwith error: %ld. Well, it didn't work.", filename, rc);
					break;
			}
		}
		#if USE_SAFE_CONFIG
		if (filename == (CONST_STRPTR)bakname)
		{
			if ((rc == PREFSPOOL_IO_MISSING) && try_backup)
			{
				smartreq_info("Ambient prefs", MV_Notification_Warning, "No backup file %s\nUsing default preferences.", filename);
			}
		}
		else
		{
			snprintf(bakname, sizeof(bakname), "%s.bak", filename);
			D(PREFSIO,bug("ctx: %p, failed to open <%s>, trying backup file <%s>..\n", ct, filename, bakname));
			filename = bakname;
			if ((rc != PREFSPOOL_IO_MISSING) && report)
			{
				try_backup = TRUE;
				smartreq_info("Ambient prefs", MV_Notification_Help, "Now trying to open backup preference file\n%s.", filename);
			}
			goto retry;
		}
		#endif
	}
	return (rc);
}


static LONG ppool_write(struct prefspool_ctx *ct, APTR fh, struct prefsnode *pitem UNUSED, struct MinList *l, ULONG reclevel, ULONG *crc, LONG *ioerr)
{
	LONG rc = PREFSPOOL_IO_OK;

	if (reclevel < MAXRECURSIONLEVEL)
	{
		ULONG v;
		struct prefsnode *n;

		/* write size */
		v = 0;
		ITERATELIST(n, l)
		{
			v++;
		}
		D(PREFSIO,bug("ctx: %p, writing %lu nodes..\n", ct, v));

		PREFS_WRITELONG(v);
		CALC_CRC(v);

		/* then the actual nodes */
		ITERATELIST(n, l)
		{
			PREFS_WRITELONG(n->id);
			CALC_CRC(n->id);
			if (n->id & DSF_LISTPOOL)
			{
				D(PREFSIO,bug("ctx: %p, writing listpoold id 0x%lx..\n", ct, n->id));
				rc = ppool_write(ct, fh, n, (struct MinList *)&n->data, reclevel + 1, crc, ioerr);

				if (rc != PREFSPOOL_IO_OK) break;
			}
			else
			{
				PREFS_WRITELONG(n->size);
				CALC_CRC(n->size);
				if (file_write(fh, n->data, n->size))
				{
					D(PREFSIO,bug("ctx: %p, wrote id: 0x%lx, size: %ld\n", ct, n->id, n->size));
					if (crc)
					{
						*crc = crc32(*crc, n->data, n->size);
					}
				}
				else
				{
					PDB(("write failed\n"));
					rc = PREFSPOOL_IO_ERROR;
					*ioerr = IoErr();
					break;
				}
			}
		}
	}
	else
	{
		PDB(("too many recursions\n"));
		rc = PREFSPOOL_IO_INCONSISTENCY;
	}
	fail:
	return (rc);
}


LONG prefspool_write(APTR ctx, CONST_STRPTR filename, ULONG prefsid, ULONG report)
{
	struct prefspool_ctx *ct = ctx;
	APTR fh = NULL;
	LONG rc = PREFSPOOL_IO_OK;
	LONG _ioerr = 0;
	LONG *ioerr = &_ioerr;
	#if USE_SAFE_CONFIG
	TEXT filenamebuf[PATH_SIZE];
	ULONG fext = fext; /* shut up gcc. here it's *really* stupid */
	#endif

	THREAD;
	ASSERT(ct);
	ASSERT(filename);
	ASSERT(prefsid);

	if (is_device_protected(filename))
	{
		D(PREFSIO,bug("ctx: %p, media for path <%s> is write protected\n", ct, filename));
		rc = PREFSPOOL_IO_PROTECTEDMEDIA;
	}
	else
	{
		CONST_STRPTR p;

		if ((p = strrchr(filename, '/')) || (p = strrchr(filename, ':')))
		{
			TEXT dir[PATH_SIZE];

			stccpy(dir, filename, p - filename + 2);
			if (!makedir(dir))
			{
				*ioerr = IoErr();
				rc = PREFSPOOL_IO_NOCREATEDIR;
				goto fail;
			}
		}

		#if USE_SAFE_CONFIG
		fext = strlen(filename);

		if (fext >= sizeof(filenamebuf) - 5)
		{
			return (PREFSPOOL_IO_NOOPENWRITE);
		}

		/*
		 * Save to <filename>.new
		 */
		strcpy(filenamebuf, filename);
		strcpy(filenamebuf + fext, ".new");

		D(PREFSIO,bug("ctx: %p, opening <%s> for writing, id: 0x%lx\n", ct, filenamebuf, prefsid));
		if ( (fh = file_open(filenamebuf, MODE_NEWFILE)) )
		#else
		D(PREFSIO,bug("ctx: %p, opening <%s> for writing, id: 0x%lx\n", ct, filename, prefsid));
		if ( (fh = file_open(filename, MODE_NEWFILE)) )
		#endif
		{
			ULONG flags = 0;
			#if USE_PREFSCRC32
			ULONG crc = 0;

			if ((prefsid & 0xff) == 0)
			{
				prefsid |= 1;
			}
			flags |= PIDF_CRC32;
			#endif

			PREFS_WRITELONG(prefsid);
			PREFS_WRITELONG(flags);

			#if USE_PREFSCRC32
			crc = crc32(crc, (UBYTE *)&prefsid, sizeof(prefsid));
			crc = crc32(crc, (UBYTE *)&flags, sizeof(flags));
			#endif

			#if USE_PREFSCRC32
			if ((rc = ppool_write(ct, fh, NULL, &ct->l, 0, &crc, ioerr)) == PREFSPOOL_IO_OK)
			#else
			if ((rc = ppool_write(ct, fh, NULL, &ct->l, 0, NULL, ioerr)) == PREFSPOOL_IO_OK)
			#endif
			{
				#if USE_PREFSCRC32
				PREFS_WRITELONG(crc);
				#endif

				#define IDSTRING1 "$VER"
				#define IDSTRING2 ": Ambient_Prefs " LVERTAG "\0"

				if (!file_write(fh, IDSTRING1, strlen(IDSTRING1)))
				{
					*ioerr = IoErr();
					goto fail;
				}
				if (!file_write(fh, IDSTRING2, strlen(IDSTRING2) + 1))
				{
					*ioerr = IoErr();
					goto fail;
				}
			}
		}
		else
		{
			rc = PREFSPOOL_IO_NOOPENWRITE;
			*ioerr = IoErr();
		}

		fail:
		if (fh)
		{
			file_close(fh);
		}

		if (rc == PREFSPOOL_IO_OK)
		{
			#if USE_SAFE_CONFIG
			rc = PREFSPOOL_IO_ERROR;

			strcpy(filenamebuf + fext, ".bak");
			DeleteFile(filenamebuf); /* delete a possible <filename>.bak */

			Rename(filename, filenamebuf); /* <filename> -> <filename>.bak */

			strcpy(filenamebuf + fext, ".new");

			if (Rename(filenamebuf, filename)) /* <filename>.new -> <filename> */
			{
				D(PREFSIO,bug("ctx: %p, renamed <%s> to <%s>, all ok\n", ct, filenamebuf, filename));
				rc = PREFSPOOL_IO_OK;
			}
			else
			{
				strcpy(filenamebuf + fext, ".bak");
				D(PREFSIO,bug("ctx: %p, couldn't rename <%s> to <%s>, trying to put back the old file..\n", ct, filenamebuf, filename));
				if (Rename(filenamebuf, filename)) /* sigh, try to put back the old prefs file */
				{
					strcpy(filenamebuf + fext, ".new");
					D(PREFSIO,bug("ctx: %p, renamed, deleting <%s> now..\n", ct, filenamebuf));
					DeleteFile(filenamebuf); /* and delete the new one */
				}
			}
			#endif
		}
		else
		{
			if (fh)
			{
				#if USE_SAFE_CONFIG
				strcpy(filenamebuf + fext, ".bak");
				Rename(filenamebuf, filename);
				#endif
				strcpy(filenamebuf + fext, ".new");
				DeleteFile(filenamebuf);
			}

			if (report)
			{
				switch (rc)
				{
					case PREFSPOOL_IO_ERROR:
						SetIoErr(*ioerr);
						smartreq_info("Ambient prefs", MV_Notification_Error, "I/O error while writing\n%s.\n%S (#%N)", filename);
						break;

					case PREFSPOOL_IO_OUTOFMEM:
						smartreq_info("Ambient prefs", MV_Notification_Error, "Out of memory while writing\n%s.", filename);
						break;

					case PREFSPOOL_IO_INCONSISTENCY:
						smartreq_info("Ambient prefs", MV_Notification_Error, "Inconsistency detected while writing\n%s.\nThe input data is wrong.", filename);
						break;

					case PREFSPOOL_IO_NOOPENWRITE:
						SetIoErr(*ioerr);
						smartreq_info("Ambient prefs", MV_Notification_Error, "Failed to create file\n%s.\n%S (#%N)", filename);
						break;

					case PREFSPOOL_IO_NOCREATEDIR:
						SetIoErr(*ioerr);
						smartreq_info("Ambient prefs", MV_Notification_Error, "Failed to create parent directory for file\n%s.\n%S (#%N)", filename);
						break;

					default:
						smartreq_info("Ambient prefs", MV_Notification_Warning, "The programmer forgot to add a proper error message\nfor the failure to write\n%s.\nwith error: %ld. Well, it didn't work.", filename, rc);
						break;
				}
			}
		}
	}
	return (rc);
}


static ULONG ppool_copy(struct prefspool_ctx *sct, struct prefspool_ctx *dct, struct prefsnode *spn, struct prefsnode *dpn, void (*fp)(ULONG id), ULONG reclevel)
{
	ULONG rc = FALSE;

	ASSERT(sct);
	ASSERT(dct);

	if (reclevel < MAXRECURSIONLEVEL)
	{
		struct MinList *l;
		struct prefsnode *ns, *nd;

		if (spn)
		{
			ASSERT(spn->id & DSF_LISTPOOL);
			l = (struct MinList *)&spn->data;
		}
		else
		{
			l = &sct->l;
		}

		ITERATELIST(ns, l)
		{
			if (ns->id & DSF_LISTPOOL)
			{
				if ( (nd = prefspool_item_add(dct, dpn, ns->id, NULL, 0)) )
				{
					rc = ppool_copy(sct, dct, ns, nd, fp, reclevel + 1);
					if (!rc) break;
				}
				else
				{
					break;
				}
			}
			else
			{
				if (!spn)
				{
					if ( (nd = ppool_item_find(dct, NULL, ns->id)) )
					{
						if (fp && !(nd->size == ns->size && !memcmp(ns->data, nd->data, ns->size)))
						{
							fp(ns->id);
						}
					}
				}
				rc = (ULONG)prefspool_item_add(dct, dpn, ns->id, ns->data, ns->size);
				if (!rc) break;
			}
		}
	}
	else
	{
		PDB(("too deep recursion level\n"));
	}
	return (rc);
}


/*
 * Copies/overwrites the entries from ctx_src to ctx_dst and runs
 * fp() if supplied when an entry is different (and not for listpools). Not sure this function
 * will be made public though.
 */
ULONG prefspool_copy(APTR ctx_src, APTR ctx_dst, void (*fp)(ULONG id))
{
	ASSERT(ctx_src);
	ASSERT(ctx_dst);

	return (ppool_copy(ctx_src, ctx_dst, NULL, NULL, fp, 0));
}


APTR prefspool_duplicate(APTR ctx)
{
	struct prefspool_ctx *dct;

	if ( (dct = prefspool_create(prefspool_uid(ctx))) )
	{
		if (!ppool_copy(ctx, dct, NULL, NULL, NULL, 0))
		{
			prefspool_delete(dct);
			dct = NULL;
		}
	}
	return (dct);
}

