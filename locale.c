/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2016 Ambient Open Source Team
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
 * $Id: locale.c,v 1.11 2026/05/15 12:15:50 geit Exp $
 */

#include "ambient.h"

/* public */
#include <libraries/locale.h>
#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/locale.h>
#include <proto/utility.h>

/* private */
#include "locale.h"
#include "ambient_cat.h"

/***************************************************************************************/

static struct Catalog *catalog;

struct Locale *locale;

LONG   locale_timezone_offset;

ULONG locale_init(void)
{
	/* Safe with NULL argument */

	locale = OpenLocale(NULL);

	locale_timezone_offset = -locale->loc_GMTOffset * 60;

	catalog = OpenCatalog(
		NULL, "Ambient.catalog",
		OC_BuiltInLanguage, "english",
		OC_Version, 7,  /* required catalog version */
		TAG_DONE
	);

	if (catalog)
	{
		int c;
		for (c = 0; c < MSG_RECOGNITION_DB_START ; c++)
		{
			((char**)__stringtable)[ c ] = GetCatalogStr(catalog, c, (char*)__stringtable[c]);
		}
	}
	return (TRUE);
}

void locale_cleanup(void)
{
	/* Both safe with NULL arguments */
	CloseLocale(locale);
	CloseCatalog(catalog);
}
/***************************************************************************************/

#if USE_RECOGTRANSLATION

/* locale_translationget()
**
*/
char *locale_translationget( char *english )
{
	int c, l;

	for( l = 0 ; english[l] && ( english[l] != '\n' ) ; l++ ) {}

	if( l ) {
		for( c = MSG_RECOGNITION_DB_START ; c < NUMCATSTRING ; c++ ) {
			if( !strncmp( ((char**)__stringtable)[ c ], english, l ) ) {
				if( strlen( ((char**)__stringtable)[ c ] ) == l ) {
					return( GetCatalogStr( catalog, c, (char*)__stringtable[c] ) );
				}
			}
		}
	}
	return( english );
}

/* locale_translatiogetenglish()
*/

char *locale_translationgetenglish( char *str )
{
	int c;

	for( c = MSG_RECOGNITION_DB_START ; c < NUMCATSTRING ; c++ ) {
		/* compare against the system language */
		if( !Stricmp( GetCatalogStr( catalog, c, (char*)__stringtable[c] ), str ) ) {
			return(  (char*)__stringtable[c] );
		}
	}
/* if english is not found this is a user string. */
	return( str );
}

/* locale_translationfillpattern()
**
** searches for any "${}" pattern, finds translation and writes it back without
** the "${}". This is a one way ticket, so only use this function on disposable
** texts like on DOS Execute or so
**
*/

char *locale_translationfillpattern( char *oldstr, int oldstrlen )
{
	int i, l, c, found;
	char *buffer, *dst, chr;


	if( ( buffer = AllocVecTaskPooled( oldstrlen + 1 ) ) ) {
		dst = buffer;
		for( i = 0 ; ( oldstrlen > 0 ) && ( chr = oldstr[ i ] ) ; i++ ) {
			if( ( ( chr = oldstr[ i ] )  == '$' ) && ( oldstr[ i + 1 ] == '{' ) )  {
				i += 2;   /* skip "${" */
				for( l = 0 ; ( oldstr[ i + l ] && ( oldstr[ i + l ] != '}' ) ) ; l++ ) {}   /* calculate length */
				/* find translation */
				found = 0;
				for( c = MSG_RECOGNITION_DB_START ; c < NUMCATSTRING ; c++ ) {
					if( !strncmp( ((char**)__stringtable)[ c ], &oldstr[ i ], l ) ) {
						if( strlen( ((char**)__stringtable)[ c ] ) == l ) {
							char *newstr = GetCatalogStr( catalog, c, (char*)__stringtable[c] );
							int newl = strlen( newstr );
							if( ( newl < oldstrlen ) ) {   /* does it fit ? */
								strcpy( dst, newstr );
								dst = &dst[ newl ];   /* skip behind inserted string */
								oldstrlen -= newl;   /* reduce free space */
								found++;
							}
							break;
						}
					}
				}
				if( !found ) {   /* we insert the contents of the pattern */
					for( c = 0 ; ( c < l ) && ( oldstrlen > 0 ) ; c++ ) {
						*dst++ = oldstr[ i + c ];
						oldstrlen--;
					}
				}
				/* complete pattern was dealed with, so now skip behind string. */
				i += l;
				if(  oldstr[ i + l ] == '}' ) {   /* only skip behind "}², if it is there */
					i++;   /* skip behind "}" */
				}
			} else {
				*dst++ = chr;
				oldstrlen--;
			}
		}
		if( oldstrlen > 0 ) {
			*dst++ = 0x00;   /* terminated buffered string */
			strcpy( oldstr, buffer );
		}
		FreeVecTaskPooled( buffer );
	}
/* if english is not found this is a user string. */
	return( oldstr );
}
#endif

/***************************************************************************************/

static void PutCharFunc(struct Hook *h, UNUSED struct Locale *l, TEXT c)
{
	STRPTR p = h->h_Data;
	*p++ = c;
	h->h_Data = p;
}

void CreateDateString(CONST_STRPTR template, struct DateStamp *ds, STRPTR buf)
{
	struct Hook h;

	h.h_Entry = (HOOKFUNC)HookEntry;
	h.h_SubEntry = (HOOKFUNC)PutCharFunc,
	h.h_Data = buf;

	FormatDate(locale, template, ds, &h);
}

static size_t GetCharFunc(struct Hook *h)
{
	STRPTR p = h->h_Data;
	TEXT c = *p++;

	h->h_Data = p;
	return c;
}

BOOL ParseDateString(CONST_STRPTR template, struct DateStamp *ds, CONST_STRPTR datestr)
{
	struct Hook h;

	h.h_Entry = (HOOKFUNC)HookEntry;
	h.h_SubEntry = (HOOKFUNC)GetCharFunc,
	h.h_Data = (APTR)datestr;

	return ParseDate(locale, ds, template, &h);
}

