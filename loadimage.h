#ifndef AMBIENT_LOADIMAGE_H
#define AMBIENT_LOADIMAGE_H
/*
 * $Id: loadimage.h,v 1.4 2006/08/08 13:31:35 fab Exp $
 */

ULONG tr_loadimage(APTR obj, CONST_STRPTR path);
ULONG tr_scaleimage(APTR obj, APTR dtp, ULONG width, ULONG height);

#endif /* AMBIENT_LOADIMAGE_H */
