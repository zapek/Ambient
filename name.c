/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2016 Ambient Open Source Team
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
 * $Id: name.c,v 1.19 2017/07/25 19:55:00 piru Exp $
 */

#include "ambient.h"

/* public */
#include <sys/param.h>
#include <dos/dos.h>
#include <proto/dos.h>

/* private */
#include "name.h"
#include "deficon_getpath.h"
#include "doslistcache.h"

ULONG name_isinfo(CONST_STRPTR name)
{
	ULONG len;

	ASSERT(name);

	/*  filters out filenames like ".info", it is not an icon file! 
	 */

	len = strlen(name);

	if (len >= 6)
	{
		UBYTE c;

		name += len - 6;
		c    =  name[0];

		if (((c = name[0]) != ':') && (c != '/') && 
		    (name[1] == '.') && 
		    (((c = name[2]) == 'i') || (c == 'I')) && 
		    (((c = name[3]) == 'n') || (c == 'N')) && 
		    (((c = name[4]) == 'f') || (c == 'F')) && 
		    (((c = name[5]) == 'o') || (c == 'O'))
		)
		{
			return (TRUE);
		}
	}
	return (FALSE);
}


STRPTR name_build(CONST_STRPTR name)
{
	int len = strlen(name) + 1;
	STRPTR n;

	ASSERT(name);

	if ( (n = malloc(len)) )
	{
		bcopy(name, n, len);
	}
	else
	{
		errormsg(ERR_NOMEM);
	}
	return (n);
}

/*
** name_buildchecksys()
**
** This function is basically identically to
** name_build().
**
** The main difference is that it replaces the
** device or volume name by "SYS:". This is
** handy when it comes to settings and system
** cloning.
**
** If you clone a system on a partition named
** "mini:" to a system partition named "peg2:"
** with normal name_build you will end up
** with an empty panel. Backdrop images are
** missing, ...
**
** Using this function prevents Ambient from
** using system specific device and volume
** names and allows easy system cloning and
** booting some 1:1 backup drive.
**
*/
STRPTR name_build_sysify(CONST_STRPTR name)
{
	STRPTR n, devend;

	ASSERT(name);

	if ( (n = malloc(strlen(name) + 1)) )
	{
		strcpy(n, name);

		if( ( devend = strchr( n, ':' ) ) )
		{
			char chr;
			ULONG devlength;
			BPTR l1, l2;
			#define NEWDEVICENAME "SYS:"
			#define NEWDEVICENAME_SIZEOF 4
			devend++; /* pointer on char behind ":" */
			chr       = devend[0];
			devend[0] = 0x00; /* terminate */
			devlength = strlen( n ); /* length of "device:" without termination */
			l1        = Lock( n, ACCESS_READ );
			devend[0] = chr; /* restore */

			if( l1 )
			{
				if( ( l2 = Lock( NEWDEVICENAME, ACCESS_READ ) ) )
				{
					if( ( SameLock( l1, l2 ) == LOCK_SAME ) ) /* both pointing to same object? */
					{
						if( devlength < NEWDEVICENAME_SIZEOF ) {
							free( n ); /* free old name clone */
							n = malloc( strlen( name ) - devlength + NEWDEVICENAME_SIZEOF + 1 ); /* realloc with new size */
						}
						if( n ) /* reallocation may fail */
						{
							strcpy( n                         , NEWDEVICENAME    ); /* save as we know sizes */
							strcpy( &n[ NEWDEVICENAME_SIZEOF ], &name[devlength] ); /* save as we know sizes */
	PDB(("NEW NAME is %s!\n", n ));
						}
					}
					UnLock( l2 );
				}
				UnLock( l1 );
			}
		}
	}
	if( !n )
	{
		errormsg(ERR_NOMEM);
	}
	return (n);
}


STRPTR name_build_noinfo(CONST_STRPTR name)
{
	ULONG len;
	STRPTR n;

	ASSERT(name);

	len = strlen(name);
	if (len > 5)
	{
		if (name[len - 5] == '.'
			&& (name[len - 4] == 'i' || name[len - 4] == 'I')
			&& (name[len - 3] == 'n' || name[len - 3] == 'N')
			&& (name[len - 2] == 'f' || name[len - 2] == 'F')
			&& (name[len - 1] == 'o' || name[len - 1] == 'O')
		)
		{
			len -= 5;

			if (len > 4)
			{
				if (name[len - 5] == ':'
					&& (name[len - 4] == 'd' || name[len - 4] == 'D')
					&& (name[len - 3] == 'i' || name[len - 3] == 'I')
					&& (name[len - 2] == 's' || name[len - 3] == 'S')
					&& (name[len - 1] == 'k' || name[len - 1] == 'K')
				)
				{
					len -= 4;
				}
			}
		}

	}
	len++;

	if ( (n = malloc(len)) )
	{
		stccpy(n, name, len);
	}
	else
	{
		errormsg(ERR_NOMEM);
	}
	return (n);
}

/*
 * These weird functions are here to save some memory allocations when we need
 * temporary names without .info suffix.
 */

APTR name_truncateinfo(STRPTR name)
{
	ULONG len;

	ASSERT(name);

	len = strlen(name);

	if (len > 5)
	{
		if (name[len - 5] == '.'
			&& (name[len - 4] == 'i' || name[len - 4] == 'I')
			&& (name[len - 3] == 'n' || name[len - 3] == 'N')
			&& (name[len - 2] == 'f' || name[len - 2] == 'F')
			&& (name[len - 1] == 'o' || name[len - 1] == 'O')
		)
		{
			name[ len - 5 ] = '\0';
			return name + len - 5;
		}

	}

	return NULL;
}

void name_restoreinfo( STRPTR name, APTR truncation )
{
	if ( name && truncation )
	{
		name[ (STRPTR)truncation - name ] = '.';
	}
}

STRPTR name_build_colon(CONST_STRPTR name)
{
	LONG len;
	STRPTR n;
	ASSERT(name);

	len = strlen(name);
	n = malloc(len + 1 + 1);

	if (n)
	{
		memcpy(n, name, len);
		n[len + 0] = ':';
		n[len + 1] = '\0';
	}

	return n;
}


STRPTR name_build_info(CONST_STRPTR name)
{
	ULONG len;
	STRPTR n;

	ASSERT(name);

	len = strlen(name);

	/* device? ... */
	if (len && name[len - 1] == ':')
	{
		BPTR l0;

		if ( !(n = malloc(len + 10)) )
		{
			return NULL;
		}

		memcpy(n, name, len);
		strcpy(n + len, "disk.info");

		/* No disk.info? */
		if (!(l0 = Lock(n, ACCESS_READ)))
		{
			struct dlcnode *dn;

			/* As a special case we check if it is device with a deficon. */
			if ( (dn = doslistcache_find_dlcdevice_by_volumename(name)) )
			{
				STRPTR n2;

				if ( (n2 = malloc(PATH_SIZE)) )
				{
					TEXT icon[32 + 13];
					BPTR l1;

					snprintf(icon, sizeof(icon), "def_%sdisk.info", dn->name);
					if (deficon_getpath(n2, PATH_SIZE, icon))
					{
						if ( (l1 = Lock(n2, ACCESS_READ)) )
						{
							free(n);
							n = n2;
							n2 = NULL;
							UnLock(l1);
						}
					}
					free(n2);
				}
			}
		}

		UnLock(l0);
	}
	else /* ... or normal path? */
	{
		if (!(len > 5
			&& name[len - 5] == '.'
			&& (name[len - 4] == 'i' || name[len - 4] == 'I')
			&& (name[len - 3] == 'n' || name[len - 3] == 'N')
			&& (name[len - 2] == 'f' || name[len - 2] == 'F')
			&& (name[len - 1] == 'o' || name[len - 1] == 'O')
		))
		{
			if ( (n = malloc(len + 6)) )
			{
				memcpy(n, name, len);
				strcpy(n + len, ".info");
			}
		}
		else
		{
			if ( (n = malloc(len + 1)) )
			{
				memcpy(n, name, len + 1);
			}
		}
	}

	if (!n)
	{
		errormsg(ERR_NOMEM);
	}
	return (n);
}


ULONG name_check_quoted(CONST_STRPTR name)
{
	ASSERT(name);

	if (name[0] == '\"')
	{
		ULONG len = strlen( name );

		if (len > 1 && name[len-1] == '\"')
		{
			return (TRUE);
		}
	}

	return (FALSE);
}


STRPTR name_build_quoted(CONST_STRPTR name)
{
	ULONG len;
	STRPTR n;

	ASSERT(name);

	len = strlen(name);

	if ( (n = malloc(len + 2 + 1)) )
	{
		/* if there's already a " in front, don't quote XXX: maybe we should scan the whole string ? */
		if (name[0] != '\"')
		{
			n[0] = '\"';
			strcpy(n + 1, name);
			n[len + 1] = '\"';
			n[len + 2] = '\0';
		}
		else
		{
			strcpy(n, name);
		}
	}
	else
	{
		errormsg(ERR_NOMEM);
	}
	return (n);
}


STRPTR name_build_unquoted(CONST_STRPTR name)
{
	ULONG len;
	STRPTR n;

	ASSERT(name);

	len = strlen(name);

	if ( (n = malloc(len + 1)) )
	{
		if (name[0] == '\"')
		{
			strcpy(n, &name[1]);
			n[len - 2] = '\0';
		}
		else
		{
			strcpy(n, &name[0]);
		}
	}
	else
	{
		errormsg(ERR_NOMEM);
	}
	return (n);
}


void name_delete(STRPTR name)
{
	ASSERT(name);
	free(name);
}


/*
 * If allocating new name succeded then deletes old one.
 */

STRPTR name_replace( STRPTR oldname, CONST_STRPTR name )
{
	STRPTR newname = NULL;

	if ( !name )
		return oldname;

	newname = name_build( name );

	if ( newname )
	{
		if ( oldname )
		{
			name_delete( oldname );
		}

		return newname;
	}
	else
	{
		errormsg(ERR_NOMEM);

		/*
		 * Try to rescue situation and return old name.
		 */

		return oldname;
	}
}

/*************/
/*
 *
 *
*/
#if 0
struct subdir
{
	WORD len;
	WORD deleted;
};

int name_build_wintitle(STRPTR wintitle, int title_length, STRPTR p)
{
	CONST_STRPTR s;
	STRPTR r;

	r = wintitle;
	s = strchr(p, ':');

	if (s)
	{
		int not_path = s[1] == '\0' ? 1 : 0;
		int len = ((size_t)s - (size_t)p) + (not_path ? 0 : 1); // Include ':' or not?

		stccpy(r, p, MIN(title_length, len + 1));

		if (not_path)
			return 1;

		title_length -= len;
		p += len;
		r += len;
	}

	if (title_length <= 4)
	{
		stccpy(r, "...", title_length);
	}
	else
	{
		int p_len = strlen(p);

		if (p_len > 0 && p[p_len - 1] == '/')
			p_len--;

		if (title_length > p_len)
		{
			stccpy(r, p, p_len + 1);
		}
		else
		{
			int subdirs = 1, tlen = p_len;
			STRPTR tmp;

			s = p;

			while ((tmp = strchr(s, '/')) && tmp[1] != '\0')
			{
				int len = (size_t)tmp - (size_t)s + 1;

				subdirs++;
				tmp++;

				tlen -= len;
				s = tmp;
			}

			if (tlen > title_length)
			{
				p += (p_len - title_length) + 4;
				snprintf(r, title_length, "...%s", p);
			}
			else
			{
				struct subdir *arr = AllocMem(sizeof(struct subdir) * subdirs, MEMF_ANY);

				if (arr)
				{
					int i = 0, left, right;

					tlen = p_len + 5;
					s = p;

					while ((tmp = strchr(s, '/')) && tmp[1] != '\0')
					{
						int len = (size_t)tmp - (size_t)s + 1;

						arr[i].len = len;
						arr[i].deleted = 0;

						p_len -= len;
						tmp++;
						i++;
						s = tmp;
					}

					arr[i].len = p_len;
					arr[i].deleted = 0;

					left = MAX(subdirs / 2, 1);
					right = MAX(subdirs / 2, 1);

					do
					{
						if (left > 0)
						{
							left--;
							tlen -= arr[left].len;
						}

						if (tlen > title_length && right <= subdirs - 2)
						{
							if (left == 0 || right < subdirs - 2)
							{
								tlen -= arr[right].len;
								right++;
							}
						}
					}
					while (tlen > title_length && left > 0);

					for (i = 0, tlen = 0; i < left; i++)
						tlen += arr[i].len;

					stccpy(r, p, tlen + 1);

					title_length -= tlen;
					r += tlen;
					p += tlen;

					for (i = left; i < right; i++)
						p += arr[i].len;

					for (tlen = 5, i = right; i < subdirs; i++)
						tlen += arr[i].len;

					snprintf(r, MIN(title_length, tlen), ".../%s", p);

					FreeMem(arr, sizeof(struct subdir) * subdirs);
				}
			}
		}
	}

	return 0;
}
#else
int name_build_wintitle(STRPTR wintitle, int titlen, STRPTR p)
{
	CONST_STRPTR q, s;
	STRPTR r;
	LONG restore_trailing = FALSE;
	LONG p_len;

	q = p;
	r = wintitle;

	if (p == NULL)
		*wintitle = '\0';

	while (*q != ':')
	{
		*r++ = *q++;
	}

	*r++ = ':';

	/* simple fix/workaround for trailing / */

	p_len = strlen( p );
	if (p_len && p[ p_len - 1 ] == '/' )
	{
		restore_trailing = TRUE;
		p_len--;
		p[ p_len ] = '\0';
	}

	s = q + 1;
	q = FilePart(p);

	if (*q)
	{
		if (q != s) /* add '..' only if the path has more than one subdir */
		{
			*r++ = '.';
			*r++ = '.';
		}

		while (*q)
		{
			*r++ = *q++;
		}
	}
	*r = '\0';

	if ( restore_trailing )
		p[ p_len ] = '/';

	return 1;
}
#endif

/*
 * Returns true if file matches given pattern.
 */

ULONG name_match(CONST_STRPTR name, CONST_STRPTR pattern)
{
	ULONG rc = FALSE;

	if (name && pattern)
	{
		TEXT statbuf[ 128 ];	/* To remove memory allocation in most cases */
		STRPTR dynbuf = NULL;
		STRPTR buf;
		ULONG len = strlen( pattern ) * 2 + 2;

		if ( len > sizeof( statbuf ) )
			buf = dynbuf = malloc( len );
		else
			buf = statbuf;

		if ( buf )
		{
			if (ParsePatternNoCase(pattern, buf, len) != -1)
			{
				if (MatchPatternNoCase(buf, (STRPTR) name))
				{
					rc = TRUE;
				}
			}
		}

		if ( dynbuf )
			free( dynbuf );
	}

	return (rc);
}

/*
 * Processes name (path) by quoting pattern characters.
 *
 * This is required for literal names for ParsePattern#?() and any
 * function using it (for example MatchFirst(), MatchNext()).
 */

STRPTR name_quotepattern(CONST_STRPTR path, STRPTR newpath, ULONG buflen)
{
	CONST_STRPTR a;
	STRPTR b;
	UBYTE c;

	ASSERT(path);

	/* already have buffer? */

	if ( !newpath || !buflen )
	{
		/* count specials to know amount of needed space */

		a = path;
		buflen = 1; /* space for terminating 0 */

		while ( ( c = *a++ ) )
		{
			buflen++;

			switch (c)
			{
				case '*':
				case '~':
				case '[':
				case ']':
				case '#':
				case '?':
				case '(':
				case ')':
				case '|':
					buflen++;
					break;
			}
		}

		newpath = malloc( buflen );
		if ( !newpath )
		{
			return NULL;
		}
	}

	a = path;
	b = newpath;

	while ( ( c = *a++ ) )
	{
		switch (c)
		{
			case '*':
			case '~':
			case '[':
			case ']':
			case '#':
			case '?':
			case '(':
			case ')':
			case '|':
				if ( --buflen == 0 )
					goto out;

				*b++ = '\'';
				break;
		}

		if ( --buflen == 0 )
			break;

		*b++ = c;
	}
	out:

	*b = '\0';

	return newpath;
}

/*
 * Build string quoted for ReadItem()/ReadArgs()
 *
 * If the string doesn't require quotes, they won't be added.
 *
 */
STRPTR name_build_readargs_quoted(CONST_STRPTR name, STRPTR newname, ULONG buflen)
{
	CONST_STRPTR a;
	ULONG newbuflen;
	STRPTR b;
	UBYTE c;
	ULONG addquotes;

	ASSERT(name);

	newbuflen = 1; /* space for terminating 0 */

	if ( *name == '\0' )
	{
		/* Empty string must be presented as "" */
		addquotes = 2;
	}
	else
	{
		a = name;
		addquotes = 0;

		while ( ( c = *a++ ) )
		{
			newbuflen++;

			switch (c)
			{
				case '\n':
				case '\e':
				case '*':
				case '\"':
					newbuflen++;
					/* fall thru */
				case ' ':
				case '\t':
				case '=': /* Must trigger quoting as it's used to separate keyword and argument - Piru */
					addquotes = 2;
					break;
			}
		}
	}

	/* already have buffer? */

	if ( !newname || !buflen )
	{
		buflen = newbuflen + addquotes;
		newname = malloc( buflen );
		if ( !newname )
		{
			return NULL;
		}
	}

	/* In case there is no quoting needed, just copy */

	if ( !addquotes )
	{
		stccpy( newname, name, buflen );

		return newname;
	}

	a = name;
	b = newname;

	if ( --buflen == 0 )
		goto out;
	*b++ = '"';

	while ( ( c = *a++ ) )
	{
		static const UBYTE carray[] = {'N', 0, 0, 0, 0, 0, 0, 0, 0, 0,
		                                 0, 0, 0, 0, 0, 0, 0, 'E'};

		switch (c)
		{
			case '\n':
			case '\e':
				c = carray[ c - '\n' ];
				/* fall thru */
			case '*':
			case '\"':
				if ( --buflen == 0 )
					goto out;

				*b++ = '*';
				break;
		}

		if ( --buflen == 0 )
			goto out;

		*b++ = c;
	}

	if ( --buflen == 0 )
		goto out;

	*b++ = '"';

	out:

	*b = '\0';

	return newname;
}


STRPTR name_shorten_ellipsis(CONST_STRPTR src, STRPTR dest, ULONG len, LONG style) 
{

#define ELLIPSIS_STR      "..."
#define ELLIPSIS_STR_LEN  (sizeof(ELLIPSIS_STR) - 1)

	/*
	  Actually, this function still lacks a few sanity checks,
	  like, what if len too short, and won't even the ellipsis fits?
	  The original idea was to take path features, like volume name
	  and slashes into account too, but it's missing for now.
	  So use with care, and it'll improve later
	 */

	int srclen = strlen(src);
	int templen;

	/*
	  if we have longer or equal dest available, than source, 
	  it's not needed to use an ellipsis
	 */
	if (srclen <= len)
	{
		stccpy(dest, src, len);
		return dest;
	}

	if (style <= NAME_ELLIPSIS_START) 
	{
		src += (srclen - MAX(len - ELLIPSIS_STR_LEN, 0));
		snprintf(dest, len, "%s%s", ELLIPSIS_STR, src);
		return dest;
	}

	if (style == NAME_ELLIPSIS_MIDDLE) 
	{
		templen = len/2;
		strncpy(dest, src, templen);
		stccpy(dest+len-templen, src+(srclen-templen), templen+1);
		strncpy(dest+templen-1, ELLIPSIS_STR, ELLIPSIS_STR_LEN);
		return dest;
	}

	if (style >= NAME_ELLIPSIS_END) 
	{
		stccpy(dest, src, len-ELLIPSIS_STR_LEN+1);
		strcpy(dest + (len-ELLIPSIS_STR_LEN), ELLIPSIS_STR);
		return dest;
	}

	return dest;
}
