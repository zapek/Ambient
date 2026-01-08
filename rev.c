/*
** Yet another revision generator
**
** Written by Oliver Wagner <owagner@vapor.com>
** Initially public domain but Oliver kindly game me the
** permission to GPL it for the Ambient GPL release. Thanks!
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
**
** $Id: rev.c,v 1.6 2017/07/29 16:26:47 piru Exp $
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>

#define PATH_MAX 256


static const char version[]
#if __GNUC__ > 2
__attribute__((used))
#endif
= "$VER: Rev 5.1 (2.1.01)";

long __oslibversion = 37;


/********************************/
typedef unsigned long BOOL;
#define TRUE 1
#define FALSE 0

typedef long LONG;
typedef unsigned long ULONG;
typedef char * STRPTR;


/********************************/

char startdir[PATH_MAX];
char * startdirp = NULL;

BOOL exists( char * filename )
{
	FILE * f = fopen(filename, "r");

	if(f)
	{
		fclose(f);
		return (TRUE);
	}
	return( FALSE );
}

LONG cmpdate( char *filename1, char *filename2 )
{
	struct stat st1, st2;
	LONG rc = 0;

	if ((stat(filename1, &st1) == 0) && (stat(filename2, &st2) == 0))
	{
		rc = st2.st_mtime - st1.st_mtime;
	}

	return (rc);
}

static ULONG smarttouch(STRPTR filename1, STRPTR filename2)
{
	struct stat st1, st2;

	if ( (stat(filename1, &st1) == 0) && (stat(filename2, &st2) == 0) )
	{
		if( st1.st_mtime == st2.st_mtime )
		{
			return (FALSE);	/* the number of seconds are the same so we don't need to touch */
		}
	}
	return (TRUE);
}

struct args {
	ULONG dir;
	ULONG incver;
	ULONG increv;
	ULONG inccomp;
	ULONG *libid;
	ULONG *base;
	ULONG global;
	ULONG printhexid;
	ULONG printver;
	ULONG printrev;
	ULONG touch;
	ULONG stouch;
	ULONG quiet;
} args;

int	main(int argc, char * argv[])
{
	char buffer[ 128 ], *p;
	char revinfoname[ 32 ]=".revinfo", revhname[ 32 ]="rev.h";
	int version = 1, revision = 0, compile = 0, quiet, changed = FALSE;
	FILE * f;
	char x[ 8 ];
	struct tm * lt;
	time_t stamp;

	//rda = ReadArgs( "DIRECTORY=DIR/K,INCVER=VER/S,INCREV=REV/S,INCCOMP=COMP/S,LIBID/K,BASE/K,GLOBAL/S,PRINTHEXID/S,PRINTVER/S,PRINTREV/S,TOUCH/S,SMARTTOUCH/S,QUIET/S", (LONG *)&args, NULL );

	memset(&args, 0, sizeof(args));

	if(argc == 1)
	{
		//fprintf( stderr, "REV" );
		// exit( 10 );
	}

	if(argc >= 2)
	{
		if(!strcasecmp(argv[1], "INCREV"))
		{
			args.increv = TRUE;
		}
		else if(!strcasecmp(argv[1], "INCVER"))
		{
			args.incver = TRUE;
		}
		else if(!strcasecmp(argv[1], "INCCOMP"))
		{
			args.inccomp = TRUE;
		}
		else if(!strcasecmp(argv[1], "SMARTTOUCH"))
		{
			args.stouch = TRUE;
		}
		else if(!strcasecmp(argv[1], "TOUCH"))
		{
			args.touch = TRUE;
		}
		else if(strstr(argv[1], "DIRECTORY="))
		{
			char * p = strstr(argv[1], "=");

			if(*(p+1))
			{
				args.dir = (ULONG) p+1;
			}
		}
	}

	if(argc == 3)
	{
		if(!strcasecmp(argv[2], "QUIET"))
		{
			args.quiet = TRUE;
		}
	}

	if( args.dir )
	{
		if( exists((STRPTR) args.dir))
		{
			startdirp = getcwd(startdir, PATH_MAX);
			chdir((STRPTR) args.dir);
		}
		else
		{
			fprintf( stderr, "REV" );
			exit( 10 );
		}
	}

	if( args.base )
	{
		sprintf( revinfoname, "%s.revinfo", (STRPTR) args.base );
		sprintf( revhname, "%s_rev.h", (STRPTR) args.base );
	}

	f = fopen( revinfoname, "r" );
	if( f )
	{
		memset( buffer, 0, 128 );
		fread( buffer, 127, 1, f );
		fclose( f );
		version = atoi( buffer );
		p = strchr( buffer, '.' );
		if( p )
		{
			revision = atoi( ++p );
			p = strchr( p, '.' );
			if( p )
				compile = atoi( ++p );
		}
	}
	else changed = TRUE;
	quiet = args.quiet;
	if( args.incver )
	{
		version++;
		revision = 0;
		compile = 0;
		changed = TRUE;
	}
	else if( args.increv )
	{
		revision++;
		compile = 0;
		changed = TRUE;
	}
	else if( args.inccomp )
	{
		compile++;
		changed = TRUE;
	}

	if( args.printver )
	{
		printf( "%d", version );
		if( args.printrev )
			printf( "." );
		else
		{
			fflush( stdout );
			quiet = TRUE;
		}
	}
	if( args.printrev )
	{
		printf( "%d", revision );
		fflush( stdout );
		quiet = TRUE;
	}
	if( args.printhexid )
	{
		printf( "0x%x\n", ( version << 16 ) | revision );
		fflush( stdout );
		quiet = TRUE;
	}

	stamp = time( NULL );
	lt = localtime( &stamp );

	x[0] = lt->tm_year;
	x[1] = lt->tm_mon+1;
	x[2] = lt->tm_mday;

	if( !args.quiet )
	{
		printf( "REV: current revision %d.%d.%d (%02d.%02d.%04d)\n", version, revision, compile, x[ 2 ], x[ 1 ], x[ 0 ] + 1900 );
		fflush( stdout );
	}

	if( changed || args.touch || (args.stouch && smarttouch(revinfoname, revhname)) || !exists( revinfoname ) )
	{
		f = fopen( revinfoname, "w+" );
		if( f )
		{
			if( compile )
				fprintf( f, "%d.%d.%d", version, revision, compile );
			else
				fprintf( f, "%d.%d", version, revision );
			fclose( f );
		}
		else
			fprintf( stderr, "REV" );
	}

	if( args.libid )
	{
		char buffer[ 128 ];

		sprintf( buffer, "%d", version );
		setenv( "LIBVER", buffer, 1 );

		sprintf( buffer, "%d", revision );
		setenv( "LIBREV", buffer, 1 );

		sprintf( buffer, "%s %d.%d (%02d.%02d.%d)", (STRPTR) args.libid, version, revision,
			x[ 2 ], x[ 1 ], x[ 0 ] + 1900
		);
		setenv( "LIBID", buffer, 1 );
	}
	else if( changed || args.touch || (args.stouch && smarttouch(revinfoname, revhname)) || !exists( revhname ) || cmpdate( revinfoname, revhname ) <= 0 )
	{
		f = fopen( revhname, "w+" );
		if( !f )
		{
			fprintf( stderr, "REV" );
			exit( 10 );
		}

		if( compile )
		fprintf( f, "#define VERSIONSTRING \"%d.%d.%d\"\n#define VERSION %d\n#define REVISION %d\n#define COMPILEREV %d\n#define COMPILEYEARNUM %d\n#define COMPILEYEAR \"%d\"\n#define REVDATE \"(%02d.%02d.%04d)\"\n#define VERTAG \"%d.%d.%d (%d.%d.%d)\"\n#define LVERTAG \"%d.%d.%d (%d.%d.%d)\"\n\n#define VERHEXID 0x%x\n",
			version, revision, compile,
			version, revision, compile,
			x[ 0 ] + 1900,
			x[ 0 ] + 1900,
			x[ 2 ], x[ 1 ], x[ 0 ] + 1900,
			version, revision, compile,
			x[ 2 ], x[ 1 ], ( x[ 0 ] - 100 ),
			version, revision, compile,
			x[ 2 ], x[ 1 ], ( x[ 0 ] + 1900 ),
			( version << 24 ) | ( revision << 16 ) | compile
		);
		else
		fprintf( f, "#define VERSIONSTRING \"%d.%d\"\n#define VERSION %d\n#define COMPILEREV 0\n#define COMPILEYEARNUM %d\n#define COMPILEYEAR \"%d\"\n#define REVISION %d\n#define REVDATE \"(%02d.%02d.%04d)\"\n#define VERTAG \"%d.%d (%d.%d.%d)\"\n#define LVERTAG \"%d.%d (%d.%d.%d)\"\n\n#define VERHEXID 0x%x\n",
			version, revision,
			version,
			x[ 0 ] + 1900,
			x[ 0 ] + 1900,
			revision,
			x[ 2 ], x[ 1 ], x[ 0 ] + 1900,
			version, revision,
			x[ 2 ], x[ 1 ], ( x[ 0 ] - 100 ),
			version, revision,
			x[ 2 ], x[ 1 ], ( x[ 0 ] + 1900 ),
			( version << 24 ) | ( revision << 16 ) | compile
		);
		fclose( f );
	}

	if( startdirp )
	{
		chdir( startdir );
	}

	return( 0 );
}
