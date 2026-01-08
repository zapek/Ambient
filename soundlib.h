#ifndef AMBIENT_SOUNDLIB_H
#define AMBIENT_SOUNDLIB_H
/*
 * $Id: soundlib.h,v 1.3 2006/02/22 14:48:22 fab Exp $
 */

#if USE_SOUNDLIB


ULONG soundlib_init(void);
void soundlib_cleanup(void);

ULONG preclose_soundlib(void);

#endif

#endif /* AMBIENT_SOUNDLIB_H */
