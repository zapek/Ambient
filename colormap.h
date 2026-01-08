#ifndef AMBIENT_COLORMAP_H
#define AMBIENT_COLORMAP_H
/*
 * $Id: colormap.h,v 1.3 2006/02/22 14:48:19 fab Exp $
 */

extern struct ColorMap *magicwb_cm;

ULONG magicwb_cm_init(void);
void magicwb_cm_cleanup(void);

#endif /* AMBIENT_COLORMAP_H */
