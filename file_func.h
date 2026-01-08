#ifndef AMBIENT_FILE_FUNC_H
#define AMBIENT_FILE_FUNC_H
/*
 * $Id: file_func.h,v 1.9 2007/05/08 19:27:08 fab Exp $
 */

#include <dos/dos.h>

#define PROTF_SCRIPT  (1 << 0UL)
#define PROTF_ARCHIVE (1 << 1UL)
#define PROTF_READ    (1 << 2UL)
#define PROTF_WRITE   (1 << 3UL)
#define PROTF_EXECUTE (1 << 4UL)
#define PROTF_DELETE  (1 << 5UL)

#define NO_FILESIZE   (0xFFFFFFFFFFFFFFFFULL)



ULONG isdir(CONST_STRPTR path);
ULONG makedir(CONST_STRPTR path);
LONG systemtags(CONST_STRPTR cmd, ...);
LONG v_systemtags(CONST_STRPTR cmd, struct TagItem *tags);
ULONG same_volume(CONST_STRPTR path1, CONST_STRPTR path2);
ULONG same_volume_nolock(CONST_STRPTR path1, CONST_STRPTR path2);
ULONG isdevicename(CONST_STRPTR name);
ULONG execute_async(CONST_STRPTR cmd, BPTR input, BPTR output);
ULONG get_blocksize(CONST_STRPTR path);
void delete_quoted_name(CONST_STRPTR name);
void dostype_to_str(ULONG dt, STRPTR s);
ULONG is_device_protected(CONST_STRPTR path);
void set_wildstar(void);
ULONG dosprotection_get(CONST_STRPTR path, ULONG *mask);
ULONG dosprotection_set(CONST_STRPTR path, ULONG mask);
ULONG dosprotection_clear(CONST_STRPTR path, ULONG mask);
ULONG islink(CONST_STRPTR name);
ULONG path_expand(CONST_STRPTR path, STRPTR to, ULONG size);
ULONG exists(CONST_STRPTR file);
ULONG is_path_contained(CONST_STRPTR dest, CONST_STRPTR source);
ULONG is_path_parent(CONST_STRPTR dest, CONST_STRPTR source );
ULONG is_path_equal(CONST_STRPTR path1, CONST_STRPTR path2);

#endif /* AMBIENT_FILE_FUNC_H */
