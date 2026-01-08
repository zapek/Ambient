#ifndef AMBIENT_MOVEDIR_H
#define AMBIENT_MOVEDIR_H

/*
 * $Id: movedir.h,v 1.9 2018/07/24 09:48:51 itix Exp $
 */

ULONG tr_movedir(APTR obj, APTR refwin, CONST_STRPTR from, CONST_STRPTR to, ULONG copy, APTR progressobj, ULONG * filecount, QUAD * totaldone, ULONG noicon, ULONG rename, ULONG *reqflags);


#endif /* AMBIENT_MOVEDIR_H */
