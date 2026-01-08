#ifndef AMBIENT_PANELEFFECTS_H
#define AMBIENT_PANELEFFECTS_H
/*
 * $Id: paneleffects.h,v 1.3 2006/02/22 14:48:21 fab Exp $
 */

struct Window;

ULONG tr_panels_move(APTR obj, struct Window *win, ULONG x, ULONG y);
ULONG tr_panels_zip(APTR obj, struct Window *win, ULONG xs, ULONG ys, ULONG reversed, ULONG zipspeed);

#endif /* AMBIENT_PANELEFFECTS_H */
