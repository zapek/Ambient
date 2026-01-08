#ifndef AMBIENT_ATOMIC_H
#define AMBIENT_ATOMIC_H
/*
 * $Id: atomic.h,v 1.3 2006/02/22 14:48:18 fab Exp $
 */


static __inline ULONG atomic_add(volatile ULONG *p, ULONG v)
{
	ULONG t;

	__asm __volatile (
		"1: lwarx %0,0,%1\n" /* load old value */
		"add %0,%0,%2\n"     /* calculate new value */
		"stwcx. %0,0,%1\n"   /* attempt to store */
		"bne- 1b\n"          /* spin if failed */
		"eieio\n"            /* drain to memory */
		: "=&r" (t)
		: "r" (p), "r" (v)
		: "memory");

	return (t);
}


static __inline ULONG atomic_sub(volatile ULONG *p, ULONG v)
{
	ULONG t;

	__asm __volatile (
		"1: lwarx %0, 0, %1\n"  /* load old value */
		"subf %0, %2, %0\n"     /* calculate new value */
		"stwcx. %0, 0, %1\n"    /* attempt to store */
		"bne- 1b\n"             /* spin if failed */
		"eieio\n"               /* drain to memory */
		: "=&r" (t)
		: "r" (p), "r" (v)
		: "memory");

	return (t);
}


#endif /* AMBIENT_ATOMIC_H */
