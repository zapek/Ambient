#ifndef AMBIENT_APPICON_H
#define AMBIENT_APPICON_H
/*
 * $Id: appicon.h,v 1.5 2008/11/07 01:22:07 kiero Exp $
 */

struct ipc_appicon;
struct ipcmessage;
struct WBArg;


void appicon_synchronize(void);
void appicon_markasloaded(APTR appicon);

ULONG tr_appicon_read(APTR obj, APTR o, struct ipc_appicon *msg);

#endif /* AMBIENT_APPICON_H */
