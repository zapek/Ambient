#ifndef AMBIENT_DOSREQ_H
#define AMBIENT_DOSREQ_H
/*
 * $Id: dosreq.h,v 1.3 2006/02/22 14:48:19 fab Exp $
 */

ULONG dosreq_init(void);
void dosreq_cleanup(void);
APTR dosreq_disable(void);
void dosreq_enable(APTR w);

#endif /* AMBIENT_DOSREQ_H */
