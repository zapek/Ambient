#ifndef AMBIENT_ALTIVEC_H
#define AMBIENT_ALTIVEC_H
/*
 * $Id: ambient_altivec.h,v 1.1 2017/07/25 20:36:32 piru Exp $
 */

extern ULONG has_altivec;
#if USE_ALTIVEC
extern ULONG use_altivec;
#endif

ULONG altivec_init(void);
void altivec_cleanup(void);

#endif /* AMBIENT_ALTIVEC_H */
