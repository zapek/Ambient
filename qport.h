#ifndef AMBIENT_QPORT_H
#define AMBIENT_QPORT_H
/*
 * $Id: qport.h,v 1.1 2006/04/12 14:01:56 fab Exp $
 */

#ifdef __STDC__
struct MsgPort;
#endif

void CreateQPort(struct MsgPort *port);
void DeleteQPort(struct MsgPort *port);

#endif /* AMBIENT_QPORT_H */
