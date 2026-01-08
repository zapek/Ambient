#ifndef AMBIENT_DOSNOTIFY_H
#define AMBIENT_DOSNOTIFY_H
/*
 * $Id: dosnotify.h,v 1.5 2006/08/08 13:31:33 fab Exp $
 */

extern ULONG dosnotifysig;

ULONG dosnotify_init(void);
void dosnotify_cleanup(void);
APTR dosnotify_start(CONST_STRPTR name, ULONG userdata);
void dosnotify_stop(APTR ctx);
void dosnotify_handle(void);

#endif /* AMBIENT_DOSNOTIFY_H */
