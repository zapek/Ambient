#ifndef AMBIENT_PNGICON_H
#define AMBIENT_PNGICON_H
/*
 * $Id: pngicon.h,v 1.4 2006/12/20 13:34:15 fab Exp $
 */

#include "pngicon_specs.h"

struct BitMap;
struct FreeList;

#ifdef BUILD_ICONLIB
void pngio_read_tags(APTR obj, UBYTE *data, ULONG len, struct FreeList *fl);
ULONG pngicon_read(APTR fh, APTR obj, struct FreeList *fl);
#else
ULONG pngicon_read(APTR fh, APTR obj, ULONG end, ULONG getimage);
#endif

void cleanup_pnglib(void);

#endif /* AMBIENT_PNGICON_H */
