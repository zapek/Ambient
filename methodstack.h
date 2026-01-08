#ifndef AMBIENT_METHODSTACK_H
#define AMBIENT_METHODSTACK_H
/*
 * $Id: methodstack.h,v 1.9 2018/09/07 10:59:58 kronos Exp $
*/

struct MsgPort;

ULONG methodstack_init(void);
void methodstack_cleanup(void);

#ifdef BUILD_ICONLIB
#define methodstack_push methodstack_push_sync
#define methodstack_push_sync(...) pushfakemethod(fl, __VA_ARGS__)
void pushfakemethod(struct FreeList *fl, ...);
#else
#if USE_ASYNC_PUSHMETHOD
void methodstack_push(APTR obj, ULONG cnt, ...);
#else
#define methodstack_push methodstack_push_sync
#endif
ULONG methodstack_push_sync(APTR obj, ULONG cnt, ...);
#if 1//def ENABLE_METHODSTACK_PUSHSYNCSAFE
ULONG methodstack_push_sync_safe(APTR obj, ULONG cnt, ...);
#endif
void methodstack_kill_methods(APTR obj);
void methodstack_check(ULONG nobreak);
#endif

#endif /* AMBIENT_METHODSTACK_H */
