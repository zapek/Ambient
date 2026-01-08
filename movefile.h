#ifndef AMBIENT_MOVEFILE_H
#define AMBIENT_MOVEFILE_H
/*
 * $Id
 */

ULONG tr_movefile(APTR obj, APTR refwin, CONST_STRPTR from, CONST_STRPTR to, ULONG copy, APTR progressobj, ULONG * filecount, QUAD * totaldone, ULONG noicon, ULONG rename, ULONG *reqflags);

#endif /* AMBIENT_MOVEFILE_H */
