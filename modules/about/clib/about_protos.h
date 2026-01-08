#ifndef CLIB_ABOUT_MOD_PROTOS_H
#define CLIB_ABOUT_MOD_PROTOS_H
// $Id: about_protos.h,v 1.2 2005/07/04 23:06:13 laire Exp $

APTR About_GetPongClass(void);
APTR About_GetLogo(ULONG type, ULONG *width, ULONG *height, ULONG *depth);
STRPTR About_GetText(ULONG type);

#endif /* CLIB_ABOUT_MOD_PROTOS_H */
