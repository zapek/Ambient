#ifndef AMBIENT_RECURSE_H
#define AMBIENT_RECURSE_H

/*
 * $Id: recurse.h,v 1.5 2019/02/19 21:57:20 piru Exp $
 */

/*
 * Return that to skip a directory from within 'enterdir'.
 */
#define RECURSE_SKIP 2

ULONG recurse(APTR obj, CONST_STRPTR path, CONST_STRPTR pattern, ULONG (*enterdir)(APTR obj, CONST_STRPTR path, APTR userdata), ULONG (*exitdir)(APTR obj, CONST_STRPTR path, APTR userdata), ULONG (*func)(APTR obj, CONST_STRPTR path, LONG type, ULONG prot, UQUAD size, APTR userdata, CONST_STRPTR comment), APTR userdata);

#endif /* AMBIENT_RECURSE_H */
