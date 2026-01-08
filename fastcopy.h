#ifndef AMBIENT_FASTCOPY_H
#define AMBIENT_FASTCOPY_H

/*
 * $Id: fastcopy.h,v 1.8 2018/07/24 09:48:51 itix Exp $
 */


/*
 * Copy modes
 */
enum {
	CM_FIXED,
	CM_BLOCK,
	CM_INCREMENTAL
};

/*
 * Replace/unprotect flags
 */

#define COPY_REPLACE_SKIP   (1L<<0)
#define COPY_REPLACE_ASK    (1L<<1)
#define COPY_REPLACE_ALL    (1L<<2)
#define COPY_REPLACE_CANCEL (1L<<3)

#define COPY_REPLACE_MASK 15

#define COPY_UNPROTECT_SKIP   (1L<<4)
#define COPY_UNPROTECT_ASK    (1L<<5)
#define COPY_UNPROTECT_ALL    (1L<<6)
#define COPY_UNPROTECT_CANCEL (1L<<7)

#define COPY_UNPROTECT_MASK 240

/*
 * fastcopy() returncodes
 */
#define FASTCOPY_ABORTED  2
#define FASTCOPY_OK       1
#define FASTCOPY_FAILED   0
#define FASTCOPY_NOSOURCE 0xffffffff

#define MOVE_ABORTED  2
#define MOVE_OK       1
#define MOVE_FAILED   0
#define MOVE_NOSOURCE 0xffffffff

ULONG fastcopy(APTR obj, CONST_STRPTR from, CONST_STRPTR to, ULONG clone, ULONG *time, ULONG count, QUAD * totaldone, ULONG *reqflags);

#endif /* AMBIENT_FASTCOPY_H */
