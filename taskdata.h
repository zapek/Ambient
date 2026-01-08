#ifndef AMBIENT_TASKDATA_H
#define AMBIENT_TASKDATA_H

/*
 * $Id: taskdata.h,v 1.3 2006/02/22 14:48:23 fab Exp $
 */

struct taskdata {
	struct MsgPort *msport;     /* methodstack port */
	struct MsgPort *reqport;    /* requester port */
};

#endif /* AMBIENT_TASKDATA_H */
