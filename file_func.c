/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2007 Ambient Open Source Team
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
 * $Id: file_func.c,v 1.16 2017/07/25 19:45:00 piru Exp $
 */

#include "ambient.h"

/* public */
#include <dos/dos.h>
#include <dos/dostags.h>
#include <string.h>
#include <stdarg.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <clib/alib_protos.h>

/* private */
#include "file_func.h"
#include "doslistcache.h"
#include "name.h"

/*
 * Returns TRUE if a file is a
 * directory.
 */
ULONG isdir(CONST_STRPTR path)
{
	BPTR l;
	ULONG ret = FALSE;

	THREAD;

	if( path )
	{
		if ( ( l = Lock( path, ACCESS_READ ) ) )
		{
			D_S(struct FileInfoBlock, fib);
			if (Examine(l, fib))
			{
				if (fib->fib_DirEntryType > 0)
				{
					ret = TRUE;
				}
			}
			UnLock(l);
		}
	}
	return (ret); /* XXX: not smart. If there's a failure it assumes it's a file */
}


/*
 * Checks if a device is write protected.
 * 'path' can be any file on the device or the
 * device itself. The function only works if
 * 'path' is a fully qualified path or it returns
 * it as not protected.
 */
ULONG is_device_protected(CONST_STRPTR path)
{
	STRPTR p;
	ULONG rc = FALSE;

	THREAD;

	if (path && (path[0] != ':') && (p = strchr(path, ':')))
	{
		int len = p - path + 1;
		if (len < VOLUME_SIZE)
		{
			TEXT devname[VOLUME_SIZE];
			BPTR l;

			memcpy(devname, path, len);
			devname[len] = '\0';

			if ( (l = Lock(devname, ACCESS_READ)) )
			{
				D_S(struct InfoData, idata);

				if (Info(l, idata))
				{
					if (idata->id_DiskState == ID_WRITE_PROTECTED)
					{
						rc = TRUE;
					}
				}
				UnLock(l);
			}
		}
	}
	else
	{
		PDB(("path is not absolute, <%s>\n", path));
	}
	return (rc);
}


/*
 * Tries to create a dir. Returns TRUE if it already exists
 * or if the creation was successful. It creates each component
 * of the path if needed. Only works with absolute path.
 * Doesn't handle fancy stuff like 'foo//bar' and so on.
 */
ULONG makedir(CONST_STRPTR path)
{
	BPTR l;

	THREAD;
	ASSERT(path);

	if ( (l = Lock(path, ACCESS_READ)) )
	{
		UnLock(l);
		return (TRUE);
	}
	else
	{
		STRPTR p;
		
		if ( (p = strchr(path, ':')) )
		{
			p++;

			if (*p)
			{
				TEXT buf[PATH_SIZE];
				STRPTR q = p;
				ULONG strip = FALSE;

				stccpy(buf, path, p - path + 1);
				
				while (*p)
				{
					if (strip)
					{
						buf[q - path - 1] = '/';
					}
					
					if (!(q = strchr(p, '/')))
					{
						q = p + strlen(p) - 1;
					}
					q++;

					stccpy(&buf[p - path], p, q - p + 1);
					
					if (buf[q - path - 1] == '/')
					{
						buf[q - path - 1] = '\0';
						strip = TRUE;
					}

					if ((l = Lock(buf, ACCESS_READ)) || (l = CreateDir(buf)))
					{
						UnLock(l);
					}
					else
					{
						return (FALSE);
					}
					p = q;
				}
				return (TRUE);
			}
		}
	}
	return (FALSE);
}


/*
 * Given a lock to a directory and a command name,
 * tests if the whole path exists and builds resulting string
 * containing the full path.
 */

static STRPTR name_buildfromlock(BPTR l)
{
	ULONG length = 256;
	STRPTR dest = malloc(length);

	while(dest != NULL)
	{
		if (NameFromLock(l, dest, length) == FALSE && IoErr() == ERROR_LINE_TOO_LONG)
		{
			free(dest);
			length *= 2;
			dest = malloc(length);
			continue;
		}

		free(dest);
		dest = NULL;
	};

	return dest;
}

static ULONG resolve_path(BPTR l, CONST_STRPTR cmd, STRPTR *to)
{
	BPTR oldcd, tl;

	THREAD;

	if (to != NULL)
		*to = NULL;

	oldcd = CurrentDir(l);
	tl = Lock(cmd, SHARED_LOCK);
	CurrentDir(oldcd);
	if (tl != NULL && to != NULL)
	{
		*to = name_buildfromlock(tl);
	}
	UnLock(tl);

	return (to == NULL || *to != NULL) ? (tl ? TRUE : FALSE) : FALSE;
}

/*
 * Given a path to a directory and a command name,
 * tests if the whole path exists.
 */
static ULONG resolve_pathname(CONST_STRPTR path, CONST_STRPTR cmd)
{
	BPTR l;
	ULONG rc = FALSE;

	THREAD;

	if ( (l = Lock(path, SHARED_LOCK)) )
	{
		rc = resolve_path(l, cmd, NULL);
		UnLock(l);
	}
	return (rc);
}

/*
 * Expands assigns etc to full path.
 */

ULONG path_expand(CONST_STRPTR path, STRPTR to, ULONG size)
{
	BPTR l;
	ULONG rc = FALSE;

	if(path[0]=='@')
	{
		stccpy(to, path, size);
		rc = TRUE;
	}
	else
	{
		if ( (l = Lock(path, SHARED_LOCK)) )
		{
			NameFromLock(l, to, size);
			DB(("<%s>-><%s>\n",path,to));
			UnLock(l);
			rc = TRUE;
		}
	}
	return (rc);
}

/* version with dynamic string building. no 'public' yet */

static ULONG path_buildexpand(CONST_STRPTR path, STRPTR *to)
{
	BPTR l;

	*to = NULL;

	if(path[0]=='@')
	{
		*to = name_build(path);
	}
	else
	{
		if ( (l = Lock(path, SHARED_LOCK)) )
		{
			*to = name_buildfromlock(l);
			DB(("<%s>-><%s>\n",path,*to));
			UnLock(l);
		}
	}
	return *to != NULL;
}

/*
 * Checks if 2 paths are same (handles assigns and volume/device names).
 */

ULONG is_path_equal(CONST_STRPTR path1, CONST_STRPTR path2)
{
	ULONG rc = FALSE;
	
	THREAD;
	ASSERT(path1);
	ASSERT(path2);

	/* try simple one */

	if ( 0 == stricmp( path1, path2 ) )
	{
		return TRUE;
	}

	/* resolve paths and try again */

	{
		STRPTR npath1 = NULL;
		STRPTR npath2 = NULL;

		if (path_buildexpand(path1, &npath1) && path_buildexpand(path2, &npath2))
		{
			if ( 0 == stricmp( npath1, npath2 ) )
				rc = TRUE;
		}
		else
		{
			/* XXX: Now, we return notequal here or what? */
		}

		if (npath1 != NULL)
			free(npath1);
		if (npath2 != NULL)
			free(npath2);

	}

	return rc;
}

/*
 * extended SystemTags() with WB support (XXX: should go away.. eventually)
 */
LONG v_systemtags(CONST_STRPTR cmd, struct TagItem *tags)
{
	struct pn {
		BPTR next;
		BPTR lock;
	} *pn;
	STRPTR ncmd = NULL;
	STRPTR p;
	struct Process *wb;
	struct CommandLineInterface *wbcli;
	LONG res;

	THREAD;
	ASSERT(cmd);

	if (!Cli())
	{
		/*
		 * We're from Workbench.
		 */
		ncmd = name_build(cmd);
		if (ncmd == NULL)
			return -1;

		p = stpbrk(ncmd, " \t ");
		if (p)
		{
			*p = 0;
		}

		/*
		 * Check default path (XXX: use internal one.. blah)
		 */
		if (!resolve_pathname("", ncmd) || !resolve_pathname("C:", ncmd))
		{
			if ( (wb = (struct Process *) FindTask( "Workbench")) )
			{
				if ( (wbcli = BADDR(wb->pr_CLI)) )
				{
					pn = BADDR(wbcli->cli_CommandDir);
					while (pn)
					{
						STRPTR ncmd_arg = NULL;
						if (resolve_path(pn->lock, ncmd, &ncmd_arg))
						{
							STRPTR fullcmd = ncmd_arg;
							if ( (p = stpbrk(cmd, " \t ")) )
							{
								fullcmd	= malloc(strlen(ncmd_arg) + strlen(p) + 1);
								if (fullcmd == NULL)
								{
									free(ncmd_arg);
									return -1;
								}

								strcpy(fullcmd, ncmd_arg);
								strcat(strchr(fullcmd, '\0'), p);
							}
							res = SystemTagList(fullcmd, tags);
							free(ncmd_arg);
							if (fullcmd != ncmd_arg)
								free(fullcmd);

							return (res);
						}
						pn = BADDR(pn->next);
					}
				}
			}
		}
	}
	res = SystemTagList(cmd, tags);
	return (res);
}


/*
 * Asynchronous Execute()
 */
ULONG execute_async(CONST_STRPTR cmd, BPTR input, BPTR output)
{
	TEXT n[PATH_SIZE + 32];
	LONG len;

	len = strlen(cmd);
	if (len <= PATH_SIZE)
	{
		sprintf(n, "failat 100\nexecute \"%s\"", cmd);

		if (!systemtags(n,
			SYS_Asynch, TRUE,
			SYS_Input, input,
			SYS_Output, output,
			NP_Priority, 0,
			TAG_DONE
		))
		{
			return (TRUE);
		}
	}
	return (FALSE);
}


/*
 * Returns TRUE if both path refer to the
 * same volume.
 */
ULONG same_volume(CONST_STRPTR path1, CONST_STRPTR path2)
{
	struct DevProc *dvp1;
	ULONG rc = FALSE;

	THREAD;
	ASSERT(path1);
	ASSERT(path2);

	if ( (dvp1 = GetDeviceProc(path1, NULL)) )
	{
		struct DevProc *dvp2;

		while ( (dvp1->dvp_Flags & DVPF_ASSIGN) )
		{
			dvp1 = GetDeviceProc(path1, dvp1);
		}

		if ( (dvp2 = GetDeviceProc(path2, NULL)) )
		{
			while ( (dvp2->dvp_Flags & DVPF_ASSIGN) )
			{
				dvp2 = GetDeviceProc(path2, dvp2);
			}

			if ( (dvp1->dvp_Port == dvp2->dvp_Port) )
			{
				rc = TRUE;
			}
			FreeDeviceProc(dvp2);
		}
		FreeDeviceProc(dvp1);
	}
	return (rc);
}


ULONG same_volume_nolock(CONST_STRPTR path1, CONST_STRPTR path2)
{
	CONST_STRPTR s, d;

	s = path1;

	while (*s)
	{
		if (*s == ':') break;
		s++;
	}

	ASSERT(*s == ':');

	d = path2;

	while (*d)
	{
		if (*d == ':') break;
		d++;
	}

	ASSERT(*d == ':');

	if ((s - path1) == (d - path2))
	{
		if (!Strnicmp(path1, path2, s - path1))
		{
			return (TRUE);
		}
	}
	return (FALSE);
}


/*
 * Returns the blocksize in bytes of a volume.
 */
ULONG get_blocksize(CONST_STRPTR path)
{
	struct DevProc *dvp;
	ULONG retval = NULL;

	THREAD;
	ASSERT(path);

	if ( (dvp = GetDeviceProc(path, NULL)) )
	{
		struct dlcnode *dlc;

		while (dvp->dvp_Flags & DVPF_ASSIGN)
		{
			dvp = GetDeviceProc(path, dvp);
		}

		if ( (dlc = doslistcache_find_dlcdevice(dvp->dvp_Port)) ) /* XXX: eek, we should lock the entry, well, it won't go away though */
		{
			retval = dlc->blocksize;
		}
		FreeDeviceProc(dvp);
	}
	return (retval);
}

/*
 * Checks if a path is a softlink
 */

ULONG islink(CONST_STRPTR name)
{
#if 1
	ULONG ret = FALSE;
	CONST_STRPTR file;
	LONG len;
	LONG pathlen;

	ASSERT(name);

	/* Separate path and file */
	file = FilePart(name);

	pathlen = file - name;
	len = strlen(file);
	if (pathlen < PATH_SIZE && len && len < 256) /* atm we only handle dir if specified without '/' */
	{
		LONG olderr;
		BPTR dirlock;
		BPTR olddir = olddir; /* stfu */

		olderr = IoErr();

		if (pathlen)
		{
			UBYTE path[pathlen + 1];

			memcpy(path, name, pathlen);
			path[pathlen] = '\0';

			dirlock = Lock(path, ACCESS_READ);
			if (dirlock)
			{
				olddir = CurrentDir(dirlock);
			}
		}
		else
		{
			/* use currentdir */
			dirlock = 1;
		}

		if (dirlock)
		{
			struct DevProc *dvp;

			if ( (dvp = GetDeviceProc(file, NULL)) )
			{
				UBYTE _buf[len + 3];
				UBYTE *buf = (APTR) ((IPTR)(_buf + 3) & ~3);
				BPTR lock;

				buf[0] = len;
				memcpy(buf + 1, file, len);

				if ( (lock = DoPkt3(dvp->dvp_Port, ACTION_LOCATE_OBJECT, dvp->dvp_Lock, MKBADDR(buf), ACCESS_READ)) )
				{
					struct FileLock *fl = BADDR(lock);
					if (fl->fl_Task)
					{
						DoPkt1(fl->fl_Task, ACTION_FREE_LOCK, lock);
					}
				}
				else
				{
					if (IoErr() == ERROR_IS_SOFT_LINK)
					{
						/* Yep, it's a softlink! */
						ret = TRUE;
					}
				}

				FreeDeviceProc(dvp);
			}

			if (pathlen)
			{
				(void) CurrentDir(olddir);

				UnLock(dirlock);
			}
		}

		SetIoErr(olderr);
	}

	return ret;

#else

#warning "Doesn't work, Lock() always resolves softlinks!"
	BPTR lock;

	ASSERT(name);

	lock = Lock( name, ACCESS_READ );
	if ( lock )
	{
		D_S(struct FileInfoBlock, fib);
		if ( Examine(lock, fib) )
		{
			if ( fib->fib_DirEntryType == ST_SOFTLINK )
			{
				ret = TRUE;
			}
		}

		UnLock( lock );
	}

	return ret;
#endif
}


/*
 * Checks if a path is a devicename only
 * (eg. "foobar:").
 */
ULONG isdevicename(CONST_STRPTR name)
{
	int len;

	ASSERT(name);

	len = strlen(name);
	if ((*FilePart(name) == '\0') && (len && name[len - 1] != '/'))
	{
		return (TRUE);
	}
	else
	{
		return (FALSE);
	}
}


/*
 * Converts a dostype to a string. Destination
 * must be 5 bytes long and is '\0' terminated.
 */
void dostype_to_str(ULONG dt, STRPTR s)
{
	STRPTR dtn;
	ULONG i;

	ASSERT(s);

	dtn = (STRPTR)&dt;

	if (dt)
	{
		for (i = 0; i < 4; i++)
		{
			if (dtn[i] < 10)
			{
				*s = dtn[i] + '0';
			}
			else
			{
				if (isalnum(dtn[i]))
				{
					*s = dtn[i];
				}
				else
				{
					*s = '-';
				}
			}
			s++;
		}
	}
	else
	{
		*s++ = '-';
		*s++ = '-';
		*s++ = '-';
		*s++ = '-';
	}
	*s = '\0';
}


void set_wildstar(void)
{
	DOSBase->dl_Root->rn_Flags |= RNF_WILDSTAR;
}

ULONG dosprotection_get(CONST_STRPTR path, ULONG *mask)
{
	BPTR l;
	ULONG rc = FALSE;
	*mask = 0;

	if ( (l = Lock(path, ACCESS_READ)) )
	{
		D_S(struct FileInfoBlock, fib);

		rc = Examine(l, fib);

		UnLock(l);

		if (rc)
		{
			if(fib->fib_Protection & FIBF_SCRIPT)
			{
				*mask |= PROTF_SCRIPT;
			}
			if(fib->fib_Protection & FIBF_ARCHIVE)
			{
				*mask |= PROTF_ARCHIVE;
			}
			if((fib->fib_Protection & FIBF_READ) == 0)
			{
				*mask |= PROTF_READ;
			}
			if((fib->fib_Protection & FIBF_WRITE) == 0)
			{
				*mask |= PROTF_WRITE;
			}
			if((fib->fib_Protection & FIBF_EXECUTE) == 0)
			{
				*mask |= PROTF_EXECUTE;
			}
			if((fib->fib_Protection & FIBF_DELETE) == 0)
			{
				*mask |= PROTF_DELETE;
			}
		}
	}

	return (rc);
}

ULONG dosprotection_set(CONST_STRPTR path, ULONG mask)
{
	BPTR l;
	ULONG rc = FALSE;

	if ( (l = Lock(path, ACCESS_READ)) )
	{
		D_S(struct FileInfoBlock, fib);

		rc = Examine(l, fib);

		UnLock(l);

		if (rc)
		{
			if (mask & PROTF_SCRIPT)
			{
				fib->fib_Protection |= FIBF_SCRIPT;
			}
			if (mask & PROTF_ARCHIVE)
			{
				fib->fib_Protection |= FIBF_ARCHIVE;
			}
			if (mask & PROTF_READ)
			{
				fib->fib_Protection &= ~FIBF_READ;
			}
			if (mask & PROTF_WRITE)
			{
				fib->fib_Protection &= ~FIBF_WRITE;
			}
			if (mask & PROTF_EXECUTE)
			{
				fib->fib_Protection &= ~FIBF_EXECUTE;
			}
			if (mask & PROTF_DELETE)
			{
				fib->fib_Protection &= ~FIBF_DELETE;
			}
			rc = SetProtection(path, fib->fib_Protection);
		}
	}
	return (rc);
}


ULONG dosprotection_clear(CONST_STRPTR path, ULONG mask)
{
	BPTR l;
	ULONG rc = FALSE;

	if ( (l = Lock(path, ACCESS_READ)) )
	{
		D_S(struct FileInfoBlock, fib);

		rc = Examine(l, fib);

		UnLock(l);

		if (rc)
		{
			if (mask & PROTF_SCRIPT)
			{
				fib->fib_Protection &= ~FIBF_SCRIPT;
			}
			if (mask & PROTF_ARCHIVE)
			{
				fib->fib_Protection &= ~FIBF_ARCHIVE;
			}
			if (mask & PROTF_READ)
			{
				fib->fib_Protection |= FIBF_READ;
			}
			if (mask & PROTF_WRITE)
			{
				fib->fib_Protection |= FIBF_WRITE;
			}
			if (mask & PROTF_EXECUTE)
			{
				fib->fib_Protection |= FIBF_EXECUTE;
			}
			if (mask & PROTF_DELETE)
			{
				fib->fib_Protection |= FIBF_DELETE;
			}

			rc = SetProtection(path, fib->fib_Protection);
		}
	}
	return (rc);
}

ULONG exists(CONST_STRPTR file)
{
	BPTR l;
	ULONG rc = FALSE;

	if ( (l = Lock(file, ACCESS_READ)) )
	{
		UnLock(l);
		rc = TRUE;
	}
	return (rc);
}

/* returns true if source path is contained in destination path */
ULONG is_path_contained(CONST_STRPTR dest, CONST_STRPTR source)
{
	TEXT s[PATH_SIZE], d[PATH_SIZE];
	int slen, dlen;

	stccpy(s, source, sizeof(s));
	stccpy(d, dest,   sizeof(d));

	slen = strlen(s);
	if (slen && s[slen - 1] != '/' && s[slen - 1] != ':')
	{
		if (slen + 1 < PATH_SIZE)
		{
			s[slen++] = '/';
			s[slen] = '\0';
		}
	}

	dlen = strlen(d);
	if (dlen && d[dlen - 1] != '/' && d[dlen - 1] != ':')
	{
		if (dlen + 1 < PATH_SIZE)
		{
			d[dlen++] = '/';
			d[dlen] ='\0';
		}
	}

	if (slen <= dlen && strstr(d, s))
	{
		return (TRUE);
	}
	else
	{
		return (FALSE);
	}
}

ULONG is_path_parent(CONST_STRPTR dest, CONST_STRPTR source )
{
	LONG dlen = dest ? strlen( dest ) : 0;
	LONG slen = source ? strlen( source ) : 0;

	if ( slen == 0 || dlen == 0 )
		return FALSE;

	/*
	 * Cut trailing /'s to make things easier.
	 */

	if ( dest[ dlen - 1 ] == '/' )
		dlen--;

	if ( source[ slen - 1 ] == '/' )
		slen--;

	if ( slen == 0 || dlen == 0 )
		return FALSE;

	/*
	 * Parent path from source pat.
	 */

	while( source[ slen - 1 ] != '/' && source[ slen - 1 ] != ':' )
		slen--;

    if ( slen == 0 || dlen == 0 )
		return FALSE;

	if ( source[ slen - 1 ] == '/' )
		slen--;

    if ( slen == 0 || dlen == 0 )
		return FALSE;

	if ( slen == dlen && !strnicmp( source, dest, slen ) )
		return TRUE;

	return FALSE;
}

#if 0
/*
 * Ok, this is a dirty hack that assumes a lot of things
 * to know if we're booting from a CD. Don't abuse it :)
 * Will probably go away once there's some query interface
 * in CDrive.
 */
ULONG is_booting_from_cd(void)
{
	BPTR l;
	BPTR oldcd;
	ULONG rc = FALSE;

	if (oldcd = CurrentDir(0))
	{
		if (l = Lock("", ACCESS_READ))
		{
			D_S(struct InfoData, inf);

			if (Info(l, inf))
			{
				if (inf->id_BytesPerBlock == 2048 &&
					inf->id_DiskType == 0x444f5300)
				{
					TEXT buf[12];
					STRPTR p;
					struct DeviceList *dl;

					dl = BADDR(inf->id_VolumeNode);
					p = BADDR(dl->dl_Name);
					stccpy(buf, p + 1, min(*p, sizeof(buf) - 1) + 1);

					if (!strcmp("MorphOSBoot", buf))
					{
						struct Task *cdtsk = dl->dl_Task->mp_SigTask;

						if (cdtsk)
						{
							if (strlen(cdtsk->tc_Node.ln_Name) == 3 && !strncmp("CD", cdtsk->tc_Node.ln_Name, 2))
							{
								rc = TRUE;
							}
						}
					}
				}
			}
			UnLock(l);
		}
		CurrentDir(oldcd);
	}
	return (rc);
}
#endif
