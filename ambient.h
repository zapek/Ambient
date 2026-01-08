#ifndef AMBIENT_H
#define AMBIENT_H
/*
 * $Id: ambient.h,v 1.11 2020/08/16 03:15:18 jacadcaps Exp $
 */

/* no libamiga stupid list macros */
#define NO_AMIGA_LISTS 1
/* but use these */
#define AROS_ALMOST_COMPATIBLE

#include <exec/types.h>
#include <intuition/classusr.h>

#if !USE_MEMTRACK
#include <proto/exec.h>
#endif

/*
 * min()/max()/abs() without macro side effects.
 */
#define max(a,b) \
	({typeof(a) _a = (a); \
	typeof(b) _b = (b);	\
	_a > _b ? _a : _b;})

#define min(a,b) \
	({typeof(a) _a = (a); \
	typeof(b) _b = (b); \
	_a > _b ? _b : _a;})

#define abs(a) \
	({typeof(a) _a = (a); \
	_a < 0 ? -_a : _a;})

#define pos(x) \
	({typeof(x) _x = (x); \
	_x > 0 ? _x : 0;})

#define neg(x) \
	({typeof(x) _x = (x); \
	_x < 0 ? _x : 0;})

#define minmax(a,x,b) (max((a),min((x),(b))))

#define swap(a,b) \
	({typeof(a) _swp = a; \
	a = b; b = _swp;})

#include "config.h"

#include <macros/compilers.h>
#include <macros/vapor.h>

#include "debug.h"

#include "errormsg.h"

#define memclr(_x, _y) memset(_x, '\0', _y)

/*
 * Some functions return TRUE, FALSE
 * and ASYNC.
 */
#define ASYNC 2

/*
 * Frequently used functions
 */
#define USE_BUILTIN_MATH
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*
 * General memory allocation
 */
ULONG malloc_init(void);
void malloc_cleanup(void);
#if USE_MEMTRACK
#include "memtrack.h"
#define malloc(size) memtrack_malloc(BASE_NAME, __FUNC__, __LINE__, MFUNC_MALLOC, size)
#define free(ptr) memtrack_free(BASE_NAME, __FUNC__, __LINE__, MFUNC_MALLOC, ptr)
#else
extern APTR mpool;
#define malloc(size) AllocVecPooled(mpool, size)
#define free(ptr) do { if(ptr) FreeVecPooled(mpool, ptr); } while(0);
#endif

ULONG xget(Object *object, ULONG attr);

#endif /* AMBIENT_H */
