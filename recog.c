/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006 Ambient Open Source Team
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
 * $Id: recog.c,v 1.13 2014/01/08 19:26:54 rzookol Exp $
 */

#include "ambient.h"

/* public */
#include <dos/dos.h>
#include <dos/doshunks.h>
#include <proto/dos.h>

/* private */
#include "recog.h"
#include "str.h"
#include "file_io.h"
#include "multimedia.h"
#include "datatypes.h"
#include "file_func.h"
#include "name.h"
#include "examine64.h"

/************************************************************************/

#define RECOGBUF_SIZE 128  /* should be enough for everyone (tm) */
#define CACHEBUFFER   32   /* can be max. 32 for now             */

#define IOBUFFERSIZE  2048 /* should be enough                   */

/************************************************************************/

struct recog_filectx {
	APTR   fh;
	ULONG  userdata;
	UQUAD  filebase; /* Only used by MatchHunk for now */
	struct fileinfo64 fi;

	UBYTE  buffer[RECOGBUF_SIZE];
	UBYTE  cache[RECOGBUF_SIZE * CACHEBUFFER];
	ULONG  cachestatus;
	UBYTE  contenttype;

	TEXT   name[0];
};

/************************************************************************/

enum {
	MODE_PLUS,
	MODE_MINUS
};

/************************************************************************/

#define SETFLAG(_x) \
	({if (mode == MODE_PLUS) \
		{ \
			flags |= (_x); \
			nonflags &= ~(_x); \
		} \
		else \
		{ \
			flags &= ~(_x); \
			nonflags |= (_x); \
		} \
	})

/************************************************************************/

static ULONG recogcmd_protection( struct recog_filectx *fct, CONST_STRPTR data )
{
	ULONG rc       = RECOG_FALSE;
	CONST_STRPTR p = data;
	ULONG flags    = 0;
	ULONG nonflags = 0;
	ULONG mode     = MODE_PLUS;
	ULONG prot;
	UBYTE c;

	/* string looks like: +rwed-a+s, rwed, -rwed, etc..
	 */
	while( ( c = *p ) )
	{
		switch( c )
		{
			case '+':
				mode = MODE_PLUS;
				break;

			case '-':
				mode = MODE_MINUS;
				break;

			case 's': /* script */
				SETFLAG( FIBF_SCRIPT );
				break;

			case 'a': /* archive */
				SETFLAG( FIBF_ARCHIVE );
				break;

			case 'p': /* pure */
				SETFLAG( FIBF_PURE );
				break;

			#if 0
			/* XXX: worth supporting? should also be in infowin then */
			case 'h': /* hold */
				SETFLAG( FIBF_HOLD );
				break;
			#endif

			case 'r': /* read */
				SETFLAG( FIBF_READ );
				break;

			case 'w': /* write */
				SETFLAG( FIBF_WRITE );
				break;

			case 'e': /* execute */
				SETFLAG( FIBF_EXECUTE );
				break;

			case 'd': /* deletable */
				SETFLAG( FIBF_DELETE );
				break;

			#ifdef DEBUG
			default:
				PDB(("unknown protection flag, wtf\n"));
				break;
			#endif
		}
		p++;
	}

	prot = fct->fi.fi_Protection ^ ( FIBF_READ | FIBF_WRITE | FIBF_DELETE | FIBF_EXECUTE ); /* dos must die */

	if( ( prot & flags ) && !( prot & nonflags ) )
	{
		rc = RECOG_TRUE;
	}

	return( rc );
}

/************************************************************************/

static ULONG recogcmd_findstring( struct recog_filectx *fct, CONST_STRPTR data )
{
	TEXT buf[RECOGBUF_SIZE];
	ULONG rc     = RECOG_FALSE;
	ULONG sl     = strlen(data);
	LONG  readin = 0;

	D(RECOG,bug("grep for %s (can take a while ;)\n", data));

	ASSERT( fct->fh )

	if( ( !data ) || (sl < 5 ) )  /* less would be silly imho, too much false positives (tokai) */
 	{
		return( RECOG_WRONGINPUT );
	}

	if( file_seek( fct->fh, 0, OFFSET_BEGINNING) == -1LL ) /* back at start */
	{
		return( RECOG_IOERROR );
	}

	while( ( readin = file_readmost( fct->fh, buf, RECOGBUF_SIZE ) ) >= 1 )
	{
		ULONG curoffs = 0;

		if( readin < sl )
		{
			break;
		}

		if( readin == sl )
		{
			if( strncmp( buf, data, sl ) == 0 )
			{
				rc = RECOG_TRUE;
			}
			break;
		}

		while( curoffs < ( readin - sl ) )
		{
			if( strncmp( buf + curoffs, data, sl ) == 0 )
			{
				rc = RECOG_TRUE;
				break;
			}
			curoffs++;
		}

		/*  check block border for partitial matches which possible could become
		 *  full matches and seek a bit back when required.
		 */
		if( readin == RECOGBUF_SIZE )
		{
			LONG i = sl - 1;
			UBYTE *t = buf+readin;

			while( i )
			{
				if (strncmp( t - i, data, i ) == 0 )
				{
					if( file_seek( fct->fh, -i, OFFSET_CURRENT ) == -1LL )
					{
						rc = RECOG_IOERROR;
					}
					break;
				}

				i--;
			}
		}

		/* we got a match (positive or negative) already,
		 * so quit scanning
		 */
		if( rc != RECOG_FALSE )
		{
			PDB(("leave loop\n"));
			break;
		}
	}

	if( readin == (-1) )
	{
		rc = RECOG_IOERROR;
	}
	return (rc);
}

/************************************************************************/

static ULONG recogcmd_datatypes( struct recog_filectx *fct, CONST_STRPTR data UNUSED )
{
	if ( (fct->userdata = datatypes_findtype( fct->name ) ) )
	{
		return( RECOG_TRUE );
	}
	return( RECOG_FALSE );
}

/************************************************************************/

#if USE_MULTIMEDIA
static ULONG recogcmd_multimedia( struct recog_filectx *fct, CONST_STRPTR data UNUSED )
{
	if ( ( fct->userdata = multimedia_findtype( fct->name ) ) )
	{
		return ( RECOG_TRUE );
	}
	return( RECOG_FALSE );
}
#endif

/************************************************************************/

static ULONG recogcmd_filename( struct recog_filectx *fct, CONST_STRPTR data UNUSED )
{
	ULONG rc = RECOG_FALSE;

	if( data && *data )
	{
		if( name_match( FilePart( fct->name ), data ) )
		{
			rc = RECOG_TRUE;
		}
	}
	return( rc );
}

/************************************************************************/

static ULONG recogcmd_isdevice( struct recog_filectx *fct, CONST_STRPTR data UNUSED )
{
	ULONG rc = RECOG_FALSE;

	if( isdevicename( fct->name ) )
	{
		rc = RECOG_TRUE;
	}

	return( rc );
}

/************************************************************************/

static ULONG recogcmd_isdirectory( struct recog_filectx *fct, CONST_STRPTR data UNUSED )
{
	ULONG rc = RECOG_FALSE;

	if( fct->fi.fi_Type > 0 )
	{
		rc = RECOG_TRUE;
	}
	return( rc );
}

/************************************************************************/

static ULONG recogcmd_isfile( struct recog_filectx *fct, CONST_STRPTR data UNUSED )
{
	ULONG rc = RECOG_FALSE;

	if( fct->fi.fi_Type <= 0 )
	{
		rc = RECOG_TRUE;
	}

	return( rc );
}

/************************************************************************/

static ULONG recogcmd_comment( struct recog_filectx *fct, CONST_STRPTR data )
{
	if( data && *data )
	{
		if ( name_match( fct->fi.fi_Comment, data ) )
		{
			return( RECOG_TRUE );
		}

		return( RECOG_FALSE );
	}

	return( RECOG_WRONGINPUT );
}

/************************************************************************/

static ULONG recogcmd_filesize( struct recog_filectx *fct, CONST_STRPTR data )
{
	ULONG rc = RECOG_FALSE;
	ULONG l;
	UQUAD matchsize;
	BOOL  matchmodulo = FALSE;

	ASSERT( fct );
	ASSERT( data );

	if( !( l = strlen( data ) ) )
	{
		return( rc );
	}

	if (*data == '%')
	{
		if( l < 2 )
		{
			return( rc );
		}

		matchmodulo = TRUE;
		data++;
	}

	matchsize = strtoull( data, NULL, 0 );

	D(RECOG, bug("\t%s: %llu\n", (matchmodulo) ? "modulo": "filesize", matchsize));

	if( matchmodulo ? !(fct->fi.fi_Size % matchsize) : ( matchsize == fct->fi.fi_Size ) )
	{
		rc = RECOG_TRUE;
	}

	return( rc );
}

/************************************************************************/

/*
 *   file caching routine
 *
 */

#define BLOCKISEMPTY(x,y)  (!((x) & (1 << (y))))
#define BLOCKOK(x,y)       ((x) |= (1 << (y)))

static ULONG fillcache( struct recog_filectx *fct, UQUAD startoffset, ULONG length, CONST_APTR *buffer )
{
	UQUAD filesize    = fct->fi.fi_Size;
	UQUAD endoffset;

	if( length == 0 )
	{
		return( RECOG_TRUE );
	}

	startoffset += fct->filebase;

	endoffset = startoffset + length;

	D(RECOG,bug("\tlength: %ld  endoffset: %llu\n", length, endoffset));


	/*  Bail out if matching is not possible anyway
	 *
	 *  If compiled with asyncio seeking failed with old recogcmd_match() (which did
	 *  a lot seeking around (even behind EOF) etc.). This only happened with very
	 *  small files for an unknown reason at some point in the scan process. This
	 *  might be a bug in asyncio.library, but I couldn't reproduce it with isolated
	 *  code in a test program. So we reduce fileio now to a minimum which avoid this
	 *  problem (hopefully) completely. (tokai)
	 */
	if( endoffset > filesize )
	{
		return( RECOG_FALSE );
	}


	/*  We can cache the first 32 blocks with a size of RECOGBUF_SIZE each, this is
	 *  usually enough, because magic numbers are placed at start of files (except a
	 *  few exceptions).
	 *
	 *  If the matching happens outside of our cache area we simply use the normal
	 *  (slow) scan mode, else we load the block (if not already present) to the
	 *  cache.
	 *
	 *  For simplicity we assume a match string is never bigger than RECOGBUF_SIZE.
	 */
	if( length > RECOGBUF_SIZE )
	{
		return( RECOG_WRONGINPUT );
	}

	if( endoffset > ( RECOGBUF_SIZE * CACHEBUFFER ) )
	{
		/*   XXX: tokai needs to add 'last block caching' here.
		 */

		D(RECOG,bug("\tslow mode\n"));

		if( file_seek( fct->fh, startoffset, OFFSET_BEGINNING) == -1LL )
		{
			D(RECOG,bug("\nseeking failed (requested offset: %llu, filehandle: 0x%lx)\n", startoffset, fct->fh));
			return( RECOG_IOERROR );
		}

		if( !file_read( fct->fh, fct->buffer, length ) )
		{
			D(RECOG,bug("file_read() failed\n"));
			return( RECOG_IOERROR );
		}

		*buffer = fct->buffer;
	} else {
		UQUAD d1, d2, lastblock;
		ULONG mbytes;
		BOOL seek2 = TRUE;

		D(RECOG,bug("\tcache mode\n"));

		/*  We need to find out which cache blocks are affected. Because we limit the
		 *  length max. 2 blocks can be affected
		 */

		d1 = startoffset / RECOGBUF_SIZE;
		d2 = (endoffset - 1) / RECOGBUF_SIZE;  /* endoffset can't be 0 here */

		ASSERT( d1 < CACHEBUFFER );
		ASSERT( d2 < CACHEBUFFER );

		if( filesize <= (RECOGBUF_SIZE * CACHEBUFFER ) )
		{
			lastblock = (filesize - 1) / RECOGBUF_SIZE;  /* filesize can't be 0 here */
			mbytes    = ((ULONG)filesize) % RECOGBUF_SIZE;
		} else {
			lastblock = 31;
			mbytes    = RECOGBUF_SIZE;
		}

		D(RECOG,bug("deltas: %llu %llu (last possible block: %llu, mb: %lu, filesize %llu)\n", d1, d2, lastblock, mbytes, filesize));


		if( BLOCKISEMPTY( fct->cachestatus, d1 ) )
		{
			UQUAD pos = d1 * RECOGBUF_SIZE;
			ULONG len = ( d1 == lastblock ) ? mbytes : RECOGBUF_SIZE;

			D(RECOG,bug("\t\tblock 1\n"));

			if( file_seek( fct->fh, pos, OFFSET_BEGINNING) == -1LL )
			{
				D(RECOG,bug("\nseeking failed (requested offset: %llu, filehandle: 0x%lx)\n", pos, fct->fh));
				return( RECOG_IOERROR );
			}

			if( !file_read(fct->fh, fct->cache + pos, len))
			{
				D(RECOG,bug("file_read() failed.\n"));
				return( RECOG_IOERROR );
			}

			BLOCKOK( fct->cachestatus, d1 );

			seek2 = FALSE; /* no seek required in case we have to read 2nd block too */
		}


		if( ( d1 != d2 ) && BLOCKISEMPTY( fct->cachestatus, d2 ) )
		{
			UQUAD pos = d2 * RECOGBUF_SIZE;
			ULONG len = ( d2 == lastblock ) ? mbytes : RECOGBUF_SIZE;

			D(RECOG,bug("\t\tblock 2\n"));

			if( seek2 && ( file_seek( fct->fh, pos, OFFSET_BEGINNING ) == -1LL ) )
			{
				D(RECOG,bug("\nseeking failed (requested offset: %llu, filehandle: 0x%lx)\n", pos, fct->fh));
				return( RECOG_IOERROR );
			}

			if (!file_read( fct->fh, fct->cache + pos, len ) )
			{
				D(RECOG,bug("file_read() failed.\n"));
				return( RECOG_IOERROR );
			}

			BLOCKOK( fct->cachestatus, d2 );
		}

		*buffer = fct->cache + startoffset;

		#if 0
		#ifdef DEBUG
		{ UBYTE i; KPutStr("CacheStatus: "); for (i=0; i < CACHEBUFFER; i++) { KPutChar((fct->cachestatus & (1<<i)) ? '1' : '0'); } KPutStr("\n"); }
		#endif
		#endif
	}

	return( RECOG_TRUE );
}

/************************************************************************/

/*
 *   a match string can look like:
 *
 *       sFOOBAR or SFOOBAR   - case sensitive string matching
 *       iFOOBAR or IFOOBAR   - case insensitive string matchin
 *         - strings can contain '?' as placeholder, e.g. F?OBAR or FOO???
 *
 *       $HEX                 - exact hex matching
 *         - hex can contain '??' as placeholder, e.g. $AB??EF (2 chars = 1 byte!)
 *
 *       additionally an offset (decimal) can be specified:
 *       12345,sFOOBAR or 12345,$HEX, 20,$HEX etc.
 *
 *       If a minus sign is found in front of the offset, then the match string is
 *       checked at 'filesize - offset' position, e.g. -20,sFOOBAR, etc.
 */

#define MATCHMODE_HEX             '$'
#define MATCHMODE_CASESENSITIVE   's'
#define MATCHMODE_CASEINSENSITIVE 'i'

#define DOFILL(o,l,p) do { LONG __rc = fillcache(fct, o, l, (CONST_APTR *)(p)); if (__rc != RECOG_TRUE) return __rc; } while(0)

static ULONG recogcmd_match( struct recog_filectx *fct, CONST_STRPTR data )
{
	UQUAD startoffset = 0;
	//UQUAD endoffset   = 0;
	ULONG length      = 0;
	UQUAD filesize    = fct->fi.fi_Size;
	BOOL  negative    = FALSE;

	CONST_STRPTR s    = data;
	STRPTR buffer;
	UBYTE  c;
	UBYTE  mode;


	ASSERT( fct );
	ASSERT( fct->fh );
	ASSERT( data );

	D(RECOG,bug("match: %s\n", data));

	/*  Check if the match string has an offset: "offset,matchstring"
	 */
	{
		c = *s;

		if( c == '-' )
		{
			negative = TRUE;

			s++;

			if( !(c = *s ) )
			{
				return( RECOG_WRONGINPUT );
			}
		}

		if( ( c >= '0' ) && ( c <= '9' ) ) /* offsets are given decimal */
		{
			STRPTR comma = strchr(s+1, ',');

			if( comma )
			{
				startoffset = strtoull( s, NULL, 10 ); /* strtoull stops at the comma */

				D(RECOG,bug("\tnew startoffset: %lld\n", startoffset));

				s = comma + 1;
			}
		}
		else if( negative )
		{
			return( RECOG_WRONGINPUT );
		}

		if( negative )
		{
			if( startoffset >= filesize )
			{
				return( RECOG_FALSE );
			}

			startoffset = filesize - startoffset; /* calculate the real startoffset */
		}
	}


	mode = tolower( *s++ );

	if( !*s )
	{
		return( RECOG_WRONGINPUT );
	}

	/*  Calculate size of data we have to read in
	 */
	if( mode == MATCHMODE_HEX )
	{
		length = strlen(s) / 2; /* we ignore a possible trailing character at uneven position, simplifies code and is all we need anyway */
	}
	else if( ( mode == MATCHMODE_CASESENSITIVE ) || ( mode == MATCHMODE_CASEINSENSITIVE ) )
	{
		length = strlen( s );
	} else {
		return( RECOG_WRONGINPUT );
	}

	D(RECOG,bug("\tmode: %c\n", mode));


	/*   fill cache
	 */
	DOFILL( startoffset, length, &buffer );


	/*   last but not least the actual matching
	 */
	switch( mode )
	{
		case MATCHMODE_HEX:
			{
				D(RECOG,bug("\thex\n"));

				while( length )
				{
					if( *s == '?' && * ( s + 1 ) == '?' )
					{
						/* continue */
					}
					else if( *buffer !=  ( ( hctod( s ) * 16 ) + hctod( s + 1 ) ) )
					{
						break;
					}
					s += 2;
					buffer++;
					length--;
				}
			}
			break;

		case MATCHMODE_CASESENSITIVE:
			{
				D(RECOG,bug("\tcase sensitive\n"));

				while( length )
				{
					if( ( *s != '?' ) && ( *s != *buffer ) )
					{
						break;
					}
					s++;
					buffer++;
					length--;
				}
			}
			break;

		case MATCHMODE_CASEINSENSITIVE:
			{
				D(RECOG,bug("\tcase insensitive\n"));

				while( length )
				{
					if( ( *s != '?' ) && ( toupper( *s ) != toupper( *buffer ) ) )
					{
						break;
					}
					s++;
					buffer++;
					length--;
				}
			}
			break;

		default:
			D(RECOG,bug("wtf.\n"));
	}

	return( (length) ? RECOG_FALSE : RECOG_TRUE );
}

/************************************************************************/


/*
 *   a match string can look like:
 *
 *	hunk,offset,match
 *	offset,match
 *	match
 *
 *	hunk 0 is the first hunk, and default if no hunk is specified.
 *	hunk 1 is the second hunk. hunk -1 is the last hunk. -2 is the 2nd last
 *	hunk.
 *	If specified hunk doesn't exist, there is no match.
 *	If the specified hunk is HUNK_BSS, there is no match.
 *	If HUNK_OVERLAY or HUNK_BREAK are met before the specified hunk, there
 *	is no match.
 *	Matching is done without relocation, so the relocation offsets can be
 *	used for matching aswell, unlike with LoadSeg()d binary.
 *
 *	offset is offset inside the hunk. match is a match in same format as
 *	for "Match" command.
 */

#undef DOFILL
#define DOFILL(o,l,p) do { LONG __rc; if ((__rc = fillcache(fct, o, l, (CONST_APTR *)(p))) != RECOG_TRUE) { err = __rc; goto err; } } while (0)

static ULONG recogcmd_matchhunk( struct recog_filectx *fct, CONST_STRPTR data )
{
	ULONG err         = RECOG_FALSE;
	ULONG matchhunknumber = 0;
	UQUAD filesize    = fct->fi.fi_Size;
	BOOL  negative    = FALSE;

	CONST_STRPTR s    = data;
	ULONG *buffer;
	UQUAD offset;
	ULONG *hunksizearray = NULL;
	ULONG numhunks;
	ULONG firsthunk;
	ULONG lasthunk;
	ULONG hunknum;
	ULONG hunkindex;
	ULONG hunksize;
	BOOL  hashunk;

	ASSERT( fct );
	ASSERT( fct->fh );
	ASSERT( data );

	D(RECOG,bug("matchhunk: %s\n", data));

	/*  Check if the match string has a hunk: "hunk,offset,"
	 */
	{
		UBYTE c = *s;

		if( c == '-' )
		{
			negative = TRUE;

			s++;

			if( !( c = *s ) )
			{
				return( RECOG_WRONGINPUT );
			}
		}

		if( (c >= '0' ) && ( c <= '9' ) ) /* hunk numbers are given decimal */
		{
			STRPTR comma = strchr( s + 1, ',' );

			if( comma )
			{
				c = comma[ 1 ];

				/*
				 * If just single value, then it is offset within hunk 0
				 */
				if( ( c >= '0' ) && ( c <= '9' ) )
				{
					matchhunknumber = strtoul( s, NULL, 10 ); /* strtoul stops at the comma */

					D(RECOG,bug("\thunk number: %lu\n", matchhunknumber));

					data = comma + 1;
					//DB(("matchhunknumber %lu, data <%s>\n", matchhunknumber, data));
				}
			}
		}
		else if (negative)
		{
			return RECOG_WRONGINPUT;
		}
	}

	/*   fetch enough data so we get the header
	 *   40 bytes is the smallest possible exe with actual data
	 */
	DOFILL(0, 40, &buffer);


	/* Verify that this indeed is an executable, if not it's a mismatch */
	if( buffer[0] != HUNK_HEADER || buffer[1] != 0 )
	{
		goto err;
	}

	numhunks  = buffer[2] & 0xffff;
	firsthunk = buffer[3] & 0xffff;
	lasthunk  = buffer[4] & 0xffff;

	/* Quickly check if we're not going to match anyway */
	if( matchhunknumber - negative > lasthunk - firsthunk )
	{
		//DB(("matchhunknumber (%ld) out of range (%lu to %lu hunks)\n",
		//    negative ? -matchhunknumber : matchhunknumber, firsthunk, lasthunk));
		goto err;
	}

	if (negative)
	{
		matchhunknumber = lasthunk - firsthunk - matchhunknumber + 1;
	}

	hunksizearray = malloc(numhunks * sizeof(ULONG));
	if (!hunksizearray)
	{
		//DB(("out of memory allocating hunksizearray\n"));
		err = RECOG_OUTOFMEM;
		goto err;
	}

	offset = (buffer[2] & 0xc0000000) == 0xc0000000 ? 24 : 20;
	for (hunkindex = firsthunk;
	     hunkindex < numhunks && hunkindex <= lasthunk;
	     hunkindex++)
	{
		DOFILL(offset, sizeof(ULONG), &buffer);
		offset += sizeof(ULONG);
		hunksizearray[ hunkindex ] = ( buffer[0] & 0x1fffffff ) * sizeof( ULONG );

		if( ( buffer[0] & 0xc0000000 ) == 0xc0000000 )
		{
			offset += sizeof(ULONG);
		}
	}
	while( hunkindex < numhunks )
	{
		hunksizearray[ hunkindex++ ] = 0;
	}

	//DB(("0x%08llx: first hunk, %lu total hunks (%lu to %lu)\n", offset, lasthunk - firsthunk + 1, firsthunk, lasthunk));

	hunkindex = firsthunk;
	hunknum = 0;
	hunksize = 0;
	hashunk = FALSE;
	for (;;)
	{
		ULONG type;
		ULONG rsize;

		DOFILL( offset, sizeof(ULONG), &buffer );
		//DB(("0x%08llx: hunk 0x%08lx\n", offset, buffer[0] & 0x3fffffff));
		offset += sizeof(ULONG);
		type = buffer[0] & 0x3fffffff;

		switch (type)
		{
			case HUNK_NAME:
			case HUNK_DEBUG:
				{
					ULONG xcnt;

					//DB(("0x%08llx: HUNK_NAME/HUNK_DEBUG, skip data\n", offset));

					DOFILL( offset, sizeof(ULONG), &buffer );
					offset += sizeof(ULONG);
					xcnt = buffer[0] & 0x00ffffff;
					/* Seek past the data */
					offset += xcnt * sizeof(ULONG);
				}
				break;

			case HUNK_CODE:
			case HUNK_DATA:
			case HUNK_BSS:
				DOFILL( offset, sizeof(ULONG), &buffer );
				offset += sizeof(ULONG);

				//DB(("0x%08llx: HUNK_<data> number %lu\n", offset, hunknum));

				if( hunkindex > lasthunk )
				{
					//DB(("0x%08llx: HUNK_<data> too many hunks! %lu > %lu\n", offset, hunkindex, lasthunk));
					goto err;
				}

				if( hunknum == matchhunknumber )
				{
					struct recog_filectx *clonefct;
					ULONG rc;
					ULONG hunksize;

					free( hunksizearray );
					hunksizearray = NULL;

					hunksize = ( buffer[0] & 0x3fffffff ) * sizeof( ULONG );
					//DB(("0x%08llx: HUNK_<data> %lu bytes of data\n", offset, hunksize));

					if (offset >= filesize || offset + hunksize > filesize || type == HUNK_BSS )   /* There's no data to match! */
					{
						//DB(("0x%08llx: HUNK_<data> something stinks. hunksize %lu type %lu!\n", offset, hunksize, type));
						goto err;
					}

					clonefct = malloc( sizeof( *clonefct ) );
					if( !clonefct )
					{
						err = RECOG_OUTOFMEM;
						//DB(("0x%08llx: HUNK_<data> out of memory\n", offset));
						goto err;
					}

					/* Rather neat trick here */

					memcpy( clonefct, fct, sizeof( *clonefct ) );

					//DB(("0x%08llx: HUNK_<data> call regular match with offset %llu size %lu, match <%s>\n", offset, offset, hunksize, data));
					clonefct->filebase   = offset;
					clonefct->fi.fi_Size = offset + hunksize;

					rc = recogcmd_match( clonefct, data );

					free( clonefct );

					return( rc );
				}
				if( type != HUNK_BSS )
				{
					/* Seek past the data */
					//DB(("0x%08llx: HUNK_<data> %lu bytes skipped\n", offset, buffer[0] * sizeof(ULONG)));
					offset += buffer[0] * sizeof(ULONG);
				}
				hunknum++;
				hunksize = hunksizearray[ hunkindex++ ];
				hashunk = TRUE;
				break;

			case HUNK_RELOC32:
			case HUNK_RELRELOC32:
			case HUNK_DREL32:
			case HUNK_RELOC32SHORT:
				if( !hashunk )
				{
					//DB(("0x%08llx: HUNK_RELOC#? without hunk\n", offset));
					goto err;
				}

				rsize = ( type == HUNK_RELOC32 || type == HUNK_RELRELOC32 ) ? sizeof(ULONG) : sizeof(UWORD);

				for (;;)
				{
					ULONG rcnt, rhunk;

					DOFILL( offset, rsize, &buffer );
					offset += rsize;
					rcnt = buffer[0] >> ( rsize == sizeof(UWORD) ? 16 : 0 );
					//DB(("0x%08llx: HUNK_RELOC#? %lu relocs\n", offset, rcnt));
					if( !rcnt )
						break;

					DOFILL(offset, rsize, &buffer);
					offset += rsize;
					rhunk = buffer[0] >> ( rsize == sizeof(UWORD) ? 16 : 0 );
					//DB(("0x%08llx: HUNK_RELOC#? ...to hunk %lu\n", offset, rhunk));
					if( rhunk > numhunks )
					{
						//DB(("0x%08llx: HUNK_RELOC#? rhunk > numhunks (%lu > %lu)\n", offset, rhunk, numhunks));
						goto err;
					}

					rcnt = ( ( ( rcnt + 1 ) & 0x0000ffff ) - 1 ) & 0x0000ffff;
					if (rcnt == 0)
						rcnt = 65536;

					do
					{
						ULONG roff;

						DOFILL( offset, rsize, &buffer );
						offset += rsize;

						roff = buffer[0] >> ( rsize == sizeof(UWORD) ? 16 : 0 );
						if( roff + rsize - 1 > hunksize )
						{
							//DB(("0x%08llx: HUNK_RELOC#? offset outside of hunk (%lu > %lu)\n", offset, roff + rsize - 1, hunksize));
							goto err;
						}
					} while( --rcnt );
				}

				if (rsize == sizeof(UWORD) && ( offset & 2 ) )
				{
					DOFILL( offset, rsize, &buffer );
					offset += rsize;
				}
				break;

			case HUNK_SYMBOL:
				//DB(("0x%08llx: HUNK_SYMBOL: skip...\n", offset));
				for( ; ; )
				{
					ULONG scnt;

					DOFILL(offset, sizeof(ULONG), &buffer);
					offset += sizeof(ULONG);
					scnt = buffer[0] & 0x00ffffff;
					if (!scnt)
						break;

					/* seek past the data */
					offset += scnt * sizeof(ULONG);

					DOFILL(offset, sizeof(ULONG), &buffer );
					offset += sizeof(ULONG);
				}
				break;

			case HUNK_END:
				hashunk = FALSE;
				break;

			//case HUNK_OVERLAY:
			//case HUNK_BREAK:
			default:
				//DB(("0x%08llx: unknown/unsupported hunk type 0x%08lx\n", offset, type));
				goto err;
		}
	}

err:

	if( hunksizearray )
	{
		free( hunksizearray );
	}
	return( err );
}


enum {
	CONTENTTYPE_UNKNOWN = 0,
	CONTENTTYPE_TEXT,
	CONTENTTYPE_BINARY
};

/************************************************************************/

static ULONG recogcmd_content( struct recog_filectx *fct, CONST_STRPTR data )
{
	ULONG rc = RECOG_FALSE;
	UBYTE contenttype;

	ASSERT(fct);
	ASSERT( fct->fh );
	ASSERT( data );

	contenttype = fct->contenttype;

	if( contenttype == CONTENTTYPE_UNKNOWN )
	{
		UQUAD filesize = fct->fi.fi_Size;
		ULONG len      = (filesize < RECOGBUF_SIZE) ? filesize : RECOGBUF_SIZE;
		UBYTE *cache   = fct->cache;

		D(RECOG,bug("\t\t block 0\n"));

		if( BLOCKISEMPTY( fct->cachestatus, 0 ) )
		{
			if( file_seek( fct->fh, 0, OFFSET_BEGINNING ) == -1LL )
			{
				D(RECOG,bug("\nseeking failed (requested offset: 0, filehandle: 0x%lx)\n", fct->fh));
				return RECOG_IOERROR;
			}

			if (!file_read( fct->fh, cache, len ) )
			{
				D(RECOG,bug("file_read() failed.\n"));
				return RECOG_IOERROR;
			}

			BLOCKOK( fct->cachestatus, 0 );
		}

		contenttype = CONTENTTYPE_TEXT;

		while( len-- )
		{
			if( isbinary( cache[ len ] ) )
			{
				D(RECOG,bug("\t\t\tbinary detected\n"));
				contenttype = CONTENTTYPE_BINARY;
				break;
			}
		}

		/*   cache the value
		 */
		fct->contenttype = contenttype;
	}

	/*   argument string can be "Text" or "Binary" (or just "T" or "B" etc.)
	 */
	if( ( data[0] == 'T' ) || ( data[0] == 't' ) )
	{
		if( contenttype == CONTENTTYPE_TEXT )
		{
			rc = RECOG_TRUE;
		}
	} else {
		/*  we explicitly check for binary content, so we can detect text files
		 *  with wrong suffix
		 */
		if( contenttype == CONTENTTYPE_BINARY )
		{
			rc = RECOG_TRUE;
		}
	}

	return( rc );
}

/************************************************************************/

/*
 * Recog proding functions.
 */
APTR recog_open( CONST_STRPTR name )
{
	struct recog_filectx *ct;

	THREAD;
	ASSERT( name );

	if( ( ct = malloc( sizeof( *ct ) + ( strlen( name ) + 1) ) ) )
	{
		strcpy( ct->name, name );

		ct->fh          = NULL;
		ct->filebase    = 0;
		ct->cachestatus = 0;
		ct->contenttype = CONTENTTYPE_UNKNOWN;

		/*  Always examine() first. We check first if it's a directory, also filename
		 *  matching only happens together with "File" (which needs a filled FIB too), so
		 *  the Examine() is required anyway and therefore means no additional performance
		 *  issue. -- tokai
		 */
		if( examine64( name, &ct->fi ) )
		{
			if( ct->fi.fi_Type < 1 )
			{
				ct->fh = file_open( name, MODE_OLDFILE );
			}
		}
		return( ct );
	}

	return( NULL );
}

/************************************************************************/

void recog_close( APTR fctx )
{
	struct recog_filectx *ct = fctx;
	THREAD;
	ASSERT( ct );

	if ( ct->fh ) {
		file_close( ct->fh );
	}

	free( ct );
}



/* expression evaluation helpers for recog_prod() */


/*
 * parenthesed expressions are supported, but operators priority is NOT
 * supported. So, don't mix AND and OR in a chain. a chain will be broken
 * as soon as possible between two parenthesis.
 * (FALSE OR TRUE OR FALSE) will stop at second term and return TRUE.
 * (TRUE AND FALSE AND TRUE) will stop at second term and return FALSE.
 * At the moment, all parenthesed groups have to be evaluated.
 */

#define RECOG_MAX_DEPTH 128
enum { TYPE_SYMBOL, TYPE_VALUE };

typedef ULONG (*RECOG_FUNC)( struct recog_filectx *fct, CONST_STRPTR data );

struct recog_item
{
	ULONG        type;
	RECOG_FUNC   func;
	CONST_STRPTR data;
	ULONG        usevalue;
	ULONG        value;
	ULONG        fileio;    /* TRUE if we have a filehandle */
	ULONG        usefileio; /* TRUE if fileio is required   */
};

struct recog_stack
{
	struct recog_item p[ RECOG_MAX_DEPTH ];
	int    top;
};

/************************************************************************/

static void recog_stack_init( struct recog_stack * s )
{
	s->top = -1;
}

/************************************************************************/

static int recog_stack_empty( struct recog_stack * s )
{
	return( s->top == -1 );
}

/************************************************************************/

static void recog_stack_push( struct recog_stack * s, struct recog_item * i )
{
	if( s->top < ( RECOG_MAX_DEPTH - 1 ) )
	{
		memcpy( &s->p[ ++s->top ], i, sizeof( *i ) );
	}
}

/************************************************************************/

static struct recog_item * recog_stack_pop( struct recog_stack * s )
{
	return( &s->p[s->top--] );
}

/************************************************************************/

static struct recog_item * recog_stack_top( struct recog_stack * s )
{
	return( &s->p[s->top] );
}

/************************************************************************/

static ULONG recog_eval( struct recog_filectx * fct, struct recog_item *a )
{
	ULONG value = a->value;

	if ( ( !a->usefileio ) || ( a->usefileio && a->fileio ) )
	{
		if( !a->usevalue )
		{
			a->value    = a->func( fct, a->data );
			a->usevalue = TRUE; /* don't reevaluate later */
		}
	}

	return( value );
}

/************************************************************************/

ULONG recog_prod( APTR fctx, APTR ctx, ULONG fileio )
{
	struct recog_ctx *ct = ctx;
	struct recog_filectx * fct = fctx;
	struct recognode *rn;
	struct recog_stack s;
	ULONG ok_final = RECOG_FALSE;
	//ULONG ok = RECOG_FALSE;
	ULONG parse_error = FALSE;

	THREAD;
	ASSERT( fct );
	ASSERT( ct );

	D(RECOG,bug("Name: %s (IO: %s)\n", fct->name, fct->fh ? "Yes" : "No"));

	recog_stack_init( &s );

	if( !fct->fh )
	{
		fileio = FALSE;
	}

	if( !ISLISTEMPTY( &ct->l ) )
	{
		ok_final = RECOG_FALSE;

		ITERATELIST( rn, &ct->l )
		{
			struct recog_item i;

			switch( rn->cmd )
			{
				case RECOGCMD_NONE:
					break;

				case RECOGCMD_OR: /* || */
					D(RECOG,bug("OR***\n"));
					i.type  = TYPE_SYMBOL;
					i.func  = NULL;
					i.data  = NULL;
					i.value = RECOGCMD_OR;
					recog_stack_push( &s, &i );
					break;

				case RECOGCMD_AND: /* && */
					D(RECOG,bug("AND***\n"));
					i.type  = TYPE_SYMBOL;
					i.func  = NULL;
					i.data  = NULL;
					i.value = RECOGCMD_AND;
					recog_stack_push( &s, &i );
					break;

				case RECOGCMD_BLOCKIN: /* ( */
					D(RECOG,bug("(***\n"));
					i.type  = TYPE_SYMBOL;
					i.func  = NULL;
					i.data  = NULL;
					i.value = RECOGCMD_BLOCKIN;
					recog_stack_push( &s, &i );
					break;

				case RECOGCMD_BLOCKOUT: /* ) */
				{
					struct recog_item finalval;
					struct recog_item op;
					struct recog_item * top;
					int gotop   = 0;
					int skip = FALSE;

					D(RECOG,bug(")***\n"));

					top = recog_stack_top( &s );

					while( !parse_error && !recog_stack_empty( &s ) && ( top->type != TYPE_SYMBOL || top->value != RECOGCMD_BLOCKIN ) )
					{
						if( !skip )
						{
							if( top->type == TYPE_VALUE )
							{
								if( gotop )
								{
									recog_eval( fct, top );

									if( op.value == RECOGCMD_AND )
									{
										if( top->value == RECOG_FALSE )
										{
											memcpy( &finalval, top, sizeof( finalval ) );
											skip = TRUE;
										} else {
											recog_eval( fct, top );
										}
									}

									if( op.value == RECOGCMD_OR )
									{
										if( top->value == RECOG_TRUE )
										{
											memcpy( &finalval, top, sizeof( finalval ) );
											skip = TRUE;
										} else {
											recog_eval( fct, top );
										}
									}
									gotop = 0;
								} else {
									memcpy( &finalval, top, sizeof( finalval ) );
								}
							}

							if( top->type == TYPE_SYMBOL )
							{
								memcpy( &op, top, sizeof( op ) );
								gotop = 1;
							}
						}

						recog_stack_pop( &s );
						top = recog_stack_top( &s );
					}

					/* replace ( with the value of ( ) block. */
					if( !parse_error && top->type == TYPE_SYMBOL && top->value == RECOGCMD_BLOCKIN )
					{
						recog_eval( fct, &finalval );
						recog_stack_pop( &s );
						recog_stack_push( &s, &finalval );
						D(RECOG, bug("block value = %ld\n", finalval.value ) );
					} else {
						if( !parse_error )
						{
							D(RECOG, bug("parse error\n"));
							parse_error = TRUE;
						}
					}

					if( parse_error )
					{
						goto parse_end;
					}

					break;
				}

				case RECOGCMD_MATCH:

					D(RECOG,bug("calling recog match..\n"));

					i.type      = TYPE_VALUE;
					i.func      = recogcmd_match;
					i.data      = rn->data;
					i.fileio    = fileio;
					i.usefileio = TRUE;
					i.usevalue  = FALSE;
					i.value     = RECOG_FALSE;
					recog_stack_push( &s, &i );

					break;

				case RECOGCMD_MATCHHUNK:

					D(RECOG,bug("calling recog matchhunk..\n"));

					i.type      = TYPE_VALUE;
					i.func      = recogcmd_matchhunk;
					i.data      = rn->data;
					i.fileio    = fileio;
					i.usefileio = TRUE;
					i.usevalue  = FALSE;
					i.value     = RECOG_FALSE;
					recog_stack_push( &s, &i );

					break;

				case RECOGCMD_FILENAME:

					D(RECOG,bug("calling recog filename..\n"));

					i.type      = TYPE_VALUE;
					i.func      = recogcmd_filename;
					i.data      = rn->data;
					i.usefileio = FALSE;
					i.usevalue  = FALSE;
					i.value     = RECOG_FALSE;
					recog_stack_push( &s, &i );

					break;

				case RECOGCMD_FILESIZE:

					D(RECOG,bug("calling recog filesize..\n"));

					i.type      = TYPE_VALUE;
					i.func      = recogcmd_filesize;
					i.data      = rn->data;
					i.usefileio = FALSE;
					i.usevalue  = FALSE;
					i.value     = RECOG_FALSE;
					recog_stack_push( &s, &i );

					break;

				case RECOGCMD_DATATYPES:

					D(RECOG,bug("asking datatypes subsystem..\n"));

					i.type      = TYPE_VALUE;
					i.func      = recogcmd_datatypes;
					i.data      = NULL;
					i.usefileio = FALSE; /* dtypes have own fileio */
					i.usevalue  = FALSE;
					i.value     = RECOG_FALSE;
					recog_stack_push( &s, &i );

					break;

				#if USE_MULTIMEDIA
				case RECOGCMD_MULTIMEDIA:

					D(RECOG,bug("asking multimedia subsystem..\n"));

					i.type      = TYPE_VALUE;
					i.func      = recogcmd_multimedia;
					i.data      = NULL;
					i.usefileio = FALSE; /* reggae has own fileio */
					i.usevalue  = FALSE;
					i.value     = RECOG_FALSE;
					recog_stack_push( &s, &i );

					break;
				#endif

				case RECOGCMD_FINDSTRING:

					D(RECOG,bug("calling recog find string..\n"));

					i.type      = TYPE_VALUE;
					i.func      = recogcmd_findstring;
					i.data      = rn->data;
					i.fileio    = fileio;
					i.usefileio = TRUE;
					i.usevalue  = FALSE;
					i.value     = RECOG_FALSE;
					recog_stack_push( &s, &i );

					break;


				case RECOGCMD_CONTENT:

					D(RECOG,bug("calling recog content..\n"));

					i.type      = TYPE_VALUE;
					i.func      = recogcmd_content;
					i.data      = rn->data;
					i.fileio    = fileio;
					i.usefileio = TRUE;
					i.usevalue  = FALSE;
					i.value     = RECOG_FALSE;
					recog_stack_push( &s, &i );

					break;


				case RECOGCMD_PROTECTION:

					D(RECOG,bug("calling recog protection..\n"));

					i.type      = TYPE_VALUE;
					i.func      = recogcmd_protection;
					i.data      = rn->data;
					i.usefileio = FALSE;
					i.usevalue  = FALSE;
					i.value     = RECOG_FALSE;
					recog_stack_push( &s, &i );

					break;

				case RECOGCMD_DEVICE:

					D(RECOG,bug("calling recog device..\n"));

					i.type      = TYPE_VALUE;
					i.func      = recogcmd_isdevice;
					i.data      = NULL;
					i.usefileio = FALSE;
					i.usevalue  = FALSE;
					i.value     = RECOG_FALSE;
					recog_stack_push( &s, &i );

					break;

				case RECOGCMD_DIRECTORY:

					D(RECOG,bug("calling recog directory..\n"));

					i.type      = TYPE_VALUE;
					i.func      = recogcmd_isdirectory;
					i.data      = NULL;
					i.usefileio = FALSE;
					i.usevalue  = FALSE;
					i.value     = RECOG_FALSE;
					recog_stack_push( &s, &i );

					break;

				case RECOGCMD_COMMENT:

					D(RECOG,bug("calling recog comment..\n"));

					i.type      = TYPE_VALUE;
					i.func      = recogcmd_comment;
					i.data      = rn->data;
					i.usefileio = FALSE;
					i.usevalue  = FALSE;
					i.value     = RECOG_FALSE;
					recog_stack_push( &s, &i );

					break;

				case RECOGCMD_FILE:

					D(RECOG,bug("calling recog file..\n"));

					i.type      = TYPE_VALUE;
					i.func      = recogcmd_isfile;
					i.data      = NULL;
					i.usefileio = FALSE;
					i.usevalue  = FALSE;
					i.value     = RECOG_FALSE;
					recog_stack_push( &s, &i );

					break;

				#ifdef DEBUG
				default:
					PDB(("unknown cmd 0x%lx\n", rn->cmd));
					break;
				#endif
			}
		}

parse_end:
		if( !parse_error )
		{
			struct recog_item finalval;
			struct recog_item op;
			struct recog_item * top;
			BOOL gotop = FALSE;
			BOOL skip  = FALSE;

			top = recog_stack_top( &s );

			while( !parse_error && !recog_stack_empty( &s ) )
			{
				if( !skip )
				{
					if( top->type == TYPE_VALUE )
					{
						if( gotop )
						{
							recog_eval( fct, top );

							if( op.value == RECOGCMD_AND )
							{
								/* break the chain as soon as a AND and a false value are detected */
								if( top->value == RECOG_FALSE )
								{
									memcpy( &finalval, top, sizeof( finalval ) );
									skip = TRUE;
								} else {
									recog_eval( fct, &finalval );
								}
							}

							if( op.value == RECOGCMD_OR )
							{
								/* break the chain as soon as a OR and a true value are detected */
								if( top->value == RECOG_TRUE )
								{
									memcpy( &finalval, top, sizeof( finalval ) );
									skip = TRUE;
								} else {
									recog_eval( fct, &finalval );
								}
							}
							gotop = FALSE;
						} else {
							memcpy( &finalval, top, sizeof( finalval ) );
						}
					}

					if( top->type == TYPE_SYMBOL )
					{
						memcpy( &op, top, sizeof( op ) );
						gotop = TRUE;
					}
				}

				recog_stack_pop( &s );
				top = recog_stack_top( &s );
			}

			if( !parse_error )
			{
				recog_eval( fct, &finalval );
				D(RECOG, bug("final value = %ld\n", finalval.value ) );
				ok_final = finalval.value;
			} else {
				ok_final = FALSE;
			}
		} else {
			ok_final = FALSE;
		}

	}
	return( ok_final );
}

/************************************************************************/

/*
 * Recog building functions.
 */

APTR recog_create( void )
{
struct recog_ctx *ct;

	if ( ( ct = malloc( sizeof( *ct ) ) ) )
	{
		NEWLIST( &ct->l );
		ct->pattern       = NULL;
		ct->parsedpattern = NULL;
	}

	return ( ct );
}

/************************************************************************/

ULONG recog_add( APTR ctx, ULONG cmd, CONST_STRPTR data )
{
struct recog_ctx *ct = ctx;
struct recognode *rn;

	ASSERT( ct );

	if( ( rn = malloc( sizeof( *rn ) + ( data ? ( strlen( data ) + 1 ) : 0 ) ) ) )
	{
		rn->cmd = cmd;
		if( data )
		{
			strcpy( rn->data, data );
		}
		ADDTAIL( &ct->l, rn );

		return( TRUE );
	}
	return( FALSE );
}

/************************************************************************/

APTR recog_duplicate( APTR ctx )
{
struct recog_ctx *srctx = (struct recog_ctx *) ctx;
struct recog_ctx *drctx = NULL;

	if( srctx )
	{
		if( (drctx = recog_create() ) )
		{
			ULONG success = TRUE;
			struct recognode * rn;

			drctx->pattern = srctx->pattern ? name_build( srctx->pattern ) : NULL;

			if( srctx->pattern && !drctx->pattern )
			{
				success = FALSE;
			}

			ITERATELIST( rn, &srctx->l )
			{
				if( !recog_add( drctx, rn->cmd, rn->data ) )
				{
					success = FALSE;
					break;
				}
			}

			if( !success )
			{
				recog_delete( drctx );
				drctx = NULL;
			}
		}
	}

	return( (APTR) drctx );
}

/************************************************************************/

void recog_sethintpattern( APTR ctx, CONST_STRPTR pattern )
{
struct recog_ctx *ct = ctx;

	if( ct->pattern )
	{
		name_delete( ct->pattern );
	}

	if( ct->parsedpattern )
	{
		free( ct->parsedpattern );
	}

	ct->pattern = NULL;
	ct->parsedpattern = NULL;

	if( pattern )
	{
		ct->pattern = name_build( pattern );
	}
}

/************************************************************************/

ULONG recog_checkhint( APTR ctx, CONST_STRPTR name )
{
struct recog_ctx *ct = ctx;

	if( ct->pattern != NULL )
	{
		if( ct->parsedpattern == NULL )
		{
			LONG len = strlen( ct->pattern );
			ct->parsedpattern = malloc( len * 2 + 2 );
			if( ct->parsedpattern != NULL )
			{
				if( ParsePatternNoCase( ct->pattern, ct->parsedpattern, len * 2 + 2 ) == -1 )
				{
					free( ct->parsedpattern );
					ct->parsedpattern = NULL;
					return( FALSE );
				}
			} else {
				return( FALSE );
			}
		}

		return( MatchPatternNoCase( ct->parsedpattern, FilePart( name ) ) );
	}

	return( FALSE );
}

/************************************************************************/

void recog_delete( APTR ctx )
{
struct recog_ctx *ct = ctx;
struct recognode *rn, *nextrn;

	ASSERT(ct);

	ITERATELISTSAFE( rn, nextrn, &ct->l )
	{
		free(rn);
	}

	if ( ct->pattern )
	{
		name_delete( ct->pattern );
	}

	if ( ct->parsedpattern )
	{
		free( ct->parsedpattern );
	}
	free( ct );
}

/************************************************************************/

ULONG recog_getfhattr( APTR fctx, ULONG val )
{
struct recog_filectx *ct = fctx;

	ASSERT(ct);

	switch ( val )
	{
		case RECOGFHATTR_USERDATA:
			return( ct->userdata );

		#ifdef DEBUG
		default:
			PDB(("wrong val %lu\n", val));
			break;
		#endif
	}
	return( 0 );
}
