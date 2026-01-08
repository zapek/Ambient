#ifndef AMBIENT_IPC_H
#define AMBIENT_IPC_H
/*
 * $Id: ipc.h,v 1.7 2015/10/26 21:18:02 itix Exp $
 */

#include <dos/dos.h>
#ifndef BUILD_WBLIB
#include "iconlib/icon_internal.h"
#endif

extern ULONG ipcsig;

struct ipcmessage {
	struct Message msg;
	APTR pool;
	ULONG type;
	APTR msgtype; /* type dependent message (user allocated) */
};

struct IpcData {
	struct SignalSemaphore sema;
	struct MinList appicon_list;
	struct Task *task;
	ULONG sigbit;
};

extern struct IpcData ipcdata;

struct IpcAppiconNode {
	struct MinNode n;
	struct ipcmessage *entry;
	BOOL newentry;     /* entry is started to being handled by ambient. can't be remove by wblib */
	BOOL removedentry; /* entry is scheduled for removal */
	BOOL loadedentry;  /* icon is done loading. it can be removed */
};

/* type (add at the *bottom*, this API is semi-public) */
enum {
	IPC_ADDAPPICON,
	IPC_REMAPPICON,
	IPC_ADDAPPMENU,
	IPC_ADDAPPWINDOW, /* REMOVED */
	IPC_NEWPATH,
	IPC_WBSTARTLIB,
	IPC_WINCOUNT,
	IPC_REMAPPWINDOW, /* REMOVED */
	IPC_REMAPPMENU,
	IPC_CLONEPATH,
};

/* msgtypes */
struct ipc_appicon {
	ULONG id;                   /* user supplied */
	ULONG userdata;             /* user supplied */
	STRPTR text;                /* user supplied */
	struct MsgPort *userport;   /* user supplied */
	struct OwnDiskObject *diskobj; /* user supplied */
	/* .. add Olsen's tag crap somewhere */
};

struct ipc_appwindow {
	ULONG id;                 /* user supplied */
	ULONG userdata;           /* user supplied */
	struct Window *window;    /* user supplied */
	struct MsgPort *userport; /* user supplied */
	/* XXX: add tagitem list perhaps.. check what crap Olsen added */
};

struct ipc_wbstartlib {
	STRPTR filename;
	ULONG *type;
	LONG *pri;
	ULONG *stacksize;
	STRPTR defaulttool;
	ULONG status;
};

struct ipc_newpath {
	BPTR pathlock;
};

struct ipc_wincount {
	ULONG wincount;
	ULONG wbr;
};

struct ipc_clonepath {
	BPTR *path;
};

ULONG ipc_init(void);
void ipc_cleanup(void);
void ipc_handle(void);
ULONG tr_ipc_handle(APTR app);

#endif /* AMBIENT_IPC_H */
