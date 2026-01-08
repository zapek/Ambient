#ifndef AMBIENT_SMARTREQ_H
#define AMBIENT_SMARTREQ_H
/*
 * $Id: smartreq.h,v 1.10 2016/08/09 18:51:26 itix Exp $
 */

#ifndef BUILD_ICONLIB
#include "ambient_cat.h"
#endif

struct reqmsg {
	struct Message msg;
	LONG retval;
};

/*
 * Some specific error titles.
 */
enum {
	SRT_NONE,
	SRT_DOSERROR,
	SRT_IOERROR,
};

ULONG smartreq_init(void);
void smartreq_cleanup(void);
ULONG smartreq_request(APTR obj, APTR winobj, CONST_STRPTR title, ULONG methodid, LONG userdata, CONST_STRPTR gadgets, LONG type, CONST_STRPTR format, ...);
ULONG smartreq_request_sync(APTR winobj, CONST_STRPTR title, CONST_STRPTR gadgets, LONG type, CONST_STRPTR format, ...);
#define smartreq_info(title, type, format, ...) smartreq_request(NULL, NULL, (CONST_STRPTR)title, 0, 0, GSI(MSG_OK_REQ), type, format, __VA_ARGS__)

VOID smartreq_ok(size_t title, size_t type, size_t format, ...);

ULONG smartreq_replacefile_sync(APTR winobj, CONST_STRPTR title, CONST_STRPTR gadgets, LONG type, CONST_STRPTR from, CONST_STRPTR to, CONST_STRPTR format, ...);

ULONG tr_smartreq_examine(APTR obj, APTR data, CONST_STRPTR file1, CONST_STRPTR file2);

#endif /* AMBIENT_SMARTREQ_H */
