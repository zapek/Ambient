#ifndef AMBIENT_CPU_VEC_H
#define AMBIENT_CPU_VEC_H
/*
 * $Id: cpu_vec.h,v 1.4 2006/08/08 13:31:33 fab Exp $
 */

#if USE_ALTIVEC
void cpu_vec_dst(CONST_APTR ptr, ULONG opts, ULONG channel);
void cpu_vec_dstt(CONST_APTR ptr, ULONG opts, ULONG channel);
void cpu_vec_dstst(CONST_APTR ptr, ULONG opts, ULONG channel);
void cpu_vec_dststt(CONST_APTR ptr, ULONG opts, ULONG channel);
void cpu_vec_dss(ULONG channel);
#endif

#endif /* AMBIENT_CPU_VEC_H */
