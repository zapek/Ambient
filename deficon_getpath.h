#ifndef AMBIENT_DEFICON_GETPATH_H
#define AMBIENT_DEFICON_GETPATH_H
/*
 * $Id: deficon_getpath.h,v 1.2 2006/08/08 13:31:33 fab Exp $
 */

STRPTR deficon_getpath(STRPTR, int, STRPTR);
void deficon_updatepath(void);

#define DEFICONPATH_FILE "ENV:DefIcon_Path"

#endif /* AMBIENT_DEFICON_GETPATH_H */
