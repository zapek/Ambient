#ifndef MACROS_H
#define MACROS_H

#include <exec/types.h>


#ifndef UNUSED
#define UNUSED __attribute__((unused))
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

#endif /* MACROS_H */
