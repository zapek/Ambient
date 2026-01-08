#ifndef AMBIENT_IMAGE_H
#define AMBIENT_IMAGE_H
/*
 * $Id: image.h,v 1.3 2006/02/22 14:48:20 fab Exp $
 */

struct BitMap;
struct AsyncFile;

#ifdef BUILD_ICONLIB
ULONG read_image(APTR fh, APTR obj, ULONG state, struct BitMap *fri, struct FreeList *fl);
#else
ULONG remap_image(APTR obj, ULONG state, struct Image *img, ULONG imgwidth, APTR imgdata);
ULONG read_image(APTR fh, APTR obj, ULONG state, ULONG ancillary);
#endif

#endif /* AMBIENT_IMAGE_H */
