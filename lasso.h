#ifndef AMBIENT_LASSO_H
#define AMBIENT_LASSO_H
/*
 * $Id: lasso.h,v 1.6 2012/05/13 09:26:03 tcheko Exp $
 */

struct BitMap;
struct RastPort;
void lasso_pixel(APTR ctx);
APTR lasso_create(struct RastPort *rp, LONG x, LONG y, LONG minx, LONG miny, LONG maxx, LONG maxy, LONG vx, LONG vy, struct BitMap *fri, ULONG color);
void lasso_delete(APTR ctx);
void lasso_render(APTR ctx, LONG x, LONG y, LONG vx, LONG vy);
void lasso_clear(APTR ctx);
void lasso_invalidate(APTR ctx);

#endif /* AMBIENT_LASSO_H */
