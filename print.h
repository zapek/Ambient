#ifndef AMBIENT_PRINT_H
#define AMBIENT_PRINT_H

/*
 * $Id: print.h,v 1.1 2007/05/08 19:27:09 fab Exp $
 */

ULONG tr_print(APTR obj, APTR buffer, ULONG type);

enum
{
	PRT_NONE = 0,
	PRT_BITMAP,
	PRT_TEXT,
};

#endif /* AMBIENT_PRINT_H */
