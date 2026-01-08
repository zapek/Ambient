#include "trashcan.h"
#include "ambient.h"
#include <proto/dos.h>
#include <dos/dosextens.h>
#include "file_func.h"
#include "methodstack.h"
#include "mui_func.h"
#include "name.h"
#include "notify.h"
#include "ambient_cat.h"
#include "deletefile.h"
#include "fileaction.h"
#include "mui_func.h"
#include "smartreq.h"
#include "dosnotify.h"
#include "appclass.h"
#include "threads.h"

#ifndef ACTION_TRASH_FILE
#define ACTION_TRASH_FILE         26600 /* arg1 - BPTR lock to a file/directory to move to Trashcan: */
#define ACTION_RESTORE_FILE       26601 /* arg1 - STRPTR name of a file/directory *in* Trashcan: to be restored */
#define ACTION_RESTORE_PATH       26602 /* arg1 - STRPTR name of a file/directory *in* Trashcan:, arg2 - STRPTR dest_buf, arg3 - LONG buffer_size */
#define ACTION_EMPTY_TRASHCAN     26603
#define ACTION_IS_TRASHCAN_FS     26604
#endif

struct MsgPort *trashcan_port; // Trashcan will not quit, once located!
APTR trashcan_icon_notify;
BOOL TrashFile(CONST_STRPTR path);
BOOL RestoreFile(CONST_STRPTR path);
ULONG trashfile( APTR obj UNUSED, APTR refwin UNUSED, CONST_STRPTR path, APTR progressobj, ULONG * filecount, BOOL noicon, ULONG *flags );
ULONG restorefile( APTR obj UNUSED, APTR refwin UNUSED, CONST_STRPTR path, APTR progressobj, ULONG * filecount, BOOL noicon, ULONG *flags );

ULONG trashcan_init(void)
{
	BPTR lock = Lock("Trashcan:", ACCESS_READ);
	
	if (lock)
	{
		struct FileLock *fl = BADDR(lock);
		
		if (DOSTRUE == DoPkt0(fl->fl_Task, ACTION_IS_TRASHCAN_FS))
		{
			trashcan_port = fl->fl_Task;
			trashcan_icon_notify = dosnotify_start(TRASHCAN_INFO, 0);

		}
		
		UnLock(lock);
	}
	
	return TRUE;
}

void trashcan_cleanup(void)
{
	if (trashcan_icon_notify)
		dosnotify_stop(trashcan_icon_notify);
}

BOOL is_trashcan(CONST_STRPTR path)
{
	if (trashcan_port)
		return Strnicmp(path, "trashcan:", 9) == 0;
	return FALSE;
}

void trashcan_updatediskinfo(void)
{
	notify_action("Trashcan:disk.info", NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Create);
}

BOOL trashcan_is_running(void)
{
	return trashcan_port != NULL;
}

void trashcan_empty(void)
{
	if (trashcan_port)
	{
		DoPkt0(trashcan_port, ACTION_EMPTY_TRASHCAN);
	}
}

BOOL TrashFile(CONST_STRPTR path)
{
	BOOL rc = FALSE;
	
	if (trashcan_port)
	{
		BPTR pathLock = Lock(path, ACCESS_READ);
		
		if (pathLock)
		{
			rc = DoPkt1(trashcan_port, ACTION_TRASH_FILE, pathLock);
		}

		if (pathLock) UnLock(pathLock);
	}
	
	return rc;
}

BOOL RestoreFile(CONST_STRPTR path)
{
	BOOL rc = FALSE;

	if (trashcan_port)
	{
		rc = DoPkt1(trashcan_port, ACTION_RESTORE_FILE, (LONG)path);
	}
	
	return rc;
}

ULONG trashfile( APTR obj UNUSED, APTR refwin UNUSED, CONST_STRPTR path, APTR progressobj, ULONG * filecount, BOOL noicon, ULONG *flags )
{
	ULONG rc = FALSE;
	LONG ioerr = ioerr;

	THREAD;
	ASSERT(path);

	flags = flags;
	
	if( progressobj ) {
		methodstack_push_sync( progressobj, 5, MM_Progresswin_Update, FilePart( path ), NULL, *filecount, NULL );
	}

restart_deletefile:

	/*
	 * Delete the file if it is there. If it isn't, it means
	 * it's an icon alone.
	 */
	if( TrashFile( path ) )
	{
		rc = TRUE;
		notify_action( path, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Delete );
	} else {
		ioerr = IoErr();

		if( ioerr == ERROR_OBJECT_NOT_FOUND )
		{
			rc = TRUE;
		}
	}

	if( rc )
	{
		STRPTR nn = nn; /* shut up gcc */

		if( noicon )
			goto done;

		if( ( nn = name_build_info( path ) ) )
		{
			/*
			 * Check if the icon is here.
			 */
			restart_deleteicon:

			{
				if( ( rc = TrashFile( nn ) ) )
				{
					rc = TRUE;
					notify_action( nn, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Delete );
				} else {
					ioerr = IoErr();

					switch( ioerr )
					{
						case ERROR_OBJECT_NOT_FOUND:
							rc = TRUE;
							break;

						default:
							switch( smartreq_request_sync( NULL, GSI( MSG_TRASHFILE_TITLE ), GSI( MSG_TRASHFILE_RETRYSKIPICONCANCEL ), MV_Notification_Error, GSI( MSG_TRASHFILE_ERRORWHILEDELETINGFILEICON ), path ) )
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
			name_delete( nn );
		}
	} else {
		switch( ioerr )
		{
			default:
				switch( smartreq_request_sync( NULL, GSI( MSG_TRASHFILE_TITLE ), GSI( MSG_TRASHFILE_RETRYCANCEL ), MV_Notification_Error, GSI( MSG_TRASHFILE_ERRORWHILEDELETINGFILE ), path ) )
				{
					case 1: /* retry */
						goto restart_deletefile;
						break;

					case 0: /* cancel */
						goto done;
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
	done:
	( *filecount )++;
	return( rc );
}

ULONG tr_trashall(APTR obj, APTR refwin, STRPTR *path, BOOL noicon)
{
	ULONG retval = FALSE;
	STRPTR firstpath;
	APTR progressobj;
	ULONG filecount = 0;

	THREAD;

	/* Supress DOS Notify for source and destination views */

	firstpath = *path;
	methodstack_push( app, 3, MM_Application_EnableDOSNotify, firstpath , FALSE );

	notify_action(firstpath, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_Enable, FALSE);

	progressobj = (APTR)methodstack_push_sync(app, 4, MM_Application_CreateProgresswin, MV_Progresswin_Look_Delete, (ULONG)FindTask(NULL), refwin);

	if(progressobj)
	{
		ULONG delete_flags = 0;

		do
		{
			retval = trashfile(obj, refwin, *path, progressobj, &filecount, noicon, &delete_flags);
			path++;
		}
		while (retval && *path);

		/* Enable DOS Notify for source and destination views */

		methodstack_push(progressobj, 5, MM_Progresswin_Update, NULL, NULL, filecount, NULL);
	}

	notify_action(firstpath, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_Enable, TRUE);

	methodstack_push_sync( app, 3, MM_Application_EnableDOSNotify, firstpath, TRUE );

	return retval;
}

ULONG restorefile( APTR obj UNUSED, APTR refwin UNUSED, CONST_STRPTR path, APTR progressobj, ULONG * filecount, BOOL noicon, ULONG *flags )
{
	ULONG rc = FALSE;
	LONG ioerr = ioerr;

	THREAD;
	ASSERT(path);

	flags = flags;
	
	if( progressobj ) {
		methodstack_push_sync( progressobj, 5, MM_Progresswin_Update, FilePart( path ), NULL, *filecount, NULL );
	}

restart_deletefile:

	/*
	 * Delete the file if it is there. If it isn't, it means
	 * it's an icon alone.
	 */
	if( RestoreFile( path ) )
	{
		rc = TRUE;
		notify_action( path, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Delete );
	} else {
		ioerr = IoErr();

		if( ioerr == ERROR_OBJECT_NOT_FOUND )
		{
			rc = TRUE;
		}
	}

	if( rc )
	{
		STRPTR nn = nn; /* shut up gcc */

		if( noicon )
			goto done;

		if( ( nn = name_build_info( path ) ) )
		{
			/*
			 * Check if the icon is here.
			 */
			restart_deleteicon:

			{
				if( ( rc = RestoreFile( nn ) ) )
				{
					rc = TRUE;
					notify_action( nn, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Delete );
				} else {
					ioerr = IoErr();

					switch( ioerr )
					{
						case ERROR_OBJECT_NOT_FOUND:
							rc = TRUE;
							break;

						default:
							switch( smartreq_request_sync( NULL, GSI( MSG_RESTOREFILE_TITLE ), GSI( MSG_RESTOREFILE_RETRYSKIPICONCANCEL ), MV_Notification_Error, GSI( MSG_RESTOREFILE_ERRORWHILEDELETINGFILEICON ), path ) )
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
			name_delete( nn );
		}
	} else {
		switch( ioerr )
		{
			default:
				switch( smartreq_request_sync( NULL, GSI( MSG_RESTOREFILE_TITLE ), GSI( MSG_RESTOREFILE_RETRYCANCEL ), MV_Notification_Error, GSI( MSG_RESTOREFILE_ERRORWHILEDELETINGFILE ), path ) )
				{
					case 1: /* retry */
						goto restart_deletefile;
						break;

					case 0: /* cancel */
						goto done;
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
	done:
	( *filecount )++;
	return( rc );
}

ULONG tr_restoreall(APTR obj, APTR refwin, STRPTR *path, BOOL noicon)
{
	ULONG retval = FALSE;
	STRPTR firstpath;
	APTR progressobj;
	ULONG filecount = 0;

	THREAD;

	/* Supress DOS Notify for source and destination views */

	firstpath = *path;
	methodstack_push( app, 3, MM_Application_EnableDOSNotify, firstpath , FALSE );

	notify_action(firstpath, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_Enable, FALSE);

	progressobj = (APTR)methodstack_push_sync(app, 4, MM_Application_CreateProgresswin, MV_Progresswin_Look_Delete, (ULONG)FindTask(NULL), refwin);

	if(progressobj)
	{
		ULONG delete_flags = 0;

		do
		{
			retval = restorefile(obj, refwin, *path, progressobj, &filecount, noicon, &delete_flags);
			path++;
		}
		while (retval && *path);

		/* Enable DOS Notify for source and destination views */

		methodstack_push(progressobj, 5, MM_Progresswin_Update, NULL, NULL, filecount, NULL);
	}

	notify_action(firstpath, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_Enable, TRUE);

	methodstack_push_sync( app, 3, MM_Application_EnableDOSNotify, firstpath, TRUE );

	return retval;
}
