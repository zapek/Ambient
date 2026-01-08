#ifndef AMBIENT_SERIAL_H
#define AMBIENT_SERIAL_H
/*
 * $Id: serial.h,v 1.3 2006/02/22 14:48:22 fab Exp $
 */

#include <exec/rawfmt.h>

/* XXX: fix this to use args */
#define SERIAL_PRINTF(x) NewRawDoFmt(x, (APTR)RAWFMTFUNC_SERIAL, NULL, NULL)

#endif /* AMBIENT_SERIAL_H */
