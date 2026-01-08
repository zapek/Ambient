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
 * $Id: fastcopy.c,v 1.21 2019/09/22 17:22:41 bitrocky Exp $
 */

#include "ambient.h"

/* public */
#include <dos/dos.h>
#include <libraries/asyncio.h>
#include <proto/asyncio.h>
#include <proto/exec.h>
#include <proto/dos.h>

/* private */
#include "fastcopy.h"
#include "fileaction.h"
#include "methodstack.h"
#include "classes.h"
#include "prefs.h"
#include "file_func.h"
#include "threads.h"
#include "time_func.h"
#include "smartreq.h"

/************************************************************************/

#define COPY_CANCEL  0
#define COPY_PROCEED 1
#define COPY_SKIP    2

/* Some tweaks, tests and results
 *
 * Read ahead: 64 kB -> slower USB/netdrive speed
 * Write buffer: 8 MB
 */
//#define COPYIO_COPYSIZE (64 * 1024)
#define COPYIO_COPYSIZE COPYIO_READAHEAD_BUFFERSIZE

/************************************************************************/

STATIC size_t getblocksize(CONST_STRPTR from, CONST_STRPTR to)
{
#if COPYIO_USE_ASYNCIO
	return COPYIO_COPYSIZE;
#else
	return MAX(MAX(get_blocksize(from), get_blocksize(to) ), 512);
#endif
}

#if !COPYIO_USE_ASYNCIO
STATIC APTR fastcopy_allocbuf( CONST_STRPTR from, CONST_STRPTR to, ULONG *startsize )
{
	ULONG size;

	THREAD;

	size = max( max( get_blocksize( from ), get_blocksize( to ) ), 512 );

	if( startsize )
	{
		*startsize = size;
	}

	#if USE_COPYIOPREFS
	switch( _conf( copy_mode ) )
	{
		case CM_FIXED:
			size = max( size, _conf( copy_buffersize ) );
			break;

		case CM_BLOCK:
			break;

		case CM_INCREMENTAL:
			size = max( size, _conf( copy_buffersize ) );
			break;
	}
	#else
	size = max( size, COPYIO_BUFFERSIZE );
	#endif
	D(COPY,bug("allocated buffer, size: %ld, startsize: %ld\n", size, startsize ? *startsize : 0));
	return( malloc ( size ) );
}

/************************************************************************/

static void fastcopy_freebuf( APTR buf )
{
	ASSERT(buf);

	free( buf );
}
#endif

/************************************************************************/

static ULONG get_copybufsize( ULONG *ctx_bufsize )
{
#if COPYIO_USE_ASYNCIO
	return COPYIO_COPYSIZE;
#else
	#if USE_COPYIOPREFS
	switch( _conf( copy_mode ) )
	{
		case CM_FIXED:
			*ctx_bufsize = _conf( copy_buffersize );
			return( *ctx_bufsize );

		case CM_BLOCK:
			return( *ctx_bufsize );

		case CM_INCREMENTAL:
			if( *ctx_bufsize < _conf( copy_buffersize ) )
			{
				*ctx_bufsize <<= 1;
				D(COPY,bug("buffer incremented to %ld\n", *ctx_bufsize));
			}
			return( *ctx_bufsize );
		
		#ifdef DEBUG
		default:
			PDB(("NYI!\n"));
			break;
		#endif
	}
	return( 0 );
	#else
	if( *ctx_bufsize < COPYIO_BUFFERSIZE )
	{
		*ctx_bufsize <<= 1;
		D(COPY,bug("buffer incremented to %ld\n", *ctx_bufsize));
	}
	return( *ctx_bufsize );
	#endif
#endif
}

/************************************************************************/

#if COPYIO_USE_ASYNCIO
struct fastcopydata
{
	APTR dest;
	LONG totaldone;
	LONG aborted;
};

STATIC ULONG fastcopy_cb(struct Hook *hook, APTR dummy, AsyncReadHookMsg *msg)
{
	if (msg->BytesInBuffer > 0)
	{
		struct fastcopydata *fcd = hook->h_Data;
		LONG aborted = threads_check_abort();

		if (!aborted)
		{
			fcd->totaldone += WriteAsync( fcd->dest, msg->Buffer, msg->BytesInBuffer );
			aborted = threads_check_abort();
		}

		if (aborted)
		{
			fcd->aborted = TRUE;
			SetIoErr(ERROR_BREAK);
			return FALSE;
		}
	}

	return TRUE;
}
#endif

ULONG fastcopy( APTR obj, CONST_STRPTR from, CONST_STRPTR to, ULONG clone, ULONG *time, ULONG cnt, QUAD * totaldone, ULONG *reqflags )
{
#if COPYIO_USE_ASYNCIO
	struct AsyncFile *af1, *af2;
#else
	BPTR f1, f2;
	ULONG own_alloc, blocksize;
#endif
	LONG got, got2;
	ULONG error = FALSE;
	ULONG retval = FASTCOPY_FAILED;
	ULONG aborted = FALSE;
	ULONG fileincomplete = FALSE;

	THREAD;
	ASSERT(from);
	ASSERT(to);
	
	#ifdef DEBUG
	if( obj )
	{
		CHECKOBJECT( obj );
	}
	#endif

	D(COPY,bug("copying from <%s> to <%s>, clone: %ld\n", from, to, clone));

	if( *reqflags & FILEACTION_REQUEST_CANCEL )
	{
		return( FASTCOPY_NOSOURCE );
	}

	if( isdir( to ) )
	{
		smartreq_request_sync( NULL, "Ambient � Copy", "*_Ok", MV_Notification_Warning, "Destination '%s' is a directory.", to );
		return( FASTCOPY_NOSOURCE );
	}

	#if !COPYIO_USE_ASYNCIO
	if( buf )
	{
		own_alloc = FALSE;
	} else {
		buf = fastcopy_allocbuf( from, to, &blocksize );
		own_alloc = TRUE;
	}

	if( buf )
	#endif
	{
		D(COPY,bug("buffer allocated\n"));
		#if COPYIO_USE_ASYNCIO
		#if USE_COPYIOPREFS
		if ( ( af1 = OpenAsync( (STRPTR) from, MODE_READ, _conf( copy_readahead_buffersize ) ) ) )
		#else
		if ( ( af1 = OpenAsync( (STRPTR) from, MODE_READ, COPYIO_COPYSIZE ) ) )
		#endif
		#else
		if ( ( f1 = Open( from, MODE_OLDFILE ) ) )
		#endif
		{
			#if !COPYIO_USE_ASYNCIO
			LONG accessMode = MODE_NEWFILE;
			#endif
			ULONG res = COPY_PROCEED;

			if( exists( to ) )
			{
				ULONG mask = 0;
				ULONG deleteprotected = 0;
				ULONG writeprotected  = 0;

				#if !COPYIO_USE_ASYNCIO
				accessMode = MODE_OLDFILE;
				#endif

				/* XXX : as in dopus, check if a both files have a version and compare them (date, md5sum ) */
				switch( fileaction_replace( from, to, FILEACTION_REPLACE_COPY, reqflags ) )
				{
					case FILEACTION_PROCEED:
						res = COPY_PROCEED;
						break;
					case FILEACTION_SKIP:
						res = COPY_SKIP;
						break;
					case FILEACTION_CANCEL:
						res = COPY_SKIP;
						break;
				}

				if( res == COPY_PROCEED && dosprotection_get( to, &mask ) )
				{
					deleteprotected = ( ( mask & PROTF_DELETE ) == 0 );
					writeprotected  = ( ( mask & PROTF_WRITE  ) == 0 );

					if( deleteprotected || writeprotected )
					{
						switch( fileaction_unprotect( to, FILEACTION_UNPROTECT_FROM_OVERWRITE, reqflags ) )
						{
							case FILEACTION_PROCEED:
								res = COPY_PROCEED;
								break;

							case FILEACTION_SKIP:
								res = COPY_SKIP;
								break;

							case FILEACTION_CANCEL:
								res = COPY_SKIP;
								break;
						}
					}
				}

				if( res == COPY_PROCEED )
				{
					if( !DeleteFile( to ) )
					{
						/* may be annoying, but user should really have a feedback */
						smartreq_request_sync( NULL, "Ambient � Copy", "*_Ok", MV_Notification_Error, "File '%s' can't be deleted.\nDos error: %S (#%N).", to );
						
						if( deleteprotected )
						{
							dosprotection_clear( to, PROTF_DELETE );
						}

						if( writeprotected )
						{
							dosprotection_clear( to, PROTF_WRITE );
						}

					    retval = FASTCOPY_FAILED;
						res = COPY_CANCEL;
					}
				}
			}

			if( res == COPY_SKIP )
			{
				retval = FASTCOPY_NOSOURCE; /* user chose to skip/cancel, it's not an error */
			}

			if( res == COPY_PROCEED )
			{
				struct FileInfoBlock fib;

				#if COPYIO_USE_ASYNCIO
				if (ExamineFH64(af1->af_File, &fib, NULL))
				#else
				if (ExamineFH64(f1, &fib, NULL))
				#endif
				{
					D(COPY,bug("examinefh64()d ok -> %lld\n", fib.fib_Size64 ) );

					#if COPYIO_USE_ASYNCIO
					#if USE_COPYIOPREFS
					if ( ( af2 = OpenAsync( (STRPTR) to, MODE_WRITE, _conf( copy_readahead_buffersize ) ) ) )
					#else
					if ( ( af2 = OpenAsync( (STRPTR) to, MODE_WRITE, MIN(COPYIO_READAHEAD_BUFFERSIZE, fib.fib_Size64) ) ) )
					#endif
					#else
					if ( ( f2 = Open( to, accessMode ) ) )
					#endif
					{
						#if COPYIO_USE_ASYNCIO
						struct fastcopydata fcd = { af2, 0, FALSE };
						struct Hook hook;
						#endif

						UQUAD progress = 0;
						ULONG ctx_bufsize = getblocksize(from, to); //blocksize;
						ULONG r2;
						ULONG has_filename = FALSE;

						D(COPY,bug("files opened ok\n"));
						if( obj )
						{
							r2 = timedm();

							if( !time || ( r2 - *time > PROGRESS_REFRESH_MS ) )
							{
								methodstack_push_sync(obj, 5, MM_Progresswin_Update, FilePart( from ), &fib.fib_Size64, cnt, totaldone );
								if( time )
								{
									*time = r2;
								}
								has_filename = TRUE;
							}
						}
						#if COPYIO_USE_ASYNCIO
						hook.h_Entry = (HOOKFUNC)HookEntry;
						hook.h_SubEntry = (HOOKFUNC)fastcopy_cb;
						hook.h_Data = &fcd;

						//while ( ( got = ReadAsync( af1, buf, get_copybufsize( &ctx_bufsize ) ) ) ) /* XXX: we don't know if we fail to read */
						while ( ( got = ReadAsyncPkt( af1, &hook, get_copybufsize( &ctx_bufsize ) ) )) /* XXX: we don't know if we fail to read */
						#else
						while ( got = Read( f1, buf, get_copybufsize( &ctx_bufsize ) ) )
						#endif
						{
							D(COPY,bug("read/write size: %ld\n", got));
							#if !COPYIO_USE_ASYNCIO
							if( threads_check_abort() )
							{
								fileincomplete = TRUE;
								error = TRUE; /* not really an error though */
								aborted = TRUE;
								break;
							}
							#endif

							if (got < 0)
							{
								//dprintf("read error\n");
								D(COPY,bug("read error\n"));
								fileincomplete = TRUE;
								error = TRUE;
								break;
							}

							#if COPYIO_USE_ASYNCIO
							//got2 = WriteAsync( af2, buf, got );
							got2 = fcd.totaldone;
							#else
							got2 = Write( f2, buf, got );
							#endif

							if( totaldone )
							{
								( *totaldone ) += got2;
							}

							if( got2 != got )
							{
								//dprintf("write error\n");
								D(COPY,bug("write error (%ld/%ld)\n", got, got2));
								fileincomplete = TRUE;
								error = TRUE;
								break; /* eek */
							}

							#if !COPYIO_USE_ASYNCIO
							/* Check again after write before next read */
							if( threads_check_abort() )
							{
								fileincomplete = TRUE;
								error = TRUE; /* not really an error though */
								aborted = TRUE;
								break;
							}
							#else
							fcd.totaldone = 0;
							#endif

							if( obj )
							{
								r2 = timedm();
								progress += got;

								if( !time || ( r2 - *time > PROGRESS_REFRESH_MS ) )
								{
									if( !has_filename )
									{
										methodstack_push( obj, 5, MM_Progresswin_Update, FilePart( from ), &fib.fib_Size64, cnt, totaldone );

										has_filename = TRUE;
									}

									methodstack_push_sync( obj, 5, MM_Progresswin_Update, NULL, &progress, cnt, totaldone );
									progress = 0;

									if( time )
									{
										*time = r2;
									}
								}
							}

							if( ( got < ctx_bufsize ) )
							{
								break;
							}
						}

						#if COPYIO_USE_ASYNCIO
						if (fcd.aborted)
						{
							fileincomplete = TRUE;
							error = TRUE; /* not really an error though */
							aborted = TRUE;
						}
						#endif
					}
					else{
						smartreq_request_sync( NULL, "Ambient � Copy", "*_Ok", MV_Notification_Error, "Failed to open file '%s'.\nDos error: %S (#%N).", to );
					    retval = FASTCOPY_FAILED;
						res = COPY_CANCEL;
						error = TRUE;
						aborted = TRUE;
					}

					if( !error )
					{
						retval = FASTCOPY_OK;
					}
					#if COPYIO_USE_ASYNCIO
					CloseAsync( af2 );
					#else
					Close( f2 );
					#endif

					if( retval == FASTCOPY_OK )
					{
						if( !SetOwner( to, ( fib.fib_OwnerUID << 16 ) | fib.fib_OwnerGID ) )
						{
							//smartreq_doserror( "Ambient copy", "Couldn't set the owner of file %s", to );
							D(COPY, bug("setowner failed\n"));
							//retval = FASTCOPY_FAILED;
						}

						if( ( retval == FASTCOPY_OK ) && fib.fib_Comment )
						{
							if( !SetComment( to, fib.fib_Comment ) )
							{
								//smartreq_doserror( "Ambient copy", "Couldn't set the comment of file %s", to );
								D(COPY, bug("setcomment failed\n"));
								//retval = FASTCOPY_FAILED;
							}
						}

						if( clone )
						{
							if( retval == FASTCOPY_OK )
							{
								if( !SetFileDate( to, &fib.fib_Date ) )
								{
									//smartreq_doserror( "Ambient copy", "Couldn't set the date of file %s", to );
									D(COPY, bug("setfiledate failed\n"));
									//retval = FASTCOPY_FAILED;
								}
							}

							if( retval == FASTCOPY_OK )
							{
								if( !SetProtection( to, fib.fib_Protection ) )
								{
									//smartreq_doserror( "Ambient copy", "Couldn't set the protection mode of file %s", to );
									D(COPY, bug("setprotection failed\n"));
									//retval = FASTCOPY_FAILED;
								}
							}
						} else {
							if( !SetProtection( to, fib.fib_Protection & ~FIBF_ARCHIVE ) )
							{
								//smartreq_doserror( "Ambient copy", "Couldn't set the protection mode of file %s", to );
								D(COPY, bug("setprotection failed\n"));
								//retval = FASTCOPY_FAILED;
							}
						}
					} else {
						D(COPY, bug("Couldn't create destination file %s\n", to));
						//smartreq_doserror( "Ambient copy", "Couldn't create destination file %s", to );
					}
				} else {
					D(COPY, bug("Couldn't copy file %s\n", from));
					//smartreq_doserror( "Ambient copy", "Couldn't copy file %s", from );
				}
			}
			#if COPYIO_USE_ASYNCIO
			CloseAsync( af1 );
			#else
			Close( f1 );
			#endif
		} else {
			if( IoErr() == ERROR_OBJECT_NOT_FOUND )
			{
				retval = FASTCOPY_NOSOURCE;
			} else {
				retval = FASTCOPY_FAILED;
			}
		}

		#if !COPYIO_USE_ASYNCIO
		if( own_alloc )
		{
			fastcopy_freebuf( buf );
		}
		#endif
	}

	if( fileincomplete )
	{
		size_t rc;

		if (IoErr() == 0)
			rc = smartreq_request_sync(NULL, "Ambient � Copy", "*_Delete|Keep", MV_Notification_Error, "File '%s' is not complete.", to);
		else
			rc = smartreq_request_sync(NULL, "Ambient � Copy", "*_Delete|Keep", MV_Notification_Error, "File '%s' is not complete.\nDos error: %S (#%N).", to);

		if (rc)
		{
			dosprotection_set( to, PROTF_DELETE );
			DeleteFile( to );
		}
	}

	if( aborted )
	{
		D(COPY, bug("Aborting copy for file %s\n", from ) );
		retval = FASTCOPY_ABORTED;
	}

	return( retval );
}
