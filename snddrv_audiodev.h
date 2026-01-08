#ifndef AMBIENT_SNDDRV_AUDIODEV_H
#define AMBIENT_SNDDRV_AUDIODEV_H
/*
 * $Id: snddrv_audiodev.h,v 1.4 2006/08/08 13:31:36 fab Exp $
 */

APTR snddrv_audiodev_open(const struct TagItem *tags);
void snddrv_audiodev_close(APTR ctx);
LONG snddrv_audiodev_write_async(APTR ctx, CONST_APTR buf, ULONG len);
void snddrv_audiodev_abort(APTR ctx);
ULONG snddrv_audiodev_wait(APTR ctx);
ULONG snddrv_audiodev_check(APTR ctx);

#endif /* AMBIENT_SNDDRV_AUDIODEV_H */
