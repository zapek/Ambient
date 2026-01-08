#ifndef AMBIENT_IOSTDREQ_H
#define AMBIENT_IOSTDREQ_H
/*
 * $Id: iostdreq.h,v 1.1 2006/04/12 14:01:54 fab Exp $
 */

#ifdef __STDC__
struct IOStdReq;
#endif

struct IOStdReq *iostd_open_device(CONST_STRPTR devname, ULONG unit, ULONG flags);
void iostd_close_device(struct IOStdReq *ior);

#endif /* AMBIENT_IOSTDREQ_H */
