#ifndef AMBIENT_DATATYPES_H
#define AMBIENT_DATATYPES_H
/*
 * $Id: datatypes.h,v 1.3 2006/02/22 14:48:19 fab Exp $
 */

enum {
	DTTYPE_NONE,
	DTTYPE_SOUND,
	DTTYPE_IMAGE,
	DTTYPE_TEXT,
	DTTYPE_HEX,
	DTTYPE_BOOPSI,
};

ULONG datatypes_findtype(STRPTR path);
STRPTR datatypes_nametype(ULONG type);

#endif /* AMBIENT_DATATYPES_H */
