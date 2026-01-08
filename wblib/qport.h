#ifndef AMBIENT_WBLIB_QPORT_H
#define AMBIENT_WBLIB_QPORT_H
/*
 * $Id: qport.h,v 1.1 2006/04/12 17:51:47 fab Exp $
 */

#ifdef __STDC__
struct MsgPort;
#endif

void CreateQPort(struct MsgPort *port);
void DeleteQPort(struct MsgPort *port);

#endif /* AMBIENT_WBLIB_QPORT_H */
