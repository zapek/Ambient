#ifndef AMBIENT_WBARG_H
#define AMBIENT_WBARG_H
/*
 * $Id: wbarg.h,v 1.4 2006/08/08 13:31:37 fab Exp $
 */

#include <workbench/startup.h>

struct wbargs {
	ULONG count;
	STRPTR basepath;
	struct WBArg wba[0]; /* in that structure all wa_Lock are ignored */
};


struct wbargs *wba_create(APTR obj);
void wba_delete(struct wbargs *wba);

#endif /* AMBIENT_WBARG_H */
