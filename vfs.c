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
 * long with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 * $Id: vfs.c,v 1.30 2025/08/28 13:06:42 piru Exp $
 */

#include "ambient.h"

/* public */
#include <exec/types.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <dos/dostags.h>

/* private */
#include "mui_func.h"
#include "name.h"
#include "vfs.h"
#include "dosnotify.h"

#define GetDosObjectAttrTags(__p0, __p1, ...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	GetDosObjectAttr(__p0, __p1, (struct TagItem *)_tags);})


static struct MinList vfslist;

static struct SignalSemaphore vfssem;

struct vfsnode
{
	struct MinNode n;
	APTR fscontext;
	TEXT path[PATH_SIZE];
	TEXT filename[PATH_SIZE];
	UBYTE name[0];
};

ULONG vfs_init(void)
{
	InitSemaphore(&vfssem);

	NEWLIST( &vfslist );

	return TRUE;
}

void vfs_cleanup(void)
{
	struct vfsnode * n, * nextn;

	ITERATELISTSAFE(n, nextn, &vfslist)
	{
		if(n->fscontext)
		{
			vfs_close(n);
		}
		free(n);
	}
}

void vfs_release(APTR obj, CONST_STRPTR path)
{
	struct vfsnode * vnode;
	TEXT device[64];
	ULONG i = 0;
	APTR lastwin = NULL;

	if(!path)
	{
		return;
	}

	DB(("vfs_release <%s>\n", path));

	while(i < sizeof(device) && *path && *path != ':')
	{
		device[i++] = *path++;
	}
	device[i]=0;

	ObtainSemaphore(&vfssem);
	ITERATELIST(vnode, &vfslist)
	{
		TEXT view_device[64];
		ULONG refcount = 0;

		if(!stricmp(device, vnode->name))
		{
			refcount++;
			lastwin = obj;
		}

		FORCHILD(app, MUIA_Application_WindowList)
		{
			ULONG type = 0;

			if(obj != child)
			{
				if(get(child, MA_Window_Type, &type) && type == MV_Window_Type_View)
				{
					STRPTR view_path = (STRPTR) getv(child, MA_Window_Path);

					if(view_path)
					{
						i = 0;

						while(i < sizeof(view_device) && *view_path && *view_path != ':')
						{
							view_device[i++] = *view_path++;
						}
						view_device[i] = 0;

						if(!stricmp(view_device, vnode->name))
						{
							refcount++;
							lastwin = child;
						}
					}
				}
			}
		}
		NEXTCHILD

		DB(("vfs_release: refcount = %d\n", refcount));

		if(lastwin && !stricmp(device, vnode->name) && refcount == 1)
		{
			if(vnode->fscontext)
			{
				DB(("vfs_close <%s>\n", vnode->name));
				set(lastwin, MA_Window_EnableDOSNotify, FALSE);
				vfs_close(vnode);
			}

			REMOVE(vnode);
			free(vnode);
			break;
		}
	}
	ReleaseSemaphore(&vfssem);
}

ULONG vfs_lookup(CONST_STRPTR type)
{
	if(!strnicmp("xad", type, 3))
		return VFS_XAD;

	if(!strnicmp("iso", type, 3))
		return VFS_ISO;

	return VFS_NONE;
}

APTR vfs_open(CONST_STRPTR path, ULONG type)
{
	APTR   ret = NULL;
	BPTR   MySegList;
	APTR   MyFSContext;
	STRPTR quotedpath;

#if 0
	/* Requires at least dos 51.21 */
	if (!LIB_MINVER(&DOSBase->dl_lib, 51, 21))
	{
		return ret;
	}
#endif

	MyFSContext = NULL;
	quotedpath = name_build_readargs_quoted(path, NULL, 0);
	if(quotedpath)
	{
		STRPTR buffer = (STRPTR) malloc(5 + strlen(quotedpath) + 1);

		if(buffer)
		{
			sprintf(buffer, "file=%s", quotedpath);

			if(type == VFS_XAD)
			{
				if ( (MySegList=LoadSeg("l:xadfs")) )
				{
					MyFSContext=AllocDosObjectTags(DOS_FSCONTEXT,
					                               ADO_DN_Seglist, (ULONG) MySegList,
					                               ADO_DN_Priority, 5,
					                               ADO_DN_Startup, (ULONG) buffer,
					                               TAG_END);
					/* If AllocDosObjectTags failed, then the seglist needs to be unloaded by us.
					   In all other cases the FSCONTEXT API will handle releasing it. */
					if (!MyFSContext)
						UnLoadSeg(MySegList);
				}
			}

			if(type == VFS_ISO)
			{
				MyFSContext=AllocDosObjectTags(DOS_FSCONTEXT,
				                               ADO_FS_DosType,/*ID_CDFS_DISK*/ 0x43444653,
				                               ADO_DN_Priority, 5,
				                               ADO_DN_Startup,(ULONG) buffer,
				                               TAG_END);
			}

			free(buffer);
		}

		name_delete(quotedpath);
	}

	if(MyFSContext)
	{
		struct vfsnode * vnode=NULL;
		struct DosAttrBuffer name={NULL,0}; /* Get required buffer size, including terminating 0. */

		if (GetDosObjectAttrTags(DOS_FSCONTEXT,MyFSContext,
		                         ADO_DN_Name,(ULONG)&name,
		                         TAG_END))
		{
			vnode = (struct vfsnode *) malloc(sizeof(*vnode) + name.dab_Len);
			if(vnode)
			{
				name.dab_Ptr = vnode->name;
				if (GetDosObjectAttrTags(DOS_FSCONTEXT,MyFSContext,
				                         ADO_DN_Name,(ULONG)&name,
				                         TAG_END))
				{
					stccpy(vnode->path, path, min(sizeof(vnode->path), FilePart(path) - path + 1));
					stccpy(vnode->filename, FilePart(path), sizeof(vnode->filename));
					vnode->fscontext = MyFSContext;
					ObtainSemaphore(&vfssem);
					ADDTAIL( &vfslist, vnode);
					ReleaseSemaphore(&vfssem);

					/* Use vnode itself as "context" so we get name easily */
					ret = vnode;
				}
			}
		}

		if (!ret)
		{
			if (vnode)
				free(vnode);
			FreeDosObject(DOS_FSCONTEXT,MyFSContext);
		}
	}

	return ret;
}

void vfs_close(APTR context)
{
	struct vfsnode * ctx = context;
	DB(("vfs_close : DeleteFSContext\n"));
//#warning "fscontext (or cdrive/xadfs) don't behave too well when a localfs is released while a lock is pending."
	FreeDosObject(DOS_FSCONTEXT,ctx->fscontext);
	ctx->fscontext = NULL;
}

STRPTR vfs_device(APTR context)
{
	struct vfsnode * ctx = context;

	return ctx->name;
}

ULONG vfs_is_vfs_device(CONST_STRPTR path)
{
	return *path == '@'; /* ahum */
}

STRPTR vfs_resolve_path(STRPTR path, STRPTR result, ULONG size, ULONG volume)
{
	struct vfsnode * vnode;
	TEXT device[64];
	int i = 0;
	STRPTR path_tmp = path;
	ULONG found = FALSE;

	if(!path_tmp)
	{
		if (size)
			result[0] = 0;
		return NULL;
	}

	while(i < sizeof(device) && *path_tmp && *path_tmp != ':')
	{
		device[i++] = *path_tmp++;
	}
	device[i]=0;

	ObtainSemaphore(&vfssem);

	ITERATELIST(vnode, &vfslist)
	{
		if(!stricmp(device, vnode->name))
		{
			if(volume)
			{
				stccpy(result, vnode->filename, size);
			}
			else
			{
				snprintf(result, size, "%s%s", vnode->filename, path_tmp);
			}

			found = TRUE;
		}
	}

	ReleaseSemaphore(&vfssem);

	if(!found)
	{
		stccpy(result, path, size);
	}

	return result;
}

STRPTR vfs_get_owner_path(STRPTR path, STRPTR result, ULONG size)
{
	struct vfsnode * vnode;
	TEXT device[64];
	int i = 0;
	STRPTR path_tmp = path;
 
	if(!path_tmp)
	{
		return NULL;
	}

	while(i < sizeof(device) && *path_tmp && *path_tmp != ':')
	{
		device[i++] = *path_tmp++;
	}
	device[i]=0;

	ObtainSemaphore(&vfssem);

	ITERATELIST(vnode, &vfslist)
	{
		if(!stricmp(device, vnode->name))
		{
			stccpy(result, vnode->path, size);
		}
	}

	ReleaseSemaphore(&vfssem);

	return result;
}
