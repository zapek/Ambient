#ifndef AMBIENT_ZUTILS_H
#define AMBIENT_ZUTILS_H
/*
 * $Id: zutils.h,v 1.1 2007/02/11 22:35:32 fab Exp $
 */

#if (USE_GLOWICONS32 || USE_PNGICONS) && !BUILD_ICONLIB
void *z_alloc(void *p, int items, int size);
void z_free(void *p, void *addr);
#endif

#endif /* AMBIENT_ZUTILS_H */
