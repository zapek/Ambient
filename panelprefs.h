#ifndef AMBIENT_PANELPREFS_H
#define AMBIENT_PANELPREFS_H
/*
 * $Id: panelprefs.h,v 1.4 2012/08/02 15:37:40 geit Exp $
 */

ULONG panelprefs_init(void);
void panelprefs_cleanup(void);

void panelprefs_loadall(void);
void panelprefs_loaded(void);

void panelprefs_save(APTR pctx);

void panelprefs_add(APTR obj,APTR prefspool);
ULONG panelprefs_getnewnum(void);
void panelprefs_getname(APTR pctx,STRPTR name);

ULONG tr_panels_loadall(void);
ULONG tr_panels_save(APTR obj, APTR pctx);
ULONG tr_panels_delete(APTR obj, APTR pctx);

void panelprefs_fix(APTR pctx);

#endif /* AMBIENT_PANELPREFS_H */
