#ifndef AMBIENT_FINDCLASS_H
#define AMBIENT_FINDCLASS_H
/*
 * $Id: findclass.h,v 1.2 2019/02/20 18:30:26 bitrocky Exp $
 */

ULONG tr_find(APTR obj, APTR wo);

struct result_item {
	STRPTR filepart;
	STRPTR comment; // has to be bofore the "text"!
	TEXT text[ 0 ];
};

#endif /* AMBIENT_FINDCLASS_H */
