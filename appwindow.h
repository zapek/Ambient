#ifndef AMBIENT_APPWINDOW_H
#define AMBIENT_APPWINDOW_H
/*
 * $Id: appwindow.h,v 1.3 2006/02/22 14:48:18 fab Exp $
 */

struct ipc_appwindow;
struct Window;

ULONG appwindows_init(void);
void appwindows_cleanup(void);

struct ipc_appwindow * appwindow_obtain(struct Window *win);
void appwindow_release(void);
ULONG appwindow_add(struct ipc_appwindow *msg);
ULONG appwindow_remove(struct ipc_appwindow *msg);

#endif /* AMBIENT_APPWINDOW_H */
