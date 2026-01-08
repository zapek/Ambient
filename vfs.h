#ifndef AMBIENT_VFS_H
#define AMBIENT_VFS_H

/*
 * $Id: vfs.h,v 1.7 2008/08/15 18:33:07 fab Exp $
 */

#include <exec/types.h>

enum { VFS_NONE, VFS_XAD, VFS_ISO };

ULONG vfs_init(void);
void vfs_cleanup(void);

void vfs_release(APTR obj, CONST_STRPTR path);
ULONG vfs_lookup(CONST_STRPTR type);
APTR vfs_open(CONST_STRPTR path, ULONG type);
void vfs_close(APTR context);
STRPTR vfs_device(APTR context);
ULONG vfs_is_vfs_device(CONST_STRPTR path);
STRPTR vfs_resolve_path(STRPTR path, STRPTR result, ULONG size, ULONG volume);
STRPTR vfs_get_owner_path(STRPTR path, STRPTR result, ULONG size);

#endif /* AMBIENT_VFS_H */
