#ifndef AMBIENT_MEM_H
#define AMBIENT_MEM_H
/*
 * $Id: iconmem.h,v 1.8 2025/09/03 15:15:46 piru Exp $
 */

#if !USE_MEMTRACK
#include <proto/exec.h>
#endif

ULONG iconmem_init(void);
void iconmem_cleanup(void);


#ifdef BUILD_ICONLIB
#define icon_malloc(s) FreeAlloc(fl, s, MEMF_ANY)
extern VOID _FreeFree(struct FreeList *freelist, APTR mem);
#define icon_free(p) _FreeFree(fl, p)
#else
#if USE_MEMTRACK
#include "memtrack.h"
#define icon_malloc(size) memtrack_malloc(BASE_NAME, __FUNC__, __LINE__, MFUNC_ICONMEM, size)
#define icon_free(ptr) memtrack_free(BASE_NAME, __FUNC__, __LINE__, MFUNC_ICONMEM, ptr)
#else
extern APTR iconpool;
#define icon_malloc(size) AllocVecPooled(iconpool, size)
#define icon_free(ptr) FreeVecPooled(iconpool, ptr)
#endif
#endif

#if USE_ICON_MEMLIST
void icon_checkmem(void);
#endif

#endif /* AMBIENT_MEM_H */
