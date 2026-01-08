#ifndef AMBIENT_PREFSCLONE_H
#define AMBIENT_PREFSCLONE_H
/*
 * $Id: prefsclone.h,v 1.3 2006/02/22 14:48:22 fab Exp $
 */

/*
 * Including this header changes the behaviour of functions
 * manipulating preferences. They work on the clone part then.
 */
#ifdef AMBIENT_PREFS_H
#error Argh, you included prefs.h and prefsclone.h.. only include one of them and make sure you understand why
#endif

#undef AMBIENT_PREFSCLONE_H
#include "prefs.h"
#define AMBIENT_PREFSCLONE_H

#undef getprefs
#define getprefs getprefs_clone

#undef getprefslong
#define getprefslong getprefslong_clone

#undef getprefsstr
#define getprefsstr getprefsstr_clone

#undef setprefs
#define setprefs(id, size, data) setprefs_ctx(cloneprefspool, id, size, data)

#undef setprefslong
#define setprefslong(id, v) setprefslong_ctx(cloneprefspool, id, v)

#undef setprefsstr
#define setprefsstr(id, data) setprefsstr_ctx(cloneprefspool, id, data)

#endif /* AMBIENT_PREFSCLONE_H */
