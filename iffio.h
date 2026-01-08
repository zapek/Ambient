#ifndef AMBIENT_IFFIO_H
#define AMBIENT_IFFIO_H
/*
 * $Id: iffio.h,v 1.3 2006/02/22 14:48:20 fab Exp $
 */

#if USE_IFF_IO
struct ScreenModePrefs * load_screenmode_prefs(STRPTR filename);
#endif /* USE_IFF_IO */

#endif /* AMBIENT_IFFIO_H */
