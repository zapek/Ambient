#ifndef AMBIENT_GLOWICON_H
#define AMBIENT_GLOWICON_H
/*
 * $Id: glowicon.h,v 1.3 2006/02/22 14:48:20 fab Exp $
 */

#if USE_GLOWICONS || defined(BUILD_ICONLIB)

struct BitMap;

#ifdef BUILD_ICONLIB
ULONG glowicon_read(APTR fh, APTR obj, struct FreeList *fl);
#else
ULONG glowicon_read(APTR fh, APTR obj);
#endif

#endif /* USE_GLOWICONS */

#endif /* AMBIENT_GLOWICON_H */
