#ifndef AMBIENT_ALIGN_VEC_H
#define AMBIENT_ALIGN_VEC_H
/*
 * $Id: align_vec.h,v 1.5 2006/08/08 13:31:33 fab Exp $
 */

#if USE_ALTIVEC

static inline VECTOR_ULONG vec_ld_align(CONST_APTR ptr)
{
	VECTOR_ULONG v0;
	VECTOR_ULONG v1;
	VECTOR_UBYTE valign;
	CONST ULONG *p;

	p = ptr;

	if ((ULONG)p & 15)
	{
		v0 = vec_ld(0, p);
		v1 = vec_ld(16, p);
		valign = vec_lvsl(0, p);

		return (vec_perm(v0, v1, valign));
	}
	else
	{
		return (vec_ld(0, p));
	}
}

#endif

#endif /* AMBIENT_ALIGN_VEC_H */
