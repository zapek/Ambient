#ifndef AMBIENT_RENAME_H
#define AMBIENT_RENAME_H
/*
 * $Id: rename.h,v 1.8 2007/05/08 19:27:09 fab Exp $
 */

/*
 * Replace/unprotect flags
 */

#define RENAME_REPLACE_CANCEL (1L<<0)

#define RENAME_REPLACE_MASK 1

#define RENAME_UNPROTECT_CANCEL (1L<<1)

#define RENAME_UNPROTECT_MASK 2

#define RENAME_ABORTED  2
#define RENAME_OK       1
#define RENAME_FAILED   0
#define RENAME_NOSOURCE 0xffffffff

ULONG tr_rename(CONST_STRPTR from, CONST_STRPTR * pathlist, CONST_STRPTR to, ULONG noicon, ULONG notify, ULONG *reqflags);

#endif /* AMBIENT_RENAME_H */
