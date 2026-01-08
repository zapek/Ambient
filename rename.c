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
 * $Id: rename.c,v 1.15 2011/12/07 23:26:09 fab Exp $
 */

#include "ambient.h"

/* public */
#include <proto/dos.h>

/* private */
#include "ambient_cat.h"
#include "rename.h"
#include "fileaction.h"
#include "file_func.h"
#include "name.h"
#include "notify.h"
#include "methodstack.h"
#include "mui_func.h"
#include "smartreq.h"
#include "threads.h"
#include "doslistcache.h"


#define RENAME_CANCEL  0
#define RENAME_PROCEED 1
#define RENAME_SKIP    2

static ULONG _rename(CONST_STRPTR from, CONST_STRPTR to, ULONG noicon, ULONG notify, ULONG *reqflags)
{
	ULONG rc = FALSE;
	
	THREAD;
	ASSERT(from);
	ASSERT(to);

	if (isdevicename(from))
	{
		if (Relabel(from, to))
		{
			struct dlcnode *dlcn;
			TEXT t[PATH_SIZE];
			ULONG l = min( strlen( to ), sizeof( t ) - 2 );

			rc = RENAME_OK;

			memcpy( t, to, l );
			t[ l ] = ':';
			t[ l + 1 ] = '\0';

			doslistcache_update();

			/* Hack the state so that volumes isn't shown twice on next device_show event */
			doslistcache_lock();
			ITERATEDLC(dlcn)
			{
				if (!strcmp(dlcn->name, to))
				{
					dlcn->state = DLC_ADDED;
					break;
				}
			}
			doslistcache_unlock();

			if(notify)
			{
				notify_action(from, NOTIFYTAG_Monitor_Device, NOTIFYTAG_Monitor_Device_Name, t);
			}
		}
		else
		{
			smartreq_request_sync(NULL, GSI(MSG_RENAME_TITLE), GSI(MSG_RENAME_OK), MV_Notification_Error, GSI(MSG_RENAME_CANTBERENAMED), from);
			rc = RENAME_FAILED;
		}
	}
	else
	{
		STRPTR ifrom = NULL, ito = NULL;
		TEXT t[PATH_SIZE];
		ULONG wasdir, res = RENAME_PROCEED;

		stccpy(t, from, (ULONG)FilePart(from) - (ULONG)from + 1);
		AddPart(t, to, PATH_SIZE);

		if (!strcmp(from, t))
		{
			rc = RENAME_OK;
			return (rc);
		}

		if(!stricmp(from, t))
		{
			res = RENAME_PROCEED;
			goto doit;
		}

		/* Supress DOS Notify for source and destination views */

		if ((wasdir = isdir(t)))
		{
			CONST_STRPTR buts = GSI(MSG_RENAME_SKIPCANCEL);

			res = smartreq_request_sync(NULL, GSI(MSG_RENAME_TITLE), buts, MV_Notification_Warning, GSI(MSG_RENAME_FILEISADIRECTORY), t);
			switch(res)
			{
				case 1:
					res = RENAME_SKIP;
					break;
				case 0:
					res = RENAME_CANCEL;
					break;
			}
		}

		/* same thing should be done for .info in !noicon mode */
		if(!wasdir && exists(t))
		{
			ULONG mask = 0;
			ULONG deleteprotected = 0;
			ULONG writeprotected  = 0;

			switch (fileaction_replace(from, t, FILEACTION_REPLACE_MOVE, reqflags))
			{
				case FILEACTION_PROCEED:
					res = RENAME_PROCEED;
					break;
				case FILEACTION_SKIP:
					res = RENAME_SKIP;
					break;
				case FILEACTION_CANCEL:
					res = RENAME_CANCEL;
					break;
			}

			if (res == RENAME_PROCEED && dosprotection_get(t, &mask))
			{
				deleteprotected = ((mask & PROTF_DELETE) == 0);
				writeprotected  = ((mask & PROTF_WRITE)  == 0) ;

				if (deleteprotected || writeprotected)
				{
					switch (fileaction_unprotect(t, FILEACTION_UNPROTECT_FROM_OVERWRITE, reqflags))
					{
						case FILEACTION_PROCEED:
							res = RENAME_PROCEED;
							break;

						case FILEACTION_SKIP:
							res = RENAME_SKIP;
							break;

						case FILEACTION_CANCEL:
							res = RENAME_CANCEL;
							break;
					}
				}
			}

			if(res == RENAME_PROCEED)
			{
				if(exists(from))
				{
					if(!DeleteFile(t))
					{
						/* may be annoying, but user should really have a feedback */
						smartreq_request_sync(NULL, GSI(MSG_RENAME_TITLE), GSI(MSG_RENAME_OK), MV_Notification_Error, GSI(MSG_RENAME_FILECANTBERENAMED), to);

						/* Restore original flags */
						if(deleteprotected)
						{
							dosprotection_clear(t, PROTF_DELETE);
						}

						if(writeprotected)
						{
							dosprotection_clear(t, PROTF_WRITE);
						}
						res = RENAME_CANCEL;
					}
					else
					{
						notify_action(t, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Delete);
					}
				}
			}
		}

		if(res == RENAME_SKIP)
		{
			rc = RENAME_NOSOURCE;
		}
		else if(res == RENAME_CANCEL)
		{
			rc = RENAME_ABORTED;
		}

doit:
		if(res == RENAME_PROCEED)
		{
			ULONG hassource;
			ULONG hasicon;
			ULONG lonelyicon;

			ifrom = name_build_info(from);
			hassource = exists(from);
			hasicon = ifrom && exists(ifrom);
			lonelyicon = hasicon && !hassource;

			if (!noicon && hasicon)
			{
				if ((ito = name_build_info(t)))
				{
					if(Rename(ifrom, ito) && notify) /* XXX: check retcode (and ponder what to do with this.. */
					{
						notify_action(ifrom, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Name, ito); /* this notification is needed in lister mode */
					}
					else
					{
						smartreq_request_sync(NULL, GSI(MSG_RENAME_TITLE), GSI(MSG_RENAME_OK), MV_Notification_Error, GSI(MSG_RENAME_CANTBERENAMED), ifrom);
					}
				}
			}

			if (Rename(from, t))
			{
				rc = RENAME_OK;

				if(notify)
				{
					notify_action(from, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Name, t);
				}
			}
			else
			{
				if(!hassource)
				{
					rc = RENAME_NOSOURCE;
				}
				else
				{
					rc = RENAME_FAILED;
				}

				// Avoid showing the error requester for the case where a lonely icon is renamed.
				if(rc == RENAME_FAILED || !lonelyicon)
				{
                    smartreq_request_sync(NULL, GSI(MSG_RENAME_TITLE), GSI(MSG_RENAME_OK), MV_Notification_Error, GSI(MSG_RENAME_CANTBERENAMED), from);
				}
			}

			if(ito)
			{
				name_delete(ito);
			}

			if(ifrom)
			{
				name_delete(ifrom);
			}
		}
	}

	return (rc);
}

static ULONG is_pattern(STRPTR name) 
{
	ULONG count = 0;

	while(*name)
	{
		if(*name == '*')
			count++;
		name++;
	}

    /* we only accept pattern with 1 joker */
	return (count == 1);
}

static ULONG match_pattern(STRPTR filename, STRPTR srcpat)
{
	STRPTR ptrfile = filename;
	STRPTR ptrsrc  = srcpat;

	while(*ptrsrc && *ptrfile && (toupper(*ptrsrc) == toupper(*ptrfile)))
	{
		ptrsrc++;
		ptrfile++;
	}

	if(*ptrsrc == 0 || *ptrsrc != '*') /* filename == pattern or difference found before joker -> rejected or doesn't match */
		return FALSE;

	ptrsrc = srcpat + strlen(srcpat) - 1;
	ptrfile = filename + strlen(filename) - 1;
	while((ptrsrc >= srcpat) && (ptrfile >= filename) && (toupper(*ptrsrc) == toupper(*ptrfile)))
	{
		ptrsrc--;
		ptrfile--;
	}

	if(*ptrsrc != '*') /* filename == pattern or difference found before joker -> rejected or doesn't match */
		return FALSE;

	return TRUE;
}

/* Get pattern-processed name */
static STRPTR get_new_name(STRPTR filename, STRPTR srcpat, STRPTR dstpat, STRPTR newname, ULONG maxlen)
{
	STRPTR ptrfile = filename;
	STRPTR ptrsrc  = srcpat;
	int startidx, endidx;

	/* get start position of the expanded string */
	while(*ptrsrc && *ptrfile && (toupper(*ptrsrc++) == toupper(*ptrfile++)));
	startidx = --ptrfile - filename;

	/* get end position of the expanded string */
	ptrsrc = srcpat + strlen(srcpat) - 1;
	ptrfile = filename + strlen(filename) - 1;
	while((ptrsrc >= srcpat) && (ptrfile >= filename) && (toupper(*ptrsrc--) == toupper(*ptrfile--)));
	endidx = ++ptrfile - filename;

	if( (strlen(dstpat) - 1 +(endidx - startidx + 1) + 1) <= maxlen)
	{
		STRPTR ptrnewfile = newname;
		STRPTR ptrdst = dstpat;

		/* copy dst pattern until joker is found */
		while(*ptrdst && *ptrdst != '*')
		{
			*ptrnewfile++ = *ptrdst++;
		}

		/* concatenate expanded string */
		memcpy(ptrnewfile, filename + startidx, endidx - startidx + 1);
		ptrnewfile += endidx - startidx + 1;

		/* concatenate end of dst pattern */
		if(*ptrdst)
		{
			ptrdst++;

			while(*ptrdst && *ptrdst != '*')
			{
				*ptrnewfile++ = *ptrdst++;
			}
		}

		*ptrnewfile = 0;

		return newname;
	}
	else
	{
		return NULL;
	}
}

ULONG tr_rename(CONST_STRPTR from, CONST_STRPTR * pathlist, CONST_STRPTR to, ULONG noicon, ULONG notify, ULONG *reqflags)
{
	/* Batch rename */
	if(pathlist && *pathlist)
	{
		STRPTR * ptr = (STRPTR *) pathlist;
		STRPTR first = *ptr;
		ULONG done = FALSE;
		ULONG rc = RENAME_NOSOURCE;

		methodstack_push_sync( app, 3, MM_Application_EnableDOSNotify, first, FALSE );

		while(!done && *ptr)
		{
			APTR patternrenamewin = NULL;

			patternrenamewin = (APTR) methodstack_push_sync( app, 4, MM_Application_CreatePatternRenamewin, thread_get(), NULL, ptr);

			if ( patternrenamewin )
			{
				ULONG result;

				methodstack_push( patternrenamewin, 3, MUIM_Set, MUIA_Window_Open, TRUE );

				thread_wait();

				methodstack_push_sync( patternrenamewin, 3, OM_GET, MA_PatternRenamewin_Result, &result );

				switch(result)
				{
					case RENAME_PROCEED:
					{
						STRPTR oldname = NULL;
						STRPTR newname = NULL;
						
						methodstack_push( patternrenamewin, 3, OM_GET, MA_PatternRenamewin_OldName, &oldname );
						methodstack_push_sync( patternrenamewin, 3, OM_GET, MA_PatternRenamewin_NewName, &newname );

						if(newname && oldname)
						{
							/* Both fields are pattern, so we apply pattern rename to the remaining files */
							if(is_pattern(oldname) && is_pattern(newname))
							{
								/* Don't refresh for each rename */
								notify_action(first, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_Enable, FALSE);

								/* XXX: We should open a progress window */
								while(!done && *ptr)
								{
									STRPTR name = FilePart(*ptr);

									if(match_pattern(name, oldname))
									{
										TEXT processedname[PATH_SIZE+1];

										if( get_new_name(name, oldname, newname, processedname, sizeof(processedname)) )
										{
											rc = _rename(*ptr, processedname, noicon, notify, reqflags);

											if(rc == RENAME_ABORTED)
											{
												done = TRUE;
											}
										}
									}
									ptr++;
								}

								notify_action(first, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_Enable, TRUE);

								done = TRUE;
							}
							else /* Standard rename */
							{
								rc = _rename(*ptr, newname, noicon, notify, reqflags);

								if(rc == RENAME_ABORTED)
								{
									done = TRUE;
								}
							}
						}
						break;
					}

					case RENAME_CANCEL:
						done = TRUE;
						break;

					case RENAME_SKIP:
						break;
				}

				methodstack_push( app, 2, MM_Application_DisposeWindow, patternrenamewin );
			}

			ptr++;
		}

		methodstack_push_sync( app, 3, MM_Application_EnableDOSNotify, first, TRUE );

		return rc; 
	}
	/* Standard rename */
	else if(from && *from)
	{
		ULONG rc;

		methodstack_push_sync( app, 3, MM_Application_EnableDOSNotify, from, FALSE );

		rc = _rename(from, to, noicon, notify, reqflags);

		methodstack_push_sync( app, 3, MM_Application_EnableDOSNotify, from, TRUE );

		return rc;
	}
	else
	{
		return RENAME_NOSOURCE;
	}
}

