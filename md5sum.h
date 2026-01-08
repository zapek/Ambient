#ifndef AMBIENT_MD5SUM_H
#define AMBIENT_MD5SUM_H
/*
 * $Id: md5sum.h,v 1.5 2006/09/18 23:17:29 fab Exp $
 */

ULONG tr_md5sum(APTR obj, CONST_STRPTR path);
ULONG tr_md5sum_list(APTR obj, struct MinList * l);

#endif /* AMBIENT_MD5SUM_H */
