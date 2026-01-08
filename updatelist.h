#ifndef AMBIENT_UPDATELIST_H
#define AMBIENT_UPDATELIST_H
/*
 * $Id: updatelist.h,v 1.3 2006/02/22 14:48:23 fab Exp $
 */

ULONG updatelist_begin(void);
void updatelist_add(ULONG id);
void updatelist_end(ULONG doit, ULONG clearit);

#endif /* AMBIENT_UPDATELIST_H */
