/*
 * $Id: clipboarddevice.h,v 1.1 2006/09/18 23:17:27 fab Exp $
 */

#ifndef CLIPBOARDDEVICE_H
#define CLIPBOARDDEVICE_H

#include <devices/clipboard.h>

struct IOClipReq * clipboard_open(ULONG unit);
void  clipboard_close(struct IOClipReq *ior);

ULONG clipboard_write_ftxt(struct IOClipReq *ior, CONST_STRPTR string);

#endif
