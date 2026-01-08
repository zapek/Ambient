#ifndef AMBIENT_WBSTART_H
#define AMBIENT_WBSTART_H
/*
 * $Id: wbstart.h,v 1.8 2007/02/11 22:35:32 fab Exp $
 */

#include <dos/dos.h>

struct WBArg;
struct ipcmessage;

/*
 * wbstart() tags.
 */
enum {
	WBSTARTTAG_FromLib = TAG_USER + 1, /* started from wbstart.library */
	WBSTARTTAG_DirLock,                /* BPTR; optional dirlock */
	WBSTARTTAG_Priority,               /* LONG; priority (default: 0) */
	WBSTARTTAG_Stack,                  /* ULONG; stack size (default: automatically computed) */
	WBSTARTTAG_Argument,               /* STRPTR; argument (this tag can be specified multiple times) */
	WBSTARTTAG_WBArgs,                 /* struct WBArg *; pointer to wbargs, mutualy exclusive with WBSTARTAG_Argument */
	WBSTARTTAG_Argument_List,          /* struct MinList *; list of wbsnode* nodes with arguments. */
	WBSTARTTAG_WBArgsCount,            /* because we can't know size of wbargs array it's passed here. bit sucky.. */
};

/*
 * node for arguments list.
 */

struct wbsnode {
	struct MinNode n;
	TEXT arg[0];
};

ULONG wbstart_init(void);
void  wbstart_cleanup(void);

ULONG v_wbstart(CONST_STRPTR filename, struct TagItem *tags);
ULONG wbstart(CONST_STRPTR filename, ...);
void wbstart_handle(void);

void launch_get_icon_properties(struct ipcmessage *msg);
ULONG get_icon_properties(CONST_STRPTR filename, ULONG *type, LONG *pri, ULONG *stacksize, STRPTR defaulttool);
ULONG preclose_wbstart(void);
#if USE_STUNTZIHACK
void castrate_wbstartport(void);
#endif
ULONG tr_wbstart(CONST_STRPTR filename, LONG pri, ULONG stack, STRPTR arg1, struct MinList *arglist, ULONG freenames);

extern ULONG wbstartsig;

#endif /* AMBIENT_WBSTART_H */
