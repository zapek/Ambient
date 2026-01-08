#ifndef AMBIENT_PREFS_DESKTOP_H
#define AMBIENT_PREFS_DESKTOP_H
/*
 * $Id: prefs_desktop.h,v 1.5 2007/02/11 22:35:30 fab Exp $
 */

#include "prefs.h"

/* Hidden drives (listpool) */
#define DDSI_LISTPOOL_HIDDENDRIVE              (DSI_GROUP(1) | DSF_LISTPOOL)
#define DDSI_LISTPOOL_HIDDENDRIVE_NAME         (1)
#define DDSI_LISTPOOL_HIDDENDRIVE_ICONX        (2)
#define DDSI_LISTPOOL_HIDDENDRIVE_ICONY        (3)
#define DDSI_LISTPOOL_HIDDENDRIVE_SHOWN        (4)

/* My MorphOS */
#define DDSI_MYMORPHOS                         (DSI_GROUP(2))
#define DDSI_MYMORPHOS_ICONX                   (DDSI_MYMORPHOS + 1)
#define DDSI_MYMORPHOS_ICONY                   (DDSI_MYMORPHOS + 2)
#define DDSI_MYMORPHOS_WINDOW_LEFT             (DDSI_MYMORPHOS + 3)
#define DDSI_MYMORPHOS_WINDOW_TOP              (DDSI_MYMORPHOS + 4)
#define DDSI_MYMORPHOS_WINDOW_WIDTH            (DDSI_MYMORPHOS + 5)
#define DDSI_MYMORPHOS_WINDOW_HEIGHT           (DDSI_MYMORPHOS + 6)
#define DDSI_MYMORPHOS_WINDOW_FLAGS            (DDSI_MYMORPHOS + 7)
#define DDSI_MYMORPHOS_NAME                    (DDSI_MYMORPHOS + 8)
#define DDSI_MYMORPHOS_SHOW                    (DDSI_MYMORPHOS + 9)
#define DDSI_MYMORPHOS_DOUBLECLICK             (DDSI_MYMORPHOS + 10)

ULONG hiddendrives_init(void);
void hiddendrives_cleanup(void);
ULONG tr_hiddendrives_load(APTR obj);
void hiddendrives_updatedevice(APTR obj, CONST_STRPTR devname, LONG shown, LONG x, LONG y);
ULONG tr_hiddendrives_hidden(CONST_STRPTR devname, LONG *x, LONG *y);

void dprefs_mymorphos_iconpos_get(LONG *x, LONG *y);
void dprefs_mymorphos_iconpos_set(LONG x, LONG y);
void dprefs_mymorphos_window_get(LONG *x, LONG *y, LONG *w, LONG *h, ULONG *flags);
void dprefs_mymorphos_window_set(LONG x, LONG y, LONG w, LONG h, ULONG flags);
ULONG tr_dprefs_save(APTR obj);

CONST_STRPTR dprefs_mymorphos_name_get(void);
void dprefs_mymorphos_name_set(CONST_STRPTR name);
void dprefs_mymorphos_notify_setobj(APTR obj);
ULONG dprefs_mymorphos_show(void);
ULONG dprefs_mymorphos_doubleclick(void);

#endif /* AMBIENT_PREFS_DESKTOP_H */
