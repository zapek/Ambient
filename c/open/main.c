/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * © 2006 - 2008 Ambient Open Source Team
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
 * $Id: main.c,v 1.6 2017/07/29 16:26:49 piru Exp $
 */

#include <dos/rdargs.h>
#include <dos/dosasl.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/wb.h>
#include <proto/openurl.h>
#include <libraries/openurl.h>
#include <string.h>

ULONG __entrypoint(void);

#define APPNAME "Open"

const char __abox__[]
#if __GNUC__ > 2
__attribute__((used))
#endif
= "$VER: Open 50.8 (12.08.2008) © 2008 Ambient Open Source Team";

/*
 * DOS Template and related argument structure
*/

#define TEMPLATE "FILE/M"

struct options {
	STRPTR *name;
};

/*
 * some string length limits
*/

#define FULLNAME_SIZEOF 0x200
#define PM_NAME_SIZEOF 0x200

/*
 * These patterns are used to identify an URL.
*/

#define PREURL_PATTERN  "(www.|http://|https://)#?"
#define POSTURL_PATTERN "#?(.org|.com|.net|.edu|.gov|.ru|.ch|.ga|.de|.fr|.uk|.se|.nu|.dk|.hu)"

#define PATTERNSIZE ( (sizeof( POSTURL_PATTERN )*2) + 2 ) /* Bantam AmigaDOS Manual 3rd Edition page 267: size * 2 + 2 is enough */

ULONG __entrypoint(void)
{
	struct ExecBase *SysBase = *(struct ExecBase **)4UL;
	struct Library  *DOSBase;
	struct Library  *WorkbenchBase;
	LONG rc = RETURN_ERROR;
	struct Process *process;
	APTR oldwinptr;

	process = (APTR) FindTask(NULL);
	oldwinptr = process->pr_WindowPtr; /* avoid requester popup, when getting urls */
	process->pr_WindowPtr = (APTR) -1;

	if ((DOSBase = OpenLibrary("dos.library", 50UL)))
	{
		if ((WorkbenchBase = OpenLibrary("workbench.library", 44UL)))
		{
			struct RDArgs *rda;
			struct options opt;

			memset(&opt, 0, sizeof(opt));

			if ((rda = ReadArgs(TEMPLATE, (LONG *) &opt, NULL)))
			{
				struct AnchorPath *ap;

				if( (ap = AllocTaskPooled( sizeof( struct AnchorPath ) + PM_NAME_SIZEOF ) ))
				{
					STRPTR name;
					STRPTR fakename[2] = { "", NULL }; /* setup a fake /M argument */
					if( !opt.name ) {
						opt.name = fakename; /* no name argument present, so fake one */
					}

					while( (name = *opt.name++) )
					{
						memset( ap, 0, sizeof( struct AnchorPath ) );
						ap->ap_Strlen = PM_NAME_SIZEOF;

						if( !MatchFirst( name, ap ) )
						{
							BPTR lock;
							do
							{
								if( (lock = Lock( &ap->ap_Buf[0], ACCESS_READ)) )
								{
									char fullname[ FULLNAME_SIZEOF ];

									if( (NameFromLock(lock, fullname, FULLNAME_SIZEOF)) )
									{
										if( OpenWorkbenchObjectA(fullname, NULL) )
										{
											rc = RETURN_OK;
										}
									}
									UnLock( lock );
								}
							} while ( !MatchNext( ap ) );
						}
						else
						{
							char pattern[ PATTERNSIZE ];
							BOOL isurl = TRUE;

							ParsePatternNoCase( PREURL_PATTERN, pattern, PATTERNSIZE );
							if( !MatchPatternNoCase( pattern, name ) ) {
								ParsePatternNoCase( POSTURL_PATTERN, pattern, PATTERNSIZE );
								if( !MatchPatternNoCase( pattern, name ) ) {
									isurl = FALSE;
								}
							}

							if( isurl )
							{
								struct Library *OpenURLBase;
								if( (OpenURLBase = OpenLibrary("openurl.library", 0L )))
								{
									rc = RETURN_OK;
									URL_OpenA( name, TAG_DONE );
									CloseLibrary( OpenURLBase );
								}
							}
						}
						MatchEnd( ap );

					}
					FreeTaskPooled( ap, sizeof( struct AnchorPath ) + PM_NAME_SIZEOF );
				}
				FreeArgs(rda);
			}
			CloseLibrary( WorkbenchBase );
		}
		if ( rc )
		{
			PrintFault(IoErr(), APPNAME );
		}

		CloseLibrary( DOSBase );
	}
	process->pr_WindowPtr = oldwinptr;

	return (rc);
}
