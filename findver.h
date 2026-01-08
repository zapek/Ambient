#ifndef AMBIENT_FINDVER_H
#define AMBIENT_FINDVER_H
/*
 * $Id: findver.h,v 1.5 2006/08/08 13:31:34 fab Exp $
 */

ULONG tr_findver(APTR obj, CONST_STRPTR path);
ULONG tr_findver_list(APTR obj, struct MinList *l);

#endif /* AMBIENT_FINDVER_H */
