/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2018 Ambient Open Source Team
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
 * $Id: movedir.c,v 1.19 2025/08/17 18:00:19 piru Exp $
 */

#include "ambient.h"

/* public */
#include <dos/dostags.h>
#include <proto/dos.h>
#include <proto/exec.h>

/* private */
#include "ambient_cat.h"
#include "movedir.h"
#include "movefile.h"
#include "fastcopy.h"
#include "recurse.h"
#include "name.h"
#include "methodstack.h"
#include "mui_func.h"
#include "file_func.h"
#include "notify.h"
#include "time_func.h"
#include "smartreq.h"
#include "dos_internal.h"
#include "rename.h"
#include "fileaction.h"
#include "threads.h"


struct copystuff {
	TEXT fulldir[PATH_SIZE]; /* used to speed up computation */
	ULONG len;               /* length of fulldir - 1 */
	ULONG offset;            /* offset into fulldir to append stuff */
	ULONG offset2;           /* offset pointing after the first directory */
	APTR progressobj;        /* object to send progress methods to */
	ULONG copy;              /* TRUE if we copy, FALSE if we move */
	ULONG time;
	ULONG count;
	QUAD  totaldone;
	ULONG aborted;
	ULONG *reqflags;
	ULONG recursionlevel;    /* recursion level is needed as first pass is a particular case when rename is needed */
	ULONG rename;            /* if a new name is given to root dir */
	TEXT  newname[PATH_SIZE];/* new name is stored here */
};


static ULONG movefunc(APTR obj UNUSED, CONST_STRPTR path, LONG type UNUSED, ULONG prot UNUSED, UQUAD size UNUSED, APTR userdata, CONST_STRPTR comment UNUSED)
{
	struct copystuff *cs = (struct copystuff *)userdata;
	ULONG rc = FALSE;

	cs->fulldir[cs->len] = '\0';

	if(cs->rename && cs->newname[0])
	{
		AddPart(cs->fulldir, cs->newname, sizeof(cs->fulldir));
		AddPart(cs->fulldir, path + cs->offset + cs->offset2, sizeof(cs->fulldir));
	}
	else
	{
		AddPart(cs->fulldir, path + cs->offset, sizeof(cs->fulldir));
	}

retry:

	rc = fastcopy(cs->progressobj, path, cs->fulldir, TRUE, &cs->time, cs->count, &(cs->totaldone), cs->reqflags);
	if ( (rc == FASTCOPY_OK) || (rc == FASTCOPY_NOSOURCE) ) /* XXX: fix blocks ? */
	{
		cs->count++;

		if(rc == FASTCOPY_OK)
		{
			notify_action(cs->fulldir, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Create);
			if (!cs->copy)
			{
				if (DeleteFile(path))
				{
					notify_action(path, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Delete);
					rc = TRUE;
				}
			}
			else
			{
				rc = TRUE;
			}
		}
		else
		{
			rc = TRUE;
		}
	}
	else
	{
		if(rc == FASTCOPY_ABORTED)
		{
			notify_action(cs->fulldir, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Create);
			cs->aborted = TRUE;
			rc = FALSE;
		}
		else
		{
			CONST_STRPTR buts = GSI( MSG_MOVEDIR_RETRYSKIPCANCEL);
			set( cs->progressobj, MUIA_Window_Sleep, TRUE );
			ULONG but = smartreq_request_sync(NULL, GSI(MSG_MOVEDIR_TITLE), buts, MV_Notification_Error, GSI(MSG_MOVEDIR_ERRORWHILEMOVING), path);
			set( cs->progressobj, MUIA_Window_Sleep, FALSE );
			switch(but)
			{
				case 1:  /* retry */
					rc = TRUE;
					goto retry;
				case 2:  /* skip */
					rc = TRUE;
					break;
				case 0:
					break;
			}
		}
	}

	return (rc);
} // movefunc()


static ULONG createdir(APTR obj UNUSED, CONST_STRPTR path, APTR userdata)
{
	LONG rc = TRUE, smart_rc;
	struct copystuff *cs = (struct copystuff *)userdata;

	cs->recursionlevel++;

	cs->fulldir[cs->len] = '\0';

	if(cs->rename && cs->newname[0])
	{
		AddPart(cs->fulldir, cs->newname, sizeof(cs->fulldir));
		if(cs->recursionlevel > 1)
		{
			AddPart(cs->fulldir, path + cs->offset + cs->offset2, sizeof(cs->fulldir));
		}
	}
	else
	{
		AddPart(cs->fulldir, path + cs->offset, sizeof(cs->fulldir));
	}

restart_createdir:

	if(exists(cs->fulldir) && !isdir(cs->fulldir))
	{
		set( cs->progressobj, MUIA_Window_Sleep, TRUE );
		smart_rc = smartreq_request_sync(NULL, GSI(MSG_MOVEDIR_TITLE), GSI(MSG_MOVEDIR_REPLACECANCEL), MV_Notification_Warning, GSI(MSG_MOVEDIR_ERRORISAFILE), cs->fulldir);
		set( cs->progressobj, MUIA_Window_Sleep, FALSE );
		if(smart_rc)
		{
			DeleteFile(cs->fulldir);
			goto restart_createdir;
		}
		else
		{
			rc = FALSE;
		}	 
	}

	if(rc == TRUE)
	{
		rc = makedir(cs->fulldir);

		if (rc)
		{
			notify_action(cs->fulldir, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Create);
			rc = TRUE;
		}
	}
	
	return (rc);
}

static const struct TagItem extags[] =
{
#ifdef EX64TAG_PosixDate
	{EX64TAG_PosixDate, TRUE},
#endif
	{TAG_DONE,}
};

static ULONG leavedir(APTR obj UNUSED, CONST_STRPTR path, APTR userdata)
{
	struct copystuff *cs = (struct copystuff *)userdata;
	BPTR l;

	if ((l = Lock(path, ACCESS_READ)))
	{
		LONG ex;
		D_S(struct FileInfoBlock, fib);

#ifdef fib_ActExtFlags
		fib->fib_ActExtFlags = 0;
#endif
		ex = Examine64(l, fib, LIB_MINVER(DOSBase, 51, 66) ? (struct TagItem *) extags : NULL);
		UnLock(l);

		if (ex)
		{
			cs->fulldir[cs->len] = '\0';

			if(cs->rename && cs->newname[0])
			{
				AddPart(cs->fulldir, cs->newname, sizeof(cs->fulldir));
				if(cs->recursionlevel > 1)
				{
					AddPart(cs->fulldir, path + cs->offset + cs->offset2, sizeof(cs->fulldir));
				}
			}
			else
			{
				AddPart(cs->fulldir, path + cs->offset, sizeof(cs->fulldir));
			}

			SetOwner(cs->fulldir, (fib->fib_OwnerUID << 16) | fib->fib_OwnerGID);
			notify_action(cs->fulldir, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_UID);
			notify_action(cs->fulldir, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_GID);

			if (fib->fib_Comment)
			{
				SetComment(cs->fulldir, fib->fib_Comment);
				notify_action(cs->fulldir, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Comment, fib->fib_Comment);
			}

			SetProtection(cs->fulldir, fib->fib_Protection & ~FIBF_ARCHIVE);
			notify_action(cs->fulldir, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Flags, fib->fib_Protection & ~FIBF_ARCHIVE );

#if defined(fib_ActExtFlags) && defined(FIBEXTF_POSIXDATE) && defined(SetFilePosixDate)
			if (fib->fib_ActExtFlags & FIBEXTF_POSIXDATE)
				SetFilePosixDate(cs->fulldir, &fib->fib_PosixDate, NULL);
			else
#endif
				SetFileDate(cs->fulldir, &fib->fib_Date);
			//notify_action(cs->fulldir, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Date, ...);
		}
	}
	/* XXX */

	if (!cs->copy)
	{
		if (*cs->reqflags != FILEACTION_REQUEST_CANCEL && DeleteFile(path))
		{
			notify_action(path, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Delete);
		}
	}

	cs->recursionlevel--;

	return (TRUE); /* XXX */
}


static ULONG renamedir(APTR obj UNUSED, CONST_STRPTR path, APTR userdata)
{
	struct copystuff *cs = (struct copystuff *)userdata;

	cs->fulldir[cs->len] = '\0';

	if(cs->rename && cs->newname[0])
	{
		AddPart(cs->fulldir, cs->newname, sizeof(cs->fulldir));
	}
	else
	{
		AddPart(cs->fulldir, path + cs->offset, sizeof(cs->fulldir));
	}

	switch(tr_rename(path, NULL, cs->fulldir, TRUE, FALSE, cs->reqflags))
	{
		case RENAME_OK:
			notify_action(path, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Delete);
			notify_action(cs->fulldir, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Create);
			return RENAME_OK;
		case RENAME_FAILED:
			return RENAME_FAILED;
		case RENAME_NOSOURCE:
			return RENAME_NOSOURCE;
		default:
		case RENAME_ABORTED:
			return RENAME_ABORTED;
	}
}



ULONG tr_movedir(APTR obj, APTR refwin, CONST_STRPTR from, CONST_STRPTR to, ULONG copy, APTR progressobj, ULONG * filecount, QUAD * totaldone, ULONG noicon, ULONG rename, ULONG *reqflags) /* XXX: we could pass a buffer for cs.buf for multimove :) */
{
	struct copystuff cs;
	STRPTR nn;
	ULONG rc = MOVE_FAILED;

	THREAD;
	ASSERT(from);
	ASSERT(to);

	if (isdir(from))
	{
		cs.offset = FilePart(from) - from;
		cs.offset2 = strlen(FilePart(from)) + 1;
		strcpy(cs.fulldir, to);
		cs.len = strlen(cs.fulldir);

		cs.time = timedm();
		cs.count = *filecount;
		cs.totaldone = *totaldone;
		cs.aborted = FALSE;

		D(COPY,bug("processing directory from <%s> to <%s>, copy: %s\n", from, to, copy ? "yes" : "no"));

			cs.progressobj = progressobj;
			cs.copy = copy;
			cs.reqflags = reqflags;
			cs.recursionlevel = 0;
			cs.rename = rename;
			cs.newname[0] = '\0';
			rc = MOVE_OK;

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
						stccpy(cs.newname, newname, sizeof(cs.newname));
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

			if (same_volume(from, to))
			{
				D(COPY,bug("same volume\n"));

				if (copy)
				{
					if (recurse(obj, from, "#?", createdir, leavedir, movefunc, (APTR)&cs))
					{
						*filecount = cs.count;
						*totaldone = cs.totaldone;

						if(cs.aborted)
						{
							rc = MOVE_ABORTED;
						}

						if ((rc == MOVE_OK) && !noicon && (nn = name_build_info(from))) /* XXX: beware.. a name must *NEVER* end with '/' there */
						{
							if(exists(nn))
							{
								cs.fulldir[cs.len] = '\0';

								if(rename && cs.newname[0])
								{
									TEXT dest[PATH_SIZE];
									STRPTR ptr = NULL;
									stccpy(dest, cs.fulldir, sizeof(dest));
									AddPart(dest, cs.newname, sizeof(dest));
									ptr = name_build_info(dest);

									if(ptr)
									{
										stccpy(cs.fulldir, ptr, sizeof(cs.fulldir));
										name_delete(ptr);
									}
								}
								else
								{
									AddPart(cs.fulldir, FilePart(nn), sizeof(cs.fulldir));
								}
									
								rc = fastcopy(cs.progressobj, nn, cs.fulldir, TRUE, &cs.time, cs.count, &(cs.totaldone), reqflags);

								if (rc != FASTCOPY_FAILED)
								{
									if (rc == FASTCOPY_OK)
									{
										(*filecount)++;
										notify_action(cs.fulldir, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Create);
									}
									/* an icon might not necessarily exist */
									rc = MOVE_OK;
								}
								else
								{
									rc = MOVE_FAILED;
								}
							}
							name_delete(nn);
						}

						/* XXX */
					}
					/* XXX */
				}
				else
				{
					TEXT dest[PATH_SIZE];

                    stccpy(dest, to, sizeof(dest));

					if(rename && cs.newname[0])
					{
						AddPart(dest, cs.newname, sizeof(dest));
					}
					else
					{
						AddPart(dest, FilePart(from), sizeof(dest));
					}

					/* in this case, we can't just rename, so a full move is needed */
					if(exists(dest) && isdir(dest))
					{
						if (recurse(obj, from, "#?", createdir, leavedir, movefunc, (APTR)&cs))
						{
							*filecount = cs.count;

							if(cs.aborted)
							{
								rc = MOVE_ABORTED;
							}

							if((rc == MOVE_OK) && *cs.reqflags != FILEACTION_REQUEST_CANCEL && DeleteFile(from))
							{
								notify_action(from, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Delete);
							}

							if ((rc == MOVE_OK) && !noicon && (nn = name_build_info(from))) /* XXX: beware.. a name must *NEVER* end with '/' there */
							{
								if(exists(nn))
								{
									cs.fulldir[cs.len] = '\0';

									if(rename && cs.newname[0])
									{
										TEXT dest[PATH_SIZE];
										STRPTR ptr = NULL;
										stccpy(dest, cs.fulldir, sizeof(dest));
										AddPart(dest, cs.newname, sizeof(dest));

										ptr = name_build_info(dest);

										if(ptr)
										{
											stccpy(cs.fulldir, ptr, sizeof(cs.fulldir));
											name_delete(ptr);
										}
									}
									else
									{
										AddPart(cs.fulldir, FilePart(nn), sizeof(cs.fulldir));
									}

									rc = fastcopy(cs.progressobj, nn, cs.fulldir, TRUE, &cs.time, cs.count, &(cs.totaldone), reqflags);

									if (rc != FASTCOPY_FAILED)
									{
										if (rc == FASTCOPY_OK)
										{
											(*filecount)++;
											notify_action(cs.fulldir, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Create);
										}
										/* an icon might not necessarily exist */
										rc = MOVE_OK;
									}
									else
									{
										rc = MOVE_FAILED;
									}

									if (rc == MOVE_OK && *cs.reqflags != FILEACTION_REQUEST_CANCEL)
									{
										if (DeleteFile(nn))
										{
											notify_action(nn, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Delete);
											rc = MOVE_OK;
										}
									}
								}
								name_delete(nn);
							}

							/* XXX */
						}
						/* XXX */
					}
					else
					{
						switch(renamedir(obj, from, (APTR)&cs))
						{
							case RENAME_OK:
								rc = MOVE_OK;
								if (!noicon && (nn = name_build_info(from))) /* XXX: beware.. a name must *NEVER* end with '/' there */
								{
									if(exists(nn))
									{
										cs.fulldir[cs.len] = '\0';

										if(rename && cs.newname[0])
										{
											TEXT dest[PATH_SIZE];
											STRPTR ptr = NULL;
											stccpy(dest, cs.fulldir, sizeof(dest));
											AddPart(dest, cs.newname, sizeof(dest));
											ptr = name_build_info(dest);

											if(ptr)
											{
												stccpy(cs.fulldir, ptr, sizeof(cs.fulldir));
												name_delete(ptr);
											}
										}
										else
										{
											AddPart(cs.fulldir, FilePart(nn), sizeof(cs.fulldir));
										}

										switch(tr_rename(nn, NULL, cs.fulldir, TRUE, FALSE, reqflags))
										{
											case RENAME_OK:
												notify_action(nn, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Delete);
												notify_action(cs.fulldir, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Create);
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
								break;
						}
					}
				}
			}
			else
			{
				D(COPY,bug("different volume.. recursing..\n"));

				if (recurse(obj, from, "#?", createdir, leavedir, movefunc, (APTR)&cs))
				{
					D(COPY,bug("recursed properly.. copying .info if any\n"));

					*filecount = cs.count;
					*totaldone = cs.totaldone;

					if(cs.aborted)
					{
						rc = MOVE_ABORTED;
					}

					if(!copy)
					{
						if((rc == MOVE_OK) && *cs.reqflags != FILEACTION_REQUEST_CANCEL && DeleteFile(from))
						{
							notify_action(from, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Delete);
						}
					}

					if ((rc == MOVE_OK) && !noicon && (nn = name_build_info(from))) /* XXX: beware.. a name must *NEVER* end with '/' there */
					{
						if(exists(nn))
						{
							cs.fulldir[cs.len] = '\0';

							if(rename && cs.newname[0])
							{
								TEXT dest[PATH_SIZE];
								STRPTR ptr = NULL;
								stccpy(dest, cs.fulldir, sizeof(dest));
								AddPart(dest, cs.newname, sizeof(dest));
								ptr = name_build_info(dest);

								if(ptr)
								{
									stccpy(cs.fulldir, ptr, sizeof(cs.fulldir));
									name_delete(ptr);
								}
							}
							else
							{
								AddPart(cs.fulldir, FilePart(nn), sizeof(cs.fulldir));
							}
								
							rc = fastcopy(cs.progressobj, nn, cs.fulldir, TRUE, &cs.time, cs.count, &(cs.totaldone), reqflags);
								
							if (rc != FASTCOPY_FAILED)
							{
								if (rc == FASTCOPY_OK)
								{
									(*filecount)++;
									notify_action(cs.fulldir, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Create);
								}
								rc = MOVE_OK;
							}
							else
							{
								rc = MOVE_FAILED;
							}
								
							if (!copy && (rc == MOVE_OK) && *cs.reqflags != FILEACTION_REQUEST_CANCEL)
							{
								if (DeleteFile(nn))
								{
									notify_action(nn, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Delete);
									rc = MOVE_OK;
								}
							}
						}
						name_delete(nn);
					}
					/* XXX */
				}
				/* XXX */
		} /* XXX */
	}
	else
	{
		rc = tr_movefile(obj, refwin, from, to, copy, progressobj, filecount, totaldone, noicon, rename, reqflags);
	}

	return (rc);
}
