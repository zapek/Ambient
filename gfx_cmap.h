#ifndef AMBIENT_GFX_CMAP_H
#define AMBIENT_GFX_CMAP_H
/*
 * $Id: gfx_cmap.h,v 1.5 2006/08/08 13:31:34 fab Exp $
 */

#include <ppcinline/macros.h>

APTR gfx_cmap_create(CONST ULONG *cmap32, ULONG colornum);
void gfx_cmap_delete(APTR ctx);
ULONG gfx_cmap_remap(APTR sbm, APTR tbm, APTR cm);

#ifndef RemapMapColours
extern struct Library *CGXDitherBase;
#define RemapMapColours(__p0, __p1, __p2, __p3, __p4, __p5) \
        LP6(66, ULONG , RemapMapColours, \
                struct BitMap *, __p0, a0, \
                struct BitMap *, __p1, a1, \
                UBYTE *, __p2, a2, \
                UBYTE *, __p3, a3, \
                struct ColorMap *, __p4, a4, \
                ULONG , __p5, d0, \
                , CGXDitherBase, 0, 0, 0, 0, 0, 0)
#endif

#endif /* AMBIENT_GFX_CMAP_H */
