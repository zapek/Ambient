#ifndef AMBIENT_MATH_SQRT_H
#define AMBIENT_MATH_SQRT_H
/*
 * $Id: math_sqrt.h,v 1.1 2015/03/02 13:50:55 geit Exp $
 */

#if USE_FPU
#include <math.h>
#define math_sqrt(v) (LONG)sqrt((double)(v))
#else
LONG math_sqrt(LONG val);
#endif

ULONG fsqrt(ULONG x);

#endif /* AMBIENT_MATH_SQRT_H */
