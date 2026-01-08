#ifndef AMBIENT_ABOUTMOD_ABOUTCLASSES_H
#define AMBIENT_ABOUTMOD_ABOUTCLASSES_H
/*
 * $Id: aboutclasses.h,v 1.2 2005/07/04 23:06:13 laire Exp $
 */

#define DEFCLASS(s) ULONG create_##s##class(void); \
	APTR get##s##class(void); \
	APTR get##s##classroot(void); \
	void delete_##s##class(void)

DEFCLASS(pong);

#endif /* AMBIENT_ABOUTMOD_ABOUTCLASSES_H */
