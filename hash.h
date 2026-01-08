#ifndef AMBIENT_HASH_H
#define AMBIENT_HASH_H
/*
 * $Id: hash.h,v 1.6 2008/04/02 20:24:20 kiero Exp $
 */

ULONG hash(CONST_STRPTR s);
ULONG hash_nocase(CONST_STRPTR s);
ULONG hash_nocase_seconds(CONST_STRPTR s, ULONG seconds);
ULONG sdbm_hash(CONST_STRPTR s);

#endif /* AMBIENT_HASH_H */
