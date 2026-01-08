#ifndef AMBIENT_NEWICON_H
#define AMBIENT_NEWICON_H
/*
 * $Id: newicon.h,v 1.4 2006/08/08 13:31:35 fab Exp $
 */

#if USE_NEWICONS

struct BitMap;

ULONG newicon_find(APTR fh, UBYTE *tt);
ULONG newicon_read_image(APTR fh, APTR obj, ULONG mode, ULONG ancillary);

extern const TEXT newiconstart[41];

#endif /* USE_NEWICONS */

#endif /* AMBIENT_NEWICON_H */
