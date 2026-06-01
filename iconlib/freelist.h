#ifndef AMBIENT_ICONLIB_FREELIST_H
#define AMBIENT_ICONLIB_FREELIST_H
/*
 * $Id: freelist.h,v 1.4 2025/09/03 15:08:21 piru Exp $
 */

struct FreeList * create_freelist(struct FreeList *freelist);
VOID _FreeFree(struct FreeList *freelist, APTR mem);

#endif /* AMBIENT_ICONLIB_FREELIST_H */
