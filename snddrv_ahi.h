#ifndef AMBIENT_SNDDRV_AHI_H
#define AMBIENT_SNDDRV_AHI_H
/*
 * $Id: snddrv_ahi.h,v 1.4 2006/08/08 13:31:36 fab Exp $
 */

APTR snddrv_ahi_open(const struct TagItem *tags);
void snddrv_ahi_close(APTR ctx);
LONG snddrv_ahi_write_async(APTR ctx, CONST_APTR buf, ULONG len);
void snddrv_ahi_abort(APTR ctx);
ULONG snddrv_ahi_wait(APTR ctx);
ULONG snddrv_ahi_check(APTR ctx);

#endif /* AMBIENT_SNDDRV_AHI_H */
