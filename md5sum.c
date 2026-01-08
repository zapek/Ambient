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
 * $Id: md5sum.c,v 1.8 2013/10/28 10:44:57 geit Exp $
 */

#include "ambient.h"

/* public */
#include <dos/dos.h>
#include <proto/exec.h>

/* private */
#include "ambient_cat.h"
#include "md5sum.h"
#include "md5.h"
#include "methodstack.h"
#include "classes.h"
#include "file_io.h"
#include "threads.h"
#include "mui_func.h"
#include "typescanner.h"


#define IOBUFFERSIZE (8192 * 2)
#define BUFFERSIZE 4096 /* XXX: perhaps should be 8192 ? test.. */


static __inline__ TEXT dtohc(UBYTE n)
{
	switch (n)
	{
		case 0: return ('0');
		case 1: return ('1');
		case 2: return ('2');
		case 3: return ('3');
		case 4: return ('4');
		case 5: return ('5');
		case 6: return ('6');
		case 7: return ('7');
		case 8: return ('8');
		case 9: return ('9');
		case 10: return ('a');
		case 11: return ('b');
		case 12: return ('c');
		case 13: return ('d');
		case 14: return ('e');
		case 15: return ('f');
	}
	return (0);
}


ULONG tr_md5sum(APTR obj, CONST_STRPTR path)
{
	APTR af;
	ULONG rc;
	UBYTE *buf;

	THREAD;
	CHECKOBJECT(obj);
	ASSERT(path);

	rc = TRUE;

	if ( (buf = malloc(BUFFERSIZE)) )
	{
		if ( (af = file_open(path, MODE_OLDFILE)) )
		{
			CONST_STRPTR sumtxt;
			UBYTE md5out[16];
			TEXT s[34];
			MD5_CTX md5ctx;
			LONG len;
			ULONG abort = FALSE;

			MD5Init(&md5ctx);

			do
			{
				if ((len = file_readmost(af, buf, BUFFERSIZE)) != -1)
				{
					if (threads_check_abort())
					{
						rc = ABORTED;
						abort = TRUE;
						break;
					}
					MD5Update(&md5ctx, buf, len);
				}
				else
				{
					abort = TRUE; /* XXX: should be I/O error */
				}
			}
			while (len == BUFFERSIZE);

			file_close(af);

			if (abort)
			{
				sumtxt = "Aborted";
			}
			else
			{
				STRPTR p;
				ULONG i;

				MD5Final(md5out, &md5ctx);
				p = s;

				for (i = 0; i < 16; i++)
				{
					*p++ = dtohc(md5out[i] >> 4);
					*p++ = dtohc(md5out[i] & 0xf);
				}
				*p = '\0';

				sumtxt = s;
			}
			methodstack_push_sync(obj, 2, MM_Infowin_UpdateMD5sum, sumtxt);
		}
		free(buf);
	}

	/* Return false if aborted by CTRL-C */
	return (rc);
}

ULONG tr_md5sum_list(APTR obj, struct MinList *l)
{
	ULONG rc = TRUE;
	struct typescannernode *tn;
	ULONG abort = FALSE;

	APTR af;
	UBYTE *buf;

	ULONG toppos = 0xffffff;	/* NAN */

	THREAD;
	CHECKOBJECT(obj);
	ASSERT(l);

	tn = FIRSTNODE( l );

	while ( !ISLISTEMPTY( l ) )
	{
		if (threads_check_abort())
		{
			abort = TRUE;
			rc = ABORTED;
		}

		if (!abort)
		{
			ULONG new_toppos;

			/*
			 * Check if view scrolled. If it did we will recalculate
			 * first node to be scanned. Some better strategy?
			 */

			methodstack_push_sync( obj, 3, OM_GET, MUIA_List_TopPixel, &new_toppos );

			if ( new_toppos != toppos )
			{
				struct typescannernode *ttn;

				toppos = new_toppos;
				tn = NULL;

				ITERATELIST( ttn, l )
				{
					ULONG isvisible;
					methodstack_push_sync( ttn->obj, 3, OM_GET, MA_ListviewEntry_IsVisible, &isvisible );

					if ( tn == NULL && isvisible )
					{
						tn = ttn;
						break;
					}
				}

				/*
				 * There is a case when all visible objects are already processed.
				 */

				if ( !tn )
				{
					tn = FIRSTNODE( l );
				}
			}

			if ( (buf = malloc(BUFFERSIZE)) )
			{
				if ( (af = file_open(tn->path, MODE_OLDFILE)) )
				{
					CONST_STRPTR sumtxt;
					UBYTE md5out[16];
					TEXT s[34];
					MD5_CTX md5ctx;
					LONG len;

					methodstack_push(tn->obj, 3, MUIM_Set, MA_Icon_MD5, GSI( MSG_MD5SUM_COMPUTINGMD5SUM) );
					methodstack_push_sync(tn->obj, 1, MM_ListviewEntry_Redraw);

					MD5Init(&md5ctx);

					do
					{
						//kprintf("Reading data\n");
						if ((len = file_readmost(af, buf, BUFFERSIZE)) != -1)
						{
							if (threads_check_abort())
							{
								abort = TRUE;
								rc = ABORTED;
								break;
							}
							MD5Update(&md5ctx, buf, len);
						}
						else
						{
							abort = TRUE; /* XXX: should be I/O error */
							rc = ABORTED;
						}
					}
					while (len == BUFFERSIZE && !abort);

					file_close(af);

					if (!abort)
					{
						STRPTR p;
						ULONG i;

						MD5Final(md5out, &md5ctx);
						p = s;

						for (i = 0; i < 16; i++)
						{
							*p++ = dtohc(md5out[i] >> 4);
							*p++ = dtohc(md5out[i] & 0xf);
						}
						*p = '\0';

						sumtxt = s;

						methodstack_push(tn->obj, 3, MUIM_Set, MA_Icon_MD5, sumtxt);
						methodstack_push_sync(tn->obj, 1, MM_ListviewEntry_Redraw);
					}
				}
				free(buf);
			}

		}

		/*
		 * If we reached end of list we need to check if we skipped some nodes.
		 */

		{
			struct typescannernode *ttn;

			if ( tn != LASTNODE( l ) )
			{
				ttn = NEXTNODE( tn );
			}
			else
			{
				ttn = FIRSTNODE( l );
			}

			REMOVE(tn);
			free(tn);
			tn = ttn;
		}
	}

	free(l);

	return (rc);
}
