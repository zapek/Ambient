#ifndef AMBIENT_APPMESSAGE_H
#define AMBIENT_APPMESSAGE_H
/*
 * $Id: appmessage.h,v 1.4 2006/08/08 13:31:33 fab Exp $
 */

struct Window;
struct WBArg;
struct MsgPort;

ULONG tr_appmessage_send(ULONG type, struct MsgPort *tmp, struct Window *win, CONST_STRPTR path, ULONG cl, ULONG id, ULONG userdata, ULONG numargs, struct WBArg *arglist, ULONG mousex, ULONG mousey);

#endif /* AMBIENT_APPMESSAGE_H */
