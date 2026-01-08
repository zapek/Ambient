#ifndef AMBIENT_SVGICON_H
#define AMBIENT_SVGICON_H
/*
 * $Id: svgicon.h,v 1.8 2020/03/22 01:14:12 piru Exp $
 */

#if USE_SVGICONS || defined(BUILD_ICONLIB)

#include <libraries/vgraphics.h>
#include <graphics/gfx.h>
#include <graphics/rpattr.h>
#include <dos/dos.h>
#include <exec/rawfmt.h>
#include <cybergraphx/cybergraphics.h>

#include <proto/vgraphics.h>

#include <proto/graphics.h>
#include <proto/dos.h>
#include <proto/exec.h>

#ifdef BUILD_ICONLIB
APTR read_svgtags(char *filename, APTR obj, struct FreeList *fl);
APTR svgicon_read(STRPTR filename, APTR obj, ULONG mode, struct FreeList *fl);
#else
APTR read_svgtags(char *filename, Object *obj);
APTR svgicon_read(STRPTR filename, APTR obj, ULONG mode, ULONG ancillary);
#endif

BOOL svg_signature(STRPTR filename);
BOOL svg_signature_buffer(const void *buffer, size_t len);

#endif /* USE_SVGICONS */

#endif /* AMBIENT_SVGICON_H */
