#ifndef AMBIENT_MATH_SQRT_H
#define AMBIENT_MATH_SQRT_H
/*
 * $Id: math_sqrt.h,v 1.3 2006/02/22 14:48:20 fab Exp $
 */

#if USE_FPU
#include <math.h>
#define math_sqrt(v) (LONG)sqrt((double)(v))
#else
LONG math_sqrt(LONG val);
#endif

ULONG fsqrt(ULONG x);

#endif /* AMBIENT_MATH_SQRT_H */
