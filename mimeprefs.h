#ifndef AMBIENT_MIMEPREFS_H
#define AMBIENT_MIMEPREFS_H
/*
 * $Id: mimeprefs.h,v 1.3 2006/02/22 14:48:21 fab Exp $
 */

extern APTR mimeprefsctx;

ULONG mimeprefs_init(void);
void mimeprefs_cleanup(void);

void mimeprefs_lock_shared(void);
void mimeprefs_unlock(void);
void mimeprefs_lock(void);

ULONG mimeprefs_load(void);
ULONG tr_savemime(APTR pctx);

#endif /* AMBIENT_MIMEPREFS_H */
