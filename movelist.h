#ifndef AMBIENT_MOVELIST_H
#define AMBIENT_MOVELIST_H
/*
 * $Id: movelist.h,v 1.7 2007/02/11 22:35:30 fab Exp $
 */

struct movenode {
	struct MinNode n;
	ULONG type;
	TEXT path[0];
};

ULONG tr_move(APTR obj, APTR refwin, struct MinList * l, CONST_STRPTR srcpath, CONST_STRPTR dstpath, ULONG copy, ULONG noicon, ULONG rename);

#endif /* AMBIENT_MOVELIST_H */
