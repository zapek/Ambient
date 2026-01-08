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
 * $Id: findver.c,v 1.10 2017/08/09 23:26:04 cyfm Exp $
 */

#include "ambient.h"

/* public */
#include <dos/dos.h>
#include <proto/exec.h>

/* private */
#include "mui_func.h"
#include "ambient_cat.h"
#include "findver.h"
#include "methodstack.h"
#include "classes.h"
#include "file_io.h"
#include "threads.h"
#include "typescanner.h"

/*
 * Improvement ideas:
 * - if $VER can't be found, try LoadSeg() the file and scan for resident
 *   tags. see if rt_IdString has sensible version info, and use that. - piru
 */

#define IOBUFFERSIZE (8192 * 16)
#define BUFFERSIZE   4096 /* XXX: perhaps should be 8192 ? test.. */

/*
 * XXX: I think it craps out when a verstring
 * without null terminator is found. it displays the trash afterwards.
 * might be only on files when the verstring is last.
 */
ULONG tr_findver(APTR obj, CONST_STRPTR path)
{
	APTR af;
	STRPTR buf;
	ULONG i, rc;
	TEXT strbuf[128];
	STRPTR mp;
	ULONG tofind = 0;
	ULONG copied = 0;
	ULONG abort  = FALSE;

	THREAD;
	CHECKOBJECT(obj);
	ASSERT(path);

	rc = TRUE;

	if ( (buf = malloc(BUFFERSIZE)) )
	{
		if ( (af = file_open(path, MODE_OLDFILE)) )
		{
			LONG len;
			strbuf[0] = '$';
			strbuf[1] = 'V';
			strbuf[2] = 'E';
			strbuf[3] = 'Q' + 1;  /* that's a useless trick, gcc optimizes that anyway */
			strbuf[4] = ':';

			mp = strbuf;

			while (!abort)
			{
				if ( (len = file_readmost(af, buf, BUFFERSIZE)) > 0 )
				{
					if ( (threads_check_abort()) )
					{
						rc = ABORTED;
						abort = TRUE;
						goto done;
					}

					/*
					 * Simple brute force search. We could use
					 * Boyer-Moore or so but it would get
					 * even more messy..
					 */
					for (i = 0; i < len; i++)
					{
						UBYTE c = buf[i]; /* current char */

						if (tofind == 5)
						{
							BOOL eov = (c == '\0') || (c == '\n') || (c == '\r'); /* possible end of VER string */

							if ((!copied) && (c == ' '))
							{
								continue; /* skip first space */
							}

							/* little heuristic */
							if (!eov)
							{
								*mp = c;

								mp++;
								copied++;
							}

							/* copy mode */
							if ((copied >= 127) || eov)
							{
								*mp = '\0'; /* terminate      */
								tofind = 7; /* what a mess :) */
								goto done;
							}
						}
						else
						{
							/* find mode */
							if (c == *mp)
							{
								mp++;
								tofind++;

								if (tofind == 5)
								{
									mp = strbuf;
									continue;
								}
							}
							else if (c == strbuf[0]) /* sometimes a '$' is directly in front of the $VER: string */
							{
								tofind = 1;
								mp = strbuf+1;
							}
							else
							{
								mp = strbuf;
								tofind = 0;
							}
						}
					}
				}
				else
				{
					tofind = 0;
					break;
				}
			}
			done:

			if(!abort)
			{
				if (tofind == 7)
				{
					methodstack_push_sync(obj, 2, MM_Infowin_UpdateVersion, strbuf);
				}
				else
				{
					methodstack_push_sync(obj, 2, MM_Infowin_UpdateVersion, GSI(MSG_FINDVER_VERSIONNOTFOUND) );
				}
			}
			file_close(af);
		}
		free(buf);
	}

	/* Return false if aborted */
	return (rc);
}

ULONG tr_findver_list(APTR obj, struct MinList *l)
{
	ULONG rc = TRUE;
	struct typescannernode *tn;
	ULONG abort = FALSE;

	APTR af;
	STRPTR buf;
	ULONG i;
	TEXT strbuf[128];
	STRPTR mp;
	ULONG tofind = 0;
	ULONG copied = 0;

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
					LONG len;
					strbuf[0] = '$';
					strbuf[1] = 'V';
					strbuf[2] = 'E';
					strbuf[3] = 'Q' + 1;  /* that's a useless trick, gcc optimizes that anyway */
					strbuf[4] = ':';

					mp = strbuf;
					tofind = 0;
					copied = 0;

					methodstack_push(tn->obj, 3, MUIM_Set, MA_Icon_Version, GSI( MSG_FINDVER_RETRIEVINGVERSION ));
					methodstack_push_sync(tn->obj, 1, MM_ListviewEntry_Redraw);

					while (1)
					{
						if ( (len = file_readmost(af, buf, BUFFERSIZE)) > 0 )
						{
							if (threads_check_abort())
							{
								abort = TRUE;
								rc = ABORTED;
								goto done;
							}

							/*
							 * Simple brute force search. We could use
							 * Boyer-Moore or so but it would get
							 * even more messy..
							 */
							for (i = 0; i < len; i++)
							{
								UBYTE c = buf[i];

								if (tofind == 5)
								{
									BOOL eov = (c == '\0') || (c == '\n') || (c == '\r'); /* possible end of VER string */

									if (!copied && c == ' ')
									{
										continue; /* skip first space */
									}

									/* little heuristic */
									if (!eov)
									{
										*mp = c;

										mp++;
										copied++;
									}

									/* copy mode */
									if (copied >= 127 || eov)
									{
										*mp = '\0'; /* terminate      */
										tofind = 7; /* what a mess :) */
										goto done;
									}
								}
								else
								{
									/* find mode */
									if (c == *mp)
									{
										mp++;
										tofind++;

										if (tofind == 5)
										{
											mp = strbuf;
											continue;
										}
									}
									else if (c == strbuf[0]) /* sometimes a '$' is directly in front of the $VER: string */
									{
										tofind = 1;
										mp = strbuf+1;
									}
									else
									{
										mp = strbuf;
										tofind = 0;
									}
								}
							}
						}
						else
						{
							tofind = 0;
							break;
						}
					}

					done:

					if (tofind == 7)
					{
						ULONG found = FALSE;
						STRPTR ptr = strbuf;

						while(*ptr && !found)
						{
							if(*ptr<='9' && *ptr>='0' &&
							   (ptr == (STRPTR)strbuf ||
								( ptr > (STRPTR)strbuf &&
								  (*(ptr-1) == ' ' || tolower(*(ptr-1)) == 'v')
								)
							   )
							  )
							{
								found = TRUE;
							}
							else
							{
								ptr++;
							}
						}

						if(found)
						{
							TEXT shortver[32];
							ULONG end = FALSE;
							ULONG i = 0;
							while(*ptr && !end)
							{
								if(*ptr && *ptr!= ' ' && *ptr !='[' && *ptr != '\n' &&
								   *ptr != '>' && *ptr !=')')
								{
									shortver[i]=*ptr;
								}
								else
								{
									shortver[i]=0;
									end = TRUE;
								}

								i++;
								ptr++;
							}

							methodstack_push(tn->obj, 3, MUIM_Set, MA_Icon_Version, shortver);
							methodstack_push_sync(tn->obj, 1, MM_ListviewEntry_Redraw);
						}
					}
					else
					{
						methodstack_push(tn->obj, 3, MUIM_Set, MA_Icon_Version, "");
						methodstack_push_sync(tn->obj, 1, MM_ListviewEntry_Redraw);
					}
					file_close(af);
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
