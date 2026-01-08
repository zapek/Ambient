#ifndef AMBIENT_PREFS_STARTUP_H
#define AMBIENT_PREFS_STARTUP_H
/*
 * $Id: prefs_startup.h,v 1.1 2007/02/11 22:35:30 fab Exp $
 */

#include "prefs.h"

/* Hidden drives (listpool) */
#define DDSI_LISTPOOL_WINDOW              (DSI_GROUP(1) | DSF_LISTPOOL)
#define DDSI_LISTPOOL_WINDOW_PATH         (1)
#define DDSI_LISTPOOL_WINDOW_LEFT         (2)
#define DDSI_LISTPOOL_WINDOW_TOP          (3)
#define DDSI_LISTPOOL_WINDOW_WIDTH        (4)
#define DDSI_LISTPOOL_WINDOW_HEIGHT       (5)
#define DDSI_LISTPOOL_WINDOW_ICONIFIED    (6)
#define DDSI_LISTPOOL_WINDOW_BROWSER      (7)
#define DDSI_LISTPOOL_WINDOW_LISTER       (8)
#define DDSI_LISTPOOL_WINDOW_MODEINDEX    (9)

ULONG tr_sprefs_load(APTR obj);
VOID sprefs_save(void);
VOID sprefs_setup(void);
VOID sprefs_cleanup(void);
VOID sprefs_resetsave(void);

#endif /* AMBIENT_PREFS_STARTUP_H */
