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
 * $Id: deletedir.c,v 1.11 2019/02/19 17:53:29 bitrocky Exp $
 */

#include "ambient.h"

/* public */
#include <proto/dos.h>
#include <proto/exec.h>

/* private */
#include "ambient_cat.h"
#include "deletedir.h"
#include "fileaction.h"
#include "recurse.h"
#include "name.h"
#include "methodstack.h"
#include "mui_func.h"
#include "smartreq.h"
#include "notify.h"
#include "file_func.h"


struct skipnode {
	struct MinNode n;
	ULONG doscode;
};


struct deletestuff {
	APTR progressobj;
	ULONG count;
	ULONG *flags;
	struct MinList skiplist;
};

static ULONG deletefile(APTR obj UNUSED, CONST_STRPTR path, LONG type UNUSED, ULONG prot UNUSED, UQUAD size UNUSED, APTR userdata, CONST_STRPTR comment UNUSED)
{
	ULONG rc = FALSE;
	struct deletestuff *ds = (struct deletestuff *)userdata;
	ASSERT(path);

	methodstack_push_sync(ds->progressobj, 5, MM_Progresswin_Update, FilePart(path), NULL, ds->count++, NULL);
	
	restart_deletefile:

	if (DeleteFile(path))
	{
		rc = TRUE;
	}

	if (rc)
	{
		notify_action(path, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Delete);
	}
	else
	{
		ULONG ioerr;
		struct skipnode *sn;

		ioerr = IoErr();

		ITERATELIST(sn, &ds->skiplist)
		{
			if (sn->doscode == ioerr)
			{
				/* skip that one */
				return (TRUE);
			}
		}

		switch (ioerr)
		{
			case ERROR_DIRECTORY_NOT_EMPTY:
				rc = TRUE; /* that is ok because it's our job to delete the file content */
				break;

			case ERROR_DELETE_PROTECTED:
				switch (fileaction_unprotect(path, FILEACTION_UNPROTECT_FROM_DELETION, ds->flags))
				{
					case FILEACTION_CANCEL:
						rc = FALSE;
						break;

					case FILEACTION_PROCEED :
						goto restart_deletefile;

					case FILEACTION_SKIP  :
						rc = TRUE;
						break;
				}
				break;

			default:
				/* XXX: that one needs to skip all the same errno (for skip all) !! */
				switch (smartreq_request_sync(NULL, GSI( MSG_DELETEDIR_TITLE ), GSI( MSG_DELETEDIR_SKIPSKIPALWAYSRETRYABORT ), MV_Notification_Error, GSI( MSG_DELETEDIR_ERRORWHILEDELETINGFILE ), path))
				{
					case 3: /* retry */
						goto restart_deletefile;
						break;

					case 1: /* skip */
						rc = TRUE;
						break;

					case 2: /* skip all */
						{
							if ((sn = malloc(sizeof(*sn))))
							{
								sn->doscode = ioerr;
								ADDTAIL(&ds->skiplist, sn);
							}
							/* XXX */
							rc = TRUE;
						}
						break;

					case 0: /* abort */
						rc = FALSE;
						break;

					#ifdef DEBUG
					default:
						PDB(("case not handled\n"));
						break;
					#endif
				}
				break;
		}
	}
	return (rc);
}


ULONG deletedir(APTR obj, APTR refwin UNUSED, STRPTR path, APTR progressobj, ULONG * filecount, BOOL noicon, ULONG *flags)
{
	STRPTR nn;
	struct deletestuff ds;
	struct skipnode *sn, *nextsn;
	ULONG rc = FALSE;
	BPTR l;
	ULONG unprot = FALSE;

	THREAD;
	ASSERT(path);

	ds.progressobj = progressobj;
	ds.count = *filecount;
	ds.flags = flags;

	NEWLIST(&ds.skiplist);

	if ((l = Lock(path, ACCESS_READ)))
	{
		ULONG skip = FALSE;
		D_S(struct FileInfoBlock, fib);

		/*
		 * Maybe perform this check only after delete error? - piru
		 */
		restart_checkprot:

		if (Examine(l, fib))
		{
			if (fib->fib_Protection & FIBF_DELETE) /* inverted, dos moron, etc.. */
			{
				switch (fileaction_unprotect(path, FILEACTION_UNPROTECT_FROM_DELETION, flags))
				{
					case FILEACTION_CANCEL:
						break;

					case FILEACTION_PROCEED :
						goto restart_checkprot;

					case FILEACTION_SKIP  :
						skip = TRUE;
						rc = TRUE;
						break;
				}
			}
		}
		/* XXX */

		UnLock(l);

		if (!skip)
		{
			if (recurse(obj, path, "#?", NULL, NULL, deletefile, &ds))
			{
				*filecount = ds.count;
				if (unprot)
				{
					if (!dosprotection_set(path, PROTF_DELETE))
					{
						smartreq_info( GSI( MSG_DELETEDIR_TITLE ), MV_Notification_Error, GSI( MSG_DELETEDIR_ERRORUNABLETOSETPROTECTON ), path);
					}
				}

				restart_deletedir:

				if ((rc = DeleteFile(path) != 0))
				{
					(*filecount)++;
					notify_action(path, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Delete);
				}
				else
				{
					if (IoErr() == ERROR_DIRECTORY_NOT_EMPTY)
					{
						rc = TRUE;
					}
					else
					{
						switch (smartreq_request_sync(NULL, GSI( MSG_DELETEDIR_TITLE ), GSI( MSG_DELETEDIR_RETRYCANCEL ), MV_Notification_Error, GSI( MSG_DELETEDIR_ERRORWHILEDELETINGDIRECTORY ), path))
						{
							case 1: /* retry */
								goto restart_deletedir;
								break;

							case 0: /* cancel */
								break;
							
							#ifdef DEBUG
							default:
								PDB(("case not handled\n"));
								break;
							#endif
						}
					}
				}
			}
		}
	}
	else
	{
		/*
		 * Probably an icon alone.
		 */
		rc = TRUE;
	}

	if (rc)
	{
		if (noicon)
			goto done;

		if ((nn = name_build_info(path)))
		{
			BPTR l;
				
			/*
			 * Check if the icon is here.
			 */
			if ((l = Lock(nn, ACCESS_READ)))
			{
				UnLock(l);
					
				restart_deleteicon:

				if ((rc = DeleteFile(nn) != 0))
				{
					(*filecount)++;
					notify_action(nn, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Delete);
				}
				else
				{
					switch (IoErr())
					{
						case ERROR_DELETE_PROTECTED:
							switch (fileaction_unprotect(nn, FILEACTION_UNPROTECT_FROM_DELETION, flags))
							{
								case FILEACTION_CANCEL:
									rc = FALSE;
									break;

								case FILEACTION_PROCEED :
									goto restart_deleteicon;

								case FILEACTION_SKIP  :
									rc = TRUE;
									break;
							}
							break;

						default:
							switch (smartreq_request_sync(NULL, GSI( MSG_DELETEDIR_TITLE ), GSI( MSG_DELETEDIR_RETRYSKIPICONCANCEL ), MV_Notification_Error, GSI( MSG_DELETEDIR_ERRORWHILEDELETINGDIRECTORYICON ), path))
							{
								case 1: /* retry */
									goto restart_deleteicon;
									break;

								case 2: /* skip icon */
									rc = TRUE;
									break;

								case 0: /* cancel */
									rc = FALSE;
									break;

								#ifdef DEBUG
								default:
									PDB(("case not handled\n"));
									break;
								#endif
							}
							break;
					}
				}
			}
			name_delete(nn);
		}
	}
	/* XXX */

done:
	ITERATELISTSAFE(sn, nextsn, &ds.skiplist)
	{
		free(sn);
	}

	return (rc);
}
