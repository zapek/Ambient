#ifndef AMBIENT_SHORTCUTS_H
#define AMBIENT_SHORTCUTS_H
/*
 * $Id: shortcuts.h,v 1.5 2007/02/11 22:35:30 fab Exp $
 */

#if USE_SHORTCUTS

struct MinList;

struct shortcutnode {
	struct MinNode n;
	LONG filetype;
	LONG x;
	LONG y;
	TEXT path[0];
};


ULONG shortcuts_init(void);
void shortcuts_cleanup(void);
ULONG shortcuts_load(void);
ULONG tr_shortcuts_load(APTR obj);
ULONG tr_shortcuts_save(APTR obj, ULONG reporterror);
ULONG tr_shortcuts_add(APTR obj, STRPTR path, LONG x, LONG y, LONG update, LONG filetype);
ULONG tr_shortcuts_list_add(APTR obj, struct MinList *l);

#endif /* USE_SHORTCUTS */

#endif /* AMBIENT_SHORTCUTS_H */
