#ifndef AMBIENT_GETDIRSIZE_H
#define AMBIENT_GETDIRSIZE_H

/*
 * $Id: getdirsize.h,v 1.6 2006/08/08 13:31:34 fab Exp $
 */

struct dirsize {
	UQUAD totalsize;
	ULONG numdirs;
	ULONG numfiles;
	ULONG numhardlinks;
	ULONG numsoftlinks;
	time_t lasttime;
};

ULONG tr_getdirsize(APTR obj, CONST_STRPTR path);
ULONG tr_getdirsizes(APTR obj, struct MinList *l);
ULONG getdirsize(CONST_STRPTR path, struct dirsize *ds);

#endif /* AMBIENT_GETDIRSIZE_H */

