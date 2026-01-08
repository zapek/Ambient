/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2018 Ambient Open Source Team
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
 * $Id: movefile.c,v 1.12 2018/07/24 09:48:52 itix Exp $
 */

#include "ambient.h"

/* public */
#include <proto/dos.h>
#include <proto/exec.h>

/* private */
#include "deletefile.h"
#include "movefile.h"
#include "fastcopy.h"
#include "name.h"
#include "mui_func.h"
#include "methodstack.h"
#include "file_func.h"
#include "notify.h"
#include "time_func.h"
#include "methodstack.h"
#include "rename.h"
#include "threads.h"


#define NOSOURCE 2

ULONG tr_movefile(APTR obj, APTR refwin, CONST_STRPTR from, CONST_STRPTR to, ULONG copy, APTR progressobj, ULONG * filecount, QUAD * totaldone, ULONG noicon, ULONG rename, ULONG *reqflags)
{
	TEXT fullpath[PATH_SIZE];
	ULONG len, dummy;
	STRPTR nn;
	ULONG rc = MOVE_FAILED;

	THREAD;
	ASSERT(from);
	ASSERT(to);

	stccpy(fullpath, to, sizeof(fullpath));
	len = strlen(fullpath);

	{
		ULONG time;
		TEXT newname2[PATH_SIZE] = "";

		if(rename)
		{
			STRPTR newname = NULL;
			APTR patternrenamewin = NULL;

			patternrenamewin = (APTR) methodstack_push_sync( app, 4, MM_Application_CreatePatternRenamewin, thread_get(), from, NULL);

			if ( patternrenamewin )
			{
				methodstack_push( patternrenamewin, 3, MUIM_Set, MUIA_Window_Open, TRUE );

				thread_wait();

				methodstack_push_sync( patternrenamewin, 3, OM_GET, MA_PatternRenamewin_NewName, &newname );

				if(newname)
				{
					stccpy(newname2, newname, sizeof(newname2));
					AddPart(fullpath, newname, sizeof(fullpath));
				}

				methodstack_push( app, 2, MM_Application_DisposeWindow, patternrenamewin );

				if(!newname)
				{
					return MOVE_NOSOURCE;
				}
			}
			else
			{
				return MOVE_FAILED;
			}
		}
		else
		{
			AddPart(fullpath, FilePart(from), sizeof(fullpath));
		}

		time = timedm();

		if (same_volume(from, to))
		{
			if (copy)
			{
				switch (fastcopy(progressobj, from, fullpath, TRUE, &time, *filecount, totaldone, reqflags))
				{
					case FASTCOPY_OK:
						(*filecount)++;
						notify_action(fullpath, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Create);
						rc = MOVE_OK;
						break;

					case FASTCOPY_NOSOURCE:
						rc = MOVE_NOSOURCE;
						break;

					case FASTCOPY_FAILED:
						rc = MOVE_FAILED;
						break;

					case FASTCOPY_ABORTED:
						notify_action(fullpath, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Create);
						rc = MOVE_ABORTED;
						break;

					#ifdef DEBUG
					default:
						PDB(("argh\n"));
						break;
					#endif
				}
			}
			else
			{
				switch(tr_rename(from, NULL, fullpath, TRUE, FALSE, reqflags))
				{
					case RENAME_OK:
						notify_action(from, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Delete);
						notify_action(fullpath, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Create);
						rc = MOVE_OK;
						break;
					case RENAME_NOSOURCE:
						rc = MOVE_NOSOURCE;
						break;
					case RENAME_FAILED:
						rc = MOVE_FAILED;
						break;
					default:
					case RENAME_ABORTED:
						rc = MOVE_ABORTED;
				}
			}

			/* Rename/copy .info file as well */
			if (!noicon && (rc == MOVE_OK || rc == MOVE_NOSOURCE) && !name_isinfo(from) && (nn = name_build_info(from)) && exists(nn))
			{
				fullpath[len] = '\0';

				if(rename && newname2[0])
				{
					TEXT dest[PATH_SIZE];
					STRPTR ptr = NULL;
					stccpy(dest, fullpath, sizeof(dest));
					AddPart(dest, newname2, sizeof(dest));
					ptr = name_build_info(dest);

					if(ptr)
					{
						stccpy(fullpath, ptr, sizeof(fullpath));
						name_delete(ptr);
					}
				}
				else
				{
					AddPart(fullpath, FilePart(nn), sizeof(fullpath));				  
				}

				if (copy)
				{
					rc = fastcopy(progressobj, nn, fullpath, TRUE, &time, *filecount, totaldone, reqflags);
						
					if (rc != FASTCOPY_FAILED)
					{
						if (rc == FASTCOPY_OK)
						{
							(*filecount)++;
							notify_action(fullpath, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Create);
						}
						/* an icon is optional */
						rc = MOVE_OK;
					}
					else
					{
						rc = MOVE_FAILED;
					}
				}
				else
				{
					switch(tr_rename(nn, NULL, fullpath, TRUE, FALSE, reqflags))
					{
						case RENAME_OK:
							notify_action(nn, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Delete);
							notify_action(fullpath, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Create);
							rc = MOVE_OK;
							break;
						case RENAME_FAILED:
							rc = MOVE_FAILED;
							break;
						case RENAME_NOSOURCE:
							rc = MOVE_NOSOURCE;
							break;
						default:
						case RENAME_ABORTED:
							rc = MOVE_ABORTED;
					}
				}
				name_delete(nn);
			}
		}
		else
		{				 
			switch (fastcopy(progressobj, from, fullpath, TRUE, &time, *filecount, totaldone, reqflags))
			{
				case FASTCOPY_OK:
					(*filecount)++;
					notify_action(fullpath, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Create);
					rc = MOVE_OK;
					break;
					
				case FASTCOPY_NOSOURCE:
					rc = MOVE_NOSOURCE;
					break;

				case FASTCOPY_FAILED:
					rc = MOVE_FAILED;
					break;

				case FASTCOPY_ABORTED:
					notify_action(fullpath, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Create);
					rc = MOVE_ABORTED;
					break;

				#ifdef DEBUG
				default:
					PDB(("argh\n"));
					break;
				#endif
			}

			if (!copy && (rc == MOVE_OK))
			{
				if (deletefile(obj, refwin, from, NULL, &dummy, TRUE, reqflags))
				{
					rc = MOVE_OK;
				}
			}

			/* Rename .info file as well */
			if (!noicon && (rc == MOVE_OK || rc == MOVE_NOSOURCE) && !name_isinfo(from) && (nn = name_build_info(from)))
			{
				if(exists(nn))
				{
					fullpath[len] = '\0';

					if(rename && newname2[0])
					{
						TEXT dest[PATH_SIZE];
						STRPTR ptr = NULL;
						stccpy(dest, fullpath, sizeof(dest));
						AddPart(dest, newname2, sizeof(dest));
						ptr = name_build_info(dest);

						if(ptr)
						{
							stccpy(fullpath, ptr, sizeof(fullpath));
							name_delete(ptr);
						}
					}
					else
					{
						AddPart(fullpath, FilePart(nn), sizeof(fullpath));
					}

					rc = fastcopy(progressobj, nn, fullpath, TRUE, &time, *filecount, totaldone, reqflags);

					if (rc != FASTCOPY_FAILED)
					{
						if (rc == FASTCOPY_OK)
						{
							(*filecount)++;
							notify_action(fullpath, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Create);
						}
						/* an icon is optional */
						rc = MOVE_OK;
					}
					else
					{
						rc = MOVE_FAILED;
					}

					if (!copy && (rc == MOVE_OK))
					{
						if (deletefile(obj, refwin, nn, NULL, &dummy, TRUE, reqflags))
						{
							rc = MOVE_OK;
						}
					}
				}
				name_delete(nn);
			}
		}
	}

	return (rc);
}
