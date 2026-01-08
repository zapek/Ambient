#ifndef AMBIENT_THREAD_DISPATCHER_H
#define AMBIENT_THREAD_DISPATCHER_H
/*
 * $Id: thread_dispatcher.h,v 1.2 2015/08/12 12:14:26 itix Exp $
 */

#if USE_THREADPOOL
struct thread_msg {
	struct MinNode n;
	struct Process *proc;
	APTR destobj;
	ULONG state;
	LONG pri;
	size_t work_id;
	ULONG action;
	ULONG status;
	APTR obj;
	struct TagItem *taglist;
};
#else
struct thread_node {
	struct MinNode n;
	struct Process *proc;
	struct MsgPort *mp;
	APTR destobj;
	ULONG action;
	ULONG state;
	LONG pri;
	LONG index;
};

struct thread_msg {
	struct Message msg;
	ULONG id;               /* startmsg or not */
	struct thread_node *tn; /* pointer to thread_node to avoid walking the list */
	ULONG action;
	ULONG status;
	APTR obj;
	struct TagItem *taglist;
};
#endif

ULONG thread_domsg(struct thread_msg *msg);

#endif /* AMBIENT_THREAD_DISPATCHER_H */
