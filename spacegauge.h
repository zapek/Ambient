#ifndef AMBIENT_SPACEGAUGE_H
#define AMBIENT_SPACEGAUGE_H
/*
 * $Id: spacegauge.h,v 1.2 2006/09/18 23:17:31 fab Exp $
 */

#include <exec/types.h>

void spacegauge_draw( struct RastPort *drawrp, LONG origin_x, LONG xw, LONG origin_y, LONG percent, ULONG alpha);

#endif /* AMBIENT_SPACEGAUGE_H */
