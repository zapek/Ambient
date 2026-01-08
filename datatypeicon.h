#ifndef AMBIENT_DATATYPEICON_H
#define AMBIENT_DATATYPEICON_H
/*
 * $Id: datatypeicon.h,v 1.3 2006/02/22 14:48:19 fab Exp $
 */

struct Screen;

ULONG datatypeicon_read(STRPTR filename, APTR obj, ULONG end, struct Screen *scr);

#endif /* AMBIENT_DATATYPEICON_H */
