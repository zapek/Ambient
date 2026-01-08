/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2007 Ambient Open Source Team
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
 * $Id: mimeuri.c,v 1.15 2017/08/21 06:17:44 cyfm Exp $
 */

#include "ambient.h"

/* public */
#include <proto/locale.h>

/* private */
#include "locale.h"
#include "mimeuri.h"
#include "doslistcache.h"
#include "vfs.h"
#include "file_func.h"
#include "rexx.h"
#include "mimetype.h"
#include "mui_func.h" /* FORTAG.. */
#include "str.h"


struct mimeuri_context {
	STRPTR uri;
	STRPTR scheme;
	STRPTR host; /* aka 'authority' */
	STRPTR username;
	STRPTR password;
	STRPTR path;
	STRPTR args;
	STRPTR fragment;
	ULONG port;
	ULONG islocal;
	/* mime stuff */
	APTR mctx;
	APTR rda;
};

/*
 * rfc to read: 2396
 */

APTR mimeuri_create(void)
{
	struct mimeuri_context *ct;

	if ( (ct = malloc(sizeof(*ct))) )
	{
		memset(ct, 0, sizeof(*ct));
	}
	return (ct);
}


static void mimeuri_clear(struct mimeuri_context *ct)
{
	/* XXX: check order, and how to transfer an URI to absolute if we free it eh ? */

	if (ct->mctx)
	{
		mimetype_delete(ct->mctx);
		ct->mctx = NULL;
	}

	if (ct->fragment)
	{
		free(ct->fragment);
		ct->fragment = NULL;
	}

	if (ct->args)
	{
		free(ct->args);
		ct->args = NULL;
	}

	if (ct->path)
	{
		free(ct->path);
		ct->path = NULL;
	}

	if (ct->password)
	{
		free(ct->password);
		ct->password = NULL;
	}

	if (ct->username)
	{
		free(ct->username);
		ct->username = NULL;
	}

	if (ct->host)
	{
		free(ct->host);
		ct->host = NULL;
	}

	if (ct->scheme)
	{
		free(ct->scheme);
		ct->scheme = NULL;
	}

	if (ct->uri)
	{
		free(ct->uri);
		ct->uri = NULL;
	}

	ct->port = 0;
	ct->islocal = FALSE;
}


void mimeuri_delete(APTR ctx)
{
	struct mimeuri_context *ct = ctx;

	ASSERT(ct);

	if (ct->rda)
	{
		freeargsstring((struct RDArgs *)ct->rda);
	}
	mimeuri_clear(ct);
	free(ct);
}


/*
 * Finds a scheme. Schemes are like:
 * foo:bar where 'foo' is the scheme. The
 * rule is they're all alpha chars and the last char
 * of the scheme is ':'. Returns the length of the
 * scheme or 0 if not found. Does not include ':' within
 * the length.
 * Warnings:
 * ---------
 * There's a side effect here. For an Amiga path like dh1:foo/bar
 * the scheme returned would be 'dh1' but is then corrected later
 * when the URI is found to be local so replaced by file:///
 * For this one to work, also non-alpha chars have to be supported
 * This may not be best solution, but should work.
 * Notes:
 * ---------
 * It had problems with uris like "name with spaces:/foo"
 * There are 2 possible solutions. Pass device name like
 * DH1 instead, or allow spaces in the URI. For now i made
 * it to acept spaces in names too. Should be discussed.
 * ---------
 * It sucks even more. DHx: names can also contain any weird
 * chars, so it's not a solution. To make stuff working i
 * added more non-alpha chars to matching condition.
 * It breakes original way this one should work tho, but
 * can't really think about any other same solution.
 *
 * XXX: what happens with a ":foobar" URI ?
 */

static ULONG mimeuri_find_scheme(CONST_STRPTR uri)
{
	ULONG len = 0;

	  while (IsAlNum(locale,*uri)
			|| *uri == ' '
			|| *uri == '_'
			|| *uri == '+'
			|| *uri == '-'
			|| *uri == '/'
			|| *uri == '\\'
			|| *uri == '.'
			|| *uri == ','
			|| *uri == '@'
            || *uri == '&'
   			|| *uri == '~'
            || *uri == '|'
            || *uri == '('
			|| *uri == ')'
            || *uri == '['
			|| *uri == ']'
            || *uri == '{'
            || *uri == '}'
			 )
	{
		uri++;
		len++;

		if (*uri == ':')
		{
			return (len);
		}

	}
	return (0);
}


/*
 * Returns a pointer to the local part, or NULL.
 */
static CONST_STRPTR mimeuri_islocal(CONST_STRPTR uri, ULONG pos)
{
	struct dlcnode *n;
	CONST_STRPTR p;

	if (!strncmp("file:", uri, 5))
	{
		D(MIMEURI,bug("file: scheme detected -> local\n"));

		/*
		 * file:, file:/ file:// and file:/// are all
		 * valid, sigh.. strip them.
		 */
		p = uri + 5;
		while (*p && *p == '/') p++;

		return (p);
	}
	if (!strncmp("devices:", uri, 8))
	{
		D(MIMEURI,bug("devices: scheme detected -> local\n"));

		/*
		 * file:, file:/ file:// and file:/// are all
		 * valid, sigh.. strip them.
		 */
		p = uri + 8;
		while (*p && *p == '/') p++;

		return (p);
	}

	if (!strncmp("vfs:", uri, 4))
	{
		D(MIMEURI,bug("vfs: scheme detected -> local\n"));

		/*
		 * file:, file:/ file:// and file:/// are all
		 * valid, sigh.. strip them.
		 */
		p = uri + 4;
		while (*p && *p == '/') p++;

		return (p);
	}
	/*
	 * If it's foo:// but not file:// then it's not
	 * a local path.
	 */
	if (uri[pos + 1] == '/' && uri[pos + 2] == '/')
	{
		D(MIMEURI,bug("// after scheme -> not local\n"));
		return (NULL);
	}

	/*
	 * Otherwise scan and if foo: exists as a local filesystem,
	 * then it's probably a local path. I hope none is stupid
	 * enough to use some volume called mailto: or news:
	 */

    /* Don't look vfs entries in doslist, since they won't appear */
	if(vfs_is_vfs_device(uri)) 
	{
		D(MIMEURI,bug("matches local fscontext device <%s> -> local\n", uri));
		return(uri);
	}

	ITERATEDLC(n)
	{
		if (n->is_fs)
		{
			if (!strnicmp(uri, n->name, pos) && !n->name[pos])
			{
				D(MIMEURI,bug("matches local device <%s> -> local\n", n->name));
				return (uri);
			}
		}
	}
	D(MIMEURI,bug("not a local uri\n"));
	return (NULL);
}


/*
 * Returns the length of the URI without NULL
 * terminator and not accounting possible
 * spaces at the end.
 */
#if 0 /* this is not used, so I disabled it */
static ULONG mimeuri_untailspaces(CONST_STRPTR uri)
{
	ULONG len;

	len = strlen(uri);

	if (len)
		while (uri[len - 1] == ' ') len--;

	return (len);
}


/*
 * Converts %20, etc..
 */
static void mimeuri_decode(STRPTR uri)
{
	TEXT hex[4];
	STRPTR p;
	LONG v;

	p = uri;

	while ( (p = strchr(p, '%')) )
	{
		stccpy(hex, p + 1, 3);
		stch_l(hex, &v);

		*p = (TEXT)v;
		strcpy(p + 1, p + 3);
		p++;
	}
}
#endif

/*
 * Only MTF_URIONLY is callable from the main task.
 * XXX: we could cleanup by having some more macros, maybe merging some functions, etc..
 */

ULONG v_mimeuri_gather(APTR ctx, CONST_STRPTR uri, struct TagItem *tags)
{
	ULONG cnt, c;
	struct mimeuri_context *ct = ctx;
	ULONG flags;

	THREAD;
	ASSERT(ct);
	ASSERT(uri);

	/* defaults */
	flags = (MTF_URI | MTF_EXTENSION | MTF_PROTOCOL | MTF_FILEIO | MTF_NETWORK);

	FORTAG(tags)
	{
		case MIMEURIGATHERTAG_URI:
			if (!tag->ti_Data)
			{
				flags &= ~MTF_URI;
			}
			break;

		case MIMEURIGATHERTAG_Extension:
			if (!tag->ti_Data)
			{
				flags &= ~MTF_EXTENSION;
			}
			break;

		case MIMEURIGATHERTAG_Protocol:
			if (!tag->ti_Data)
			{
				flags &= ~MTF_PROTOCOL;
			}
			break;

		case MIMEURIGATHERTAG_FileIO:
			if (!tag->ti_Data)
			{
				flags &= ~MTF_FILEIO;
			}
			break;

		case MIMEURIGATHERTAG_Recurse:
			if (tag->ti_Data)
			{
				flags |= MTF_RECURSE;
			}
			break;

		case MIMEURIGATHERTAG_Network:
			if (!tag->ti_Data)
			{
				flags &= ~MTF_NETWORK;
			}
			break;

		#ifdef DEBUG
		default:
			PDB(("unknown tag 0x%lx\n", tag->ti_Tag));
			break;
		#endif
	}
	NEXTTAG

	uri = stpblk(uri); /* skip spaces in front */

	/*
	 * There is *NO* heuristic detection of URIs in there.
	 * Stuff like www.foo.com -> http://www.foo.com/ are
	 * done on a higher level. URIs without scheme
	 * are relative.
	 */

	/* XXX: detect URIs like "" ? what to do with them ? */

	/* XXX: '//' means a host is specified */

	if ( (cnt = mimeuri_find_scheme(uri)) )
	{
		CONST_STRPTR p, q;
		CONST_STRPTR r = r; /* shut up gcc */
		CONST_STRPTR s = s; /* shut up gcc */
		ULONG len;

		D(MIMEURI,bug("absolute URI <%s>\n", uri));
		mimeuri_clear(ct);

		/* XXX: Trailing spaces should probably be encoded instead of being removed,
		 * at least for file:// scheme. For now, as there's only file:// scheme, just ignore that.
		 */
		/* len = mimeuri_untailspaces(uri); */
		len = strlen(uri) + 1;

		if ( (ct->uri = malloc(len)) == NULL)
		{
			return (FALSE);
		}
		stccpy(ct->uri, uri, len); /* XXX: actually the URL/URI should be cleaned up ! (%20 -> ' ', ../ -> prevpath, etc.. */
		/* XXX */

		/* XXX: MAKE SURE WE USE ct->uri EVERYTIME!! REMOVE THAT COMMENT ONCE EVERYTHING IS CHECKED */

		/*
		 * Now we need to find out if this
		 * is a local path 'SYS:foo/bar' or
		 * not.
		 */
		if ( (p = mimeuri_islocal(ct->uri, cnt)) )
		{
			ct->islocal = TRUE;

			if((STRPTR) strstr(ct->uri, "vfs") == ct->uri
			|| (STRPTR) strstr(ct->uri, "devices") == ct->uri)
			{
				if ( (ct->scheme = malloc(cnt + 1)) )
				{
					stccpy(ct->scheme, ct->uri, cnt + 1);
				}
			}
			else
			{
				if ( (ct->scheme = malloc(5)) )
				{
					strcpy(ct->scheme, "file");
				}
				/* XXX */
			}

			/*
			 * XXX : what if a filename contains '?' char ? Its path
			 *		 will be truncated and the rest will be considered
			 *		 as argument, as it is now
			 */

			if (!(q = strchr(p, '?')))
			{
				q = p + strlen(p);
			}
			if (q > p)
			{
				UBYTE decodedpath[q - p + 1];
				UBYTE resolved_path[PATH_SIZE*2];
				STRPTR path = NULL;
				LONG pathlen;
				struct dlcnode *dn;

				mimeuri_decodepath(decodedpath, sizeof(decodedpath), p, q - p);
						
				if(path_expand(decodedpath, resolved_path, sizeof(resolved_path)))
				{
					path = resolved_path;
				}
				else
				{
					path = decodedpath;
				}

				pathlen = strlen(path);

				//DB(("path: <%s>\n", path));

				doslistcache_lock();

				/*
				 * We try to convert to a volume name so that
				 * we avoid cases where the path is the same
				 * but Ambient can't find out.
				 */
				if ( (dn = doslistcache_find_dlcvolume_by_devicename(path)) )
				{
					CONST_STRPTR p1;
					ULONG devnamelen;

					devnamelen = strlen(dn->name);
					p1 = strchr(path, ':');

					if ( (ct->path = malloc(path + pathlen - p1 + 1 + devnamelen)) ) /* the '\0' is in p1 */
					{
						stccpy(ct->path, dn->name, devnamelen + 1);
						ct->path[devnamelen] = ':';
						stccpy(&ct->path[devnamelen + 1], p1 + 1, path + pathlen - p1);
					}
					/* XXX */
				}

				doslistcache_unlock();

				if (!ct->path)
				{
					/*
					 * It was a volume to begin with, so
					 * we just copy.
					 */
					if ( (ct->path = malloc(pathlen + 1)) )
					{
						strcpy(ct->path, path);
					}
					/* XXX */
				}
			}

			p = q + 1;

			if (*p && (*q == '?'))
			{
				/* do we have args ? */
				if (!(q = strchr(p, '#')))
				{
					q = p + strlen(p);
				}
				if (q > p)
				{
					if ( (ct->args = malloc(q - p + 1)) )
					{
						stccpy(ct->args, p, q - p + 1);
					}
					/* XXX */
				}

				p = q + 1;

				if (*p && (*q == '#'))
				{
					/* do we have a fragment ? */
					q = p + strlen(p);

					if (q > p)
					{
						if ( (ct->fragment = malloc(q - p + 1)) )
						{
							stccpy(ct->fragment, p, q - p + 1);
						}
						/* XXX */
					}
				}
			}
		}
		else
		{
			/*
			 * Is there a host?
			 */
			p = ct->uri + cnt + 1;
			c = 0;

			/*
			 * Has to be "//" or not "/" only.
			 */
			while (*p == '/')
			{
				c++;
				p++;
			}

			/* XXX: what about "///" ? */

			if (c == 2 || !c)
			{
				if (!(q = strpbrk(p, "/?#")))
				{
					q = p + strlen(p);
				}

				if (q > p)
				{
					/* we have a host */

					if ((r = strchr(p, '@')) && r < q)
					{
						/* we have a username */

						if ((s = strchr(p, ':')) && s < r)
						{
							/* we have a password */
							if ( (ct->password = malloc(r - s)) )
							{
								stccpy(ct->password, s + 1, r - s);
								D(MIMEURI,bug("password: <%s>\n", ct->password));
							}
							/* XXX */
						}

						if (!ct->password)
						{
							s = r;
						}

						if ( (ct->username = malloc(s - p + 1)) )
						{
							stccpy(ct->username, p, s - p + 1);
							D(MIMEURI,bug("username: <%s>\n", ct->username));
						}
						/* XXX */

						p = r + 1;
					}

					/* do we have a port ? */
					if ((r = strchr(p, ':')) && r < q)
					{
						ULONG port;

						port = atoi(r + 1);

						if (port > 0 && port < 65536)
						{
							ct->port = port;
						}
					}
					else
					{
						r = q;
					}

					if ( (ct->host = malloc(r - p + 1)) )
					{
						stccpy(ct->host, p, r - p + 1);
						D(MIMEURI,bug("host: <%s>\n", ct->host));
					}
					/* XXX */

					p = q + 1;

					if (*p && (*q == '/'))
					{
						/* do we have a path ? */
						if (!(q = strpbrk(p, "?#")))
						{
							q = p + strlen(p);
						}
						if (q > p)
						{
							if ( (ct->path = malloc(q - p + 1)) )
							{
								mimeuri_decodepath(ct->path, q - p + 1, p, q - p);
								//DB(("ct->path: <%s>\n", ct->path));
							}
						}
					}

					p = q + 1;

					if (*p && (*q == '?'))
					{
						/* do we have args ? */
						if (!(q = strchr(p, '#')))
						{
							q = p + strlen(p);
						}
						if (q > p)
						{
							if ( (ct->args = malloc(q - p + 1)) )
							{
								stccpy(ct->args, p, q - p + 1);
							}
							/* XXX */
						}
					}

					p = q + 1;

					if (*p && (*q == '#'))
					{
						/* do we have a fragment ? */
						q = p + strlen(p);

						if (q > p)
						{
							if ( (ct->fragment = malloc(q - p + 1)) )
							{
								stccpy(ct->fragment, p, q - p + 1);
							}
							/* XXX */
						}
					}
				}
				else
				{
					/* no path, XXX: what to do ? */
				}
			}

			if ( (ct->scheme = malloc(cnt + 1)) )
			{
				stccpy(ct->scheme, ct->uri, cnt + 1);
				D(MIMEURI,bug("scheme: <%s>\n", ct->scheme));
			}
			/* XXX */
		}
	}
	else
	{
		/*
		 * If that's a relative URI, we need
		 * to merge it.
		 */
		PDB(("NYI for uri <%s>\n", uri));
	}

	/*
	   XXX: relatives don't start with a scheme, host is optional, . and .. have special meanings.
	   thoe starting with // are net references (rare), starting with / are absolute path refs and
	   no scheme or nor slash is a relative path ref. there are some rules to simplify the path as well
	   '.', '..', etc..
	*/

	/*
	 * XXX: also convert the ascii chars (%20, etc..)
	 */

	//if (ct->uri &&

	/*
	 * Find out the mimetype.
	 */
	if (cnt && flags & (MTF_EXTENSION | MTF_PROTOCOL | MTF_FILEIO))
	{
		ct->mctx = mimetype_create(ct->scheme, ct->path, flags);
		//D(MIMEURI,bug("mimetype created successfully for scheme <%s> and path <%s> -> mimetype <%s>, action: %ld\n", ct->scheme, ct->path, ct->mt->mimetype, ct->mt->action));

		ASSERT(ct->mctx); /* XXX: ahem.. */
	}

	return (TRUE); /* XXX: for now.. */
}


APTR mimeuri_getattr(APTR ctx, ULONG attr)
{
	struct mimeuri_context *ct = ctx;

	ASSERT(ct);

	switch (attr)
	{
		case MIMEURIATTR_URI:
			return (ct->uri);

		case MIMEURIATTR_SCHEME:
			return (ct->scheme);

		case MIMEURIATTR_HOST:
			return (ct->host);

		case MIMEURIATTR_PORT:
			return ((APTR)ct->port);

		case MIMEURIATTR_USERNAME:
			return (ct->username);

		case MIMEURIATTR_PASSWORD:
			return (ct->password);

		case MIMEURIATTR_PATH:
			return (ct->path);

		case MIMEURIATTR_ARGS:
			return (ct->args);

		case MIMEURIATTR_FRAGMENT:
			return (ct->fragment);

		case MIMEURIATTR_LOCAL:
			return ((APTR)ct->islocal);

		case MIMEURIATTR_MIMETYPE:
			if (ct->mctx)
			{
				return (mimetype_getattr(ct->mctx, MIMETYPETAG_MIME));
			}
			return (NULL);

		case MIMEURIATTR_MIME_ACTION:
			if (ct->mctx)
			{
				return (mimetype_getattr(ct->mctx, MIMETYPETAG_ACTION));
			}
			return (NULL);

		case MIMEURIATTR_MIME_NEWWIN:
			if (ct->mctx)
			{
				return (mimetype_getattr(ct->mctx, MIMETYPETAG_NEWWIN));
			}
			return (NULL);

		case MIMEURIATTR_MIME_SUBTYPE:
			if (ct->mctx)
			{
				return (mimetype_getattr(ct->mctx, MIMETYPETAG_SUBTYPE));
			}
			return (NULL);

		case MIMEURIATTR_MIME_DESCRIPTION:
			if (ct->mctx)
			{
				return (mimetype_getattr(ct->mctx, MIMETYPETAG_DESCRIPTION));
			}
			return (NULL);

		case MIMEURIATTR_MIME_FILEINFO:
			if (ct->mctx)
			{
				return (mimetype_getattr(ct->mctx, MIMETYPETAG_FILEINFO));
			}
			return (NULL);

		case MIMEURIATTR_MIME_SECONDS:
			if (ct->mctx)
			{
				return (mimetype_getattr(ct->mctx, MIMETYPETAG_SECONDS));
			}
			return (NULL);

		case MIMEURIATTR_MIME_FILESIZE:
			if (ct->mctx)
			{
				return (mimetype_getattr(ct->mctx, MIMETYPETAG_FILESIZE));
			}
			return (NULL);

		case MIMEURIATTR_MIME_FILESIZEPTR:
			if (ct->mctx)
			{
				return (mimetype_getattr(ct->mctx, MIMETYPETAG_FILESIZEPTR));
			}
			return (NULL);

		#ifdef DEBUG
		default:
			PDB(("wrong attribute %lu\n", attr));
			break;
		#endif
	}
	return (NULL);
}


void v_mimeuri_setattrs(APTR ctx, struct TagItem *tags)
{
	struct mimeuri_context *ct = ctx;

	ASSERT(ct);

	FORTAG(tags)
	{
		case MIMEURIATTR_PATH:
			{
				STRPTR p; /* so it's possible to mimeuri_setattrs(ctx, MIMEURIATTR_PATH, mimeuri_getattr(ctx, MIMEURIATTR_PATH), TAG_DONE); */

				if ( (p = malloc(strlen((STRPTR)tag->ti_Data) + 1)) )
				{
					strcpy(p, (STRPTR)tag->ti_Data);

					if (ct->path)
					{
						free(ct->path);
					}
					ct->path = p;
				}
				/* XXX */
			}
			break;

		case MIMEURIATTR_ARGS:	/* XXX: URI will be invalid then, but that shouldn't matter for what we need it */
			{
				STRPTR a = malloc( strlen( (STRPTR)tag->ti_Data ) + 1 );

				if ( a )
				{
					strcpy(a, (STRPTR)tag->ti_Data);

					if (ct->args)
					{
						free(ct->args);
					}
					ct->args = a;
				}
			}
			break;

		case MIMEURIATTR_SCHEME:
			{
				STRPTR p; 

				if ( (p = malloc(strlen((STRPTR)tag->ti_Data) + 1)) )
				{
					strcpy(p, (STRPTR)tag->ti_Data);

					if (ct->scheme)
					{
						free(ct->scheme);
					}
					ct->scheme = p;
				}
				/* XXX */
			}
			break;

		case MIMEURIATTR_MIMETYPE:
			if (ct->mctx)
			{
				mimetype_setattrs(ct->mctx,
					MIMETYPETAG_MIME, tag->ti_Data,
				TAG_DONE);
			}
			break;

		case MIMEURIATTR_MIME_ACTION:
			if (ct->mctx)
			{
				mimetype_setattrs(ct->mctx,
					MIMETYPETAG_ACTION, tag->ti_Data,
				TAG_DONE);
			}
			break;

		case MIMEURIATTR_MIME_NEWWIN:
			if (ct->mctx)
			{
				mimetype_setattrs(ct->mctx,
					MIMETYPETAG_NEWWIN, tag->ti_Data,
				TAG_DONE);
			}
			break;

		#ifdef DEBUG
		default:
			PDB(("non settable attr 0x%lx\n", tag->ti_Tag));
			break;
		#endif
	}
	NEXTTAG
}


static void mimeuri_stripcgi(STRPTR from, STRPTR to)
{
	while (*from)
	{
		if (*from == '&')
		{
			*to = ' ';
		}
		else
		{
			*to = *from;
		}
		from++;
		to++;
	}
	*to = '\0';
}


ULONG mimeuri_readargs(APTR ctx, CONST_STRPTR templ, LONG *array)
{
	struct mimeuri_context *ct = ctx;
	ULONG rc = FALSE;

	ASSERT(ct);
	ASSERT(templ);
	ASSERT(array);

	if (ct->args)
	{
		STRPTR t;

		if (ct->rda)
		{
			freeargsstring((struct RDArgs *)ct->rda);
			ct->rda = NULL;
		}

		if ( (t = malloc(strlen(ct->args) + 1)) )
		{
			mimeuri_stripcgi(ct->args, t);
			if ( (ct->rda = readargsstring(t, templ, array)) )
			{
				rc = TRUE;
			}
			free (t);
		}
	}
	return (rc);
}


ULONG mimeuri_hasscheme(CONST_STRPTR path)
{
	if (mimeuri_find_scheme(path))
	{
		return (TRUE);
	}
	return (FALSE);
}


/*
 * Encode path so that it in can safely be used with file:///
 */
#define USE_URI_CODING 0   /* use uri_encode/uri_decode compatible strings */

LONG mimeuri_encodepath(UBYTE *buf, LONG buflen, CONST_STRPTR path)
{
	LONG len = 1;
	UBYTE c;

	if (buf && !buflen)
	{
		return -1;
	}

	/* Since sprintf %s supports NULL, make sure we don't nuke if
	 * something abuses that.
	 */
	if (!path)
	{
		path = "";
	}

#if USE_URI_CODING

	/*
	 * WARNING: uri_encode doesn't implement buflen, so if USE_URI_CODING
	 * is used, the caller must NOT pass too small buffer! - Piru
	 */

	return 1 + uri_encode(path, buf);

#else

	if (buf)
	{
		UBYTE *ptr;

		ptr = buf;
		while ((c = *path++))
		{
			len++;
			switch (c)
			{
				case '@':
				case '"':
				case '%':
				case '?':
				case '=':
				case '#':
				case '&':
					if (len + 2 > buflen)
					{
						break;
					}
					len += 2;
					*ptr++ = '%';
					*ptr++ = __hex[c >> 4];
					*ptr++ = __hex[c & 0xf];
					break;

				default:
					if (len > buflen)
					{
						break;
					}
					*ptr++ = c;
					break;
			}
		}

		*ptr = '\0';
	}
	else
	{
		while ((c = *path++))
		{
			len++;
			switch (c)
			{
				case '@':
				case '"':
				case '%':
				case '?':
				case '=':
				case '#':
				case '&':
					len += 2;
					break;
			}
		}
	}

	return len;
#endif
}


/*
 * Decode path to destionation buffer
 */

LONG mimeuri_decodepath(UBYTE *buf, LONG buflen, CONST_STRPTR uri, LONG urilen)
{
	LONG len = 1;
	UBYTE *ptr;
	UBYTE c;

	if (!buf || !buflen)
	{
		return 0;
	}

	if (!uri)
	{
		uri = "";
	}

	if (urilen == -1)
	{
		urilen = 0x7fffffff;
	}

	ptr = buf;
	while ((c = *uri++))
	{
		if (urilen-- <= 0)
		{
			break;
		}

		if (++len > buflen)
		{
			break;
		}

#if USE_URI_CODING
		if (c == '+')
		{
			c = ' ';
		}
		else
#endif

		if (c == '%' && urilen >= 2 && isxdigit(uri[0]) && isxdigit(uri[1]))
		{
			c = (_hctodarray[(BYTE)uri[0]] << 4) | _hctodarray[(BYTE)uri[1]];
			uri += 2;
			urilen -= 2;
		}
		*ptr++ = c;
	}

	*ptr = '\0';

	return len;
}

