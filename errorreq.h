#ifndef AMBIENT_ERRORREQ_H
#define AMBIENT_ERRORREQ_H
/*
 * $Id: errorreq.h,v 1.6 2007/02/11 22:35:28 fab Exp $
 */

LONG errorreq(CONST_STRPTR title, CONST_STRPTR body, CONST_STRPTR gadgets, ...);
LONG v_errorreq(CONST_STRPTR title, CONST_STRPTR body, CONST_STRPTR gadgets, APTR args);

#endif /* AMBIENT_ERRORREQ_H */
