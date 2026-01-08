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
 * $Id: deletefile.c,v 1.11 2012/09/03 08:33:25 geit Exp $
 */

#include "ambient.h"

/* public */
#include <proto/dos.h>
#include <proto/exec.h>

/* private */
#include "ambient_cat.h"
#include "deletefile.h"
#include "fileaction.h"
#include "name.h"
#include "methodstack.h"
#include "mui_func.h"
#include "smartreq.h"
#include "notify.h"
#include "file_func.h"

/************************************************************************/

ULONG deletefile( APTR obj UNUSED, APTR refwin UNUSED, CONST_STRPTR path, APTR progressobj, ULONG * filecount, BOOL noicon, ULONG *flags )
{
	ULONG rc = FALSE;
	LONG ioerr = ioerr;

	THREAD;
	ASSERT(path);

	if( progressobj ) {
		methodstack_push_sync( progressobj, 5, MM_Progresswin_Update, FilePart( path ), NULL, *filecount, NULL );
	}

restart_deletefile:

	/*
	 * Delete the file if it is there. If it isn't, it means
	 * it's an icon alone.
	 */
	if( DeleteFile( path ) )
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
				if( ( rc = DeleteFile( nn ) ) )
				{
					rc = TRUE;
					notify_action( nn, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Delete );
				} else {
					ioerr = IoErr();

					switch( ioerr )
					{
						case ERROR_DELETE_PROTECTED:
							switch( fileaction_unprotect( nn, FILEACTION_UNPROTECT_FROM_DELETION, flags ) )
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

						case ERROR_OBJECT_NOT_FOUND:
							rc = TRUE;
							break;

						default:
							switch( smartreq_request_sync( NULL, GSI( MSG_DELETEFILE_TITLE ), GSI( MSG_DELETEFILE_RETRYSKIPICONCANCEL ), MV_Notification_Error, GSI( MSG_DELETEFILE_ERRORWHILEDELETINGFILEICON ), path ) )
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
			case ERROR_DELETE_PROTECTED:
				switch( fileaction_unprotect( path, FILEACTION_UNPROTECT_FROM_DELETION, flags ) )
				{
					case FILEACTION_CANCEL :
						goto done;

					case FILEACTION_PROCEED:
						goto restart_deletefile;

					case FILEACTION_SKIP   :
						rc = TRUE;
						return( rc );
				}
				break;

			default:
				switch( smartreq_request_sync( NULL, GSI( MSG_DELETEFILE_TITLE ), GSI( MSG_DELETEFILE_RETRYCANCEL ), MV_Notification_Error, GSI( MSG_DELETEFILE_ERRORWHILEDELETINGFILE ), path ) )
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
