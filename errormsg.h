#ifndef AMBIENT_ERRORMSG_H
#define AMBIENT_ERRORMSG_H
/*
 * $Id: errormsg.h,v 1.3 2006/02/22 14:48:19 fab Exp $
 */

enum {
	ERR_NOMEM,
	ERR_READERROR,
	ERR_WRITEERROR,
};

#ifdef BUILD_ICONLIB
#define errormsg(x)
#else
void errormsg(ULONG type);
#endif

#endif /* AMBIENT_ERRORMSG_H */
