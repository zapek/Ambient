#ifndef AMBIENT_DOS_INTERNAL_H
#define AMBIENT_DOS_INTERNAL_H
/*
 * $Id: dos_internal.h,v 1.3 2006/02/22 14:48:19 fab Exp $
 */

extern struct DosLibrary *DOSBase;

STRPTR DosGetString(LONG num);

#define DosGetString(__p0) \
	LP1(0x3d2, STRPTR, DosGetString, \
	LONG, __p0, d1, \
	, DOSBase, 0, 0, 0, 0, 0, 0)

#endif /* AMBIENT_DOS_INTERNAL_H */
