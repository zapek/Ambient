/* GLIB - Library of useful routines for C programming
* Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the
* Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA 02111-1307, USA.
*/

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/. 
 */

#include "ambient.h"

#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/* private */

#include "strings.h"

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif
#endif

#define return_val_if_fail(expr,val)  {\
		if (expr)                        \
		{                                \
		}                                \
		else					         \
		{								 \
			DB(("#expr Failed!\n"));	  \
			return (val);				 \
		}                               \
	}

#define return_if_fail(expr)  {          \
		if (expr)                        \
		{                                \
		}                                \
		else					         \
		{								 \
			return;	      				 \
		}                               \
	}


#define MY_MAXSIZE ((unsigned int)-1)

static inline unsigned int nearest_power( unsigned int base, unsigned int num)
{
	if (num > MY_MAXSIZE / 2)
	{
		return MY_MAXSIZE;
	}
	else
	{
		unsigned int n = base;

		while (n < num)
			n <<= 1;

		return n;
	}
}

#define G_VA_COPY(ap1, ap2)	  (*(ap1) = *(ap2))

int vasprintf( char **string, const char *fmt, va_list args );

int vasprintf( char **string, const char *fmt, va_list args )
{
	int len;

	va_list args2;

	G_VA_COPY (args2, args);

	len = vsnprintf( NULL, 0 , fmt , args );

	if	( len >= 0 )
	{
		*string = malloc( len + 1 );
		if	( *string )
		{
			(*string)[ len ] = 0;

			vsnprintf( *string, len + 1, fmt , args2 );

		}
		else
		{
			len = -1;
		}
	}
	va_end( args2 );

	return len;
}


static void string_maybe_expand( String* string, int len )
{
	if ( string->len + len >= string->allocated_len )
	{
		char *newstr;
		string->allocated_len = nearest_power( 1, string->len + len + 1 );

		newstr = malloc( string->allocated_len );
		if ( !newstr )
		{
			PDB(("Not enough memory (%d requested)\n", string->allocated_len ));
			return;
		}

		memcpy( newstr, string->str, string->len );
		free( string->str );
		string->str = newstr;

	}
}

String* string_sized_new ( int dfl_size )
{
	String *string;
	
	string = malloc( sizeof( *string ) );
	if ( !string )
		return NULL;
	
	string->allocated_len = 0;
	
	string->len = 0;
	
	string->str = NULL;
	
	string_maybe_expand ( string, max ( dfl_size, 2 ) );
	
	string->str[ 0 ] = 0;
	
	return string;
}

String* string_new( const char *init )
{
	String * string;
	
	if ( init == NULL || *init == '\0' )
		string = string_sized_new( 2 );
	else
	{
		int len;
			
		len = strlen( init );
		string = string_sized_new( len + 2 );
			
		string_append_len( string, init, len );
	}
		
	return string;
}

String* string_new_len( const char *init, int len )
{
	String * string;
	
	if ( len < 0 )
		return string_new( init );
	else
	{
		string = string_sized_new( len );
			
		if ( init )
			string_append_len( string, init, len );
				
		return string;
	}
}

char *string_free( String *string, int return_c_string )
{
	char *c_string;

	if ( !string )
		return NULL;

	if ( !return_c_string )
	{
		free( string->str );
		c_string = NULL;
	}
	else
		c_string = string->str;
		
	free( string );

	return c_string;
}

int string_equal( const String *v, const String *v2 )
{
	char * p, *q;
	String *string1 = ( String * ) v;
	String *string2 = ( String * ) v2;
	int i = string1->len;
	
	if ( i != string2->len )
		return FALSE;
		
	p = string1->str;
	
	q = string2->str;
	
	while ( i )
	{
		if ( *p != *q )
			return FALSE;
				
		p++;
			
		q++;
			
		i--;
	}
		
	return TRUE;
}

String* string_assign( String *string, const char *rval )
{
	return_val_if_fail ( string != NULL, NULL );
	return_val_if_fail ( rval != NULL, string );
	
	/* Make sure assigning to itself doesn't corrupt the string.  */
	
	if ( string->str != rval )
	{
		/* Assigning from substring should be ok since string_truncate
		does not realloc.  */
		string_truncate ( string, 0 );
		string_append ( string, rval );
	}
		
	return string;
}

String* string_truncate( String *string, int len )
{
	return_val_if_fail ( string != NULL, NULL );
	
	string->len = min( len, string->len );
	string->str[ string->len ] = 0;
	
	return string;
}

/*
 * string_set_size:
 * @string: a #String
 * @len: the new length
 * 
 * Sets the length of a #String. If the length is less than
 * the current length, the string will be truncated. If the
 * length is greater than the current length, the contents
 * of the newly added area are undefined. (However, as
 * always, string->str[string->len] will be a nul byte.) 
 * 
 * Return value: @string
 */

String* string_set_size( String *string, int len )
{
	return_val_if_fail ( string != NULL, NULL );
	
	if ( len >= string->allocated_len )
		string_maybe_expand ( string, len - string->len );
		
	string->len = len;
	
	string->str[ len ] = 0;
	
	return string;
}

String* string_insert_len( String *string, int pos, const char *val, int len )
{
	return_val_if_fail ( string != NULL, NULL );
	return_val_if_fail ( val != NULL, string );
	
	if ( len < 0 )
		len = strlen ( val );
		
	if ( pos < 0 )
		pos = string->len;
	else
		return_val_if_fail ( pos <= string->len, string );
		
	/* Check whether val represents a substring of string.  This test
	   probably violates chapter and verse of the C standards, since
	   ">=" and "<=" are only valid when val really is a substring.
	   In practice, it will work on modern archs.  */
	if ( val >= string->str && val <= string->str + string->len )
	{
		int offset = val - string->str;
		int precount = 0;
			
		string_maybe_expand ( string, len );
		val = string->str + offset;
		/* At this point, val is valid again.  */
			
		/* Open up space where we are going to insert.  */
			
		if ( pos < string->len )
			memmove( string->str + pos + len, string->str + pos, string->len - pos );
				
		/* Move the source part before the gap, if any.  */
		if ( offset < pos )
		{
			precount = min( len, pos - offset );
			memcpy( string->str + pos, val, precount );
		}
				
		/* Move the source part after the gap, if any.  */
		if ( len > precount )
			memcpy( string->str + pos + precount,
					 val +     /* Already moved: */ precount +     /* Space opened up: */ len,
					 len - precount );
	}
	else
	{
		string_maybe_expand( string, len );
			
		/* If we aren't appending at the end, move a hunk
		 * of the old string to the end, opening up space
		 */
			
		if ( pos < string->len )
			memmove( string->str + pos + len, string->str + pos, string->len - pos );
				
		/* insert the new string */
		memcpy( string->str + pos, val, len );
	}
		
	string->len += len;
	
	string->str[ string->len ] = 0;
	
	return string;
}

String* string_append( String *string, const char *val )
{
	return_val_if_fail ( string != NULL, NULL );
	return_val_if_fail ( val != NULL, string );
	
	return string_insert_len ( string, -1, val, -1 );
}

String* string_append_len( String *string, const char *val, int len )
{
	return_val_if_fail ( string != NULL, NULL );
	return_val_if_fail ( val != NULL, string );
	
	return string_insert_len ( string, -1, val, len );
}

#undef string_append_c
String* string_append_c( String *string, char c )
{
	return_val_if_fail ( string != NULL, NULL );
	
	return string_insert_c( string, -1, c );
}

String* string_prepend( String *string, const char *val )
{
	return_val_if_fail( string != NULL, NULL );
	return_val_if_fail( val != NULL, string );
	
	return string_insert_len( string, 0, val, -1 );
}

String* string_prepend_len( String *string, const char *val, int len )
{
	return_val_if_fail( string != NULL, NULL );
	return_val_if_fail( val != NULL, string );
	
	return string_insert_len( string, 0, val, len );
}

String* string_prepend_c( String *string, char c )
{
	return_val_if_fail( string != NULL, NULL );
	
	return string_insert_c( string, 0, c );
}

String* string_insert( String *string, int pos, const char *val )
{
	return_val_if_fail( string != NULL, NULL );
	return_val_if_fail( val != NULL, string );
	
	if ( pos >= 0 )
		return_val_if_fail( pos <= string->len, string );
		
	return string_insert_len( string, pos, val, -1 );
}

String* string_insert_c( String *string, int pos, char c )
{
	return_val_if_fail( string != NULL, NULL );
	
	string_maybe_expand( string, 1 );
	
	if ( pos < 0 )
		pos = string->len;
	else
		return_val_if_fail( pos <= string->len, string );
		
	/* If not just an append, move the old stuff */
	if ( pos < string->len )
		memmove( string->str + pos + 1, string->str + pos, string->len - pos );
		
	string->str[ pos ] = c;
	
	string->len += 1;
	
	string->str[ string->len ] = 0;
	
	return string;
}

String* string_erase( String *string, int pos, int len )
{
	return_val_if_fail( string != NULL, NULL );
	return_val_if_fail( pos >= 0, string );
	return_val_if_fail( pos <= string->len, string );
	
	if ( len < 0 )
		len = string->len - pos;
	else
	{
		return_val_if_fail( pos + len <= string->len, string );
			
		if ( pos + len < string->len )
			memmove( string->str + pos, string->str + pos + len, string->len - ( pos + len ) );
	}
		
	string->len -= len;
	
	string->str[ string->len ] = 0;
	
	return string;
}


void string_printf( String *string, const char *fmt, ... )
{
    char *buffer;
	int length;

	va_list args;

	return_if_fail( string != NULL );
	return_if_fail( fmt != NULL );

	string_truncate( string, 0 );
	
	va_start( args, fmt );

	length = vasprintf( &buffer, fmt, args );
	if ( buffer )
	{
		string_append_len( string, buffer, length );
		free( buffer );
	}
	va_end( args );
}

void string_append_printf( String *string, const char *fmt, ... )
{
	char *buffer;
	int length;

	va_list args;
	
	return_if_fail( string != NULL );
	return_if_fail( fmt != NULL );

	va_start( args, fmt );
	length = vasprintf( &buffer, fmt, args );
	if ( buffer )
	{
		string_append_len( string, buffer, length );
		free( buffer );
	}
	va_end( args );
}


/* Avoid using errno for these
*/

extern long int __strtol_impl(const char *nptr, char **endptr, int base, int *errnoptr);
extern unsigned long int __strtoul_impl(const char *nptr, char **endptr, int base, int *errnoptr);
extern unsigned long long int __strtoull_impl(const char *nptr, char **endptr, int base, int *errnoptr);

long int strtol(const char *nptr, char **endptr, int base)
{
	int dummy;
	return __strtol_impl(nptr, endptr, base, &dummy);
}
unsigned long int strtoul(const char *nptr, char **endptr, int base)
{
	int dummy;
	return __strtoul_impl(nptr, endptr, base, &dummy);
}
unsigned long long int strtoull(const char *nptr, char **endptr, int base)
{
	int dummy;
	return __strtoull_impl(nptr, endptr, base, &dummy);
}
