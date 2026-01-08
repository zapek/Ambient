#ifndef AMBIENT_RANDOM_H
#define AMBIENT_RANDOM_H
/*
 * $Id: random.h,v 1.3 2006/02/22 14:48:22 fab Exp $
 */

#if USE_STRONG_RANDOM
#define RANDOMMAX 0xffffffff
#else
#define RANDOMMAX RAND_MAX
#endif

ULONG random_init(void);
void random_cleanup(void);

ULONG random_ulong(void);
UBYTE random_ubyte(void);

#endif /* AMBIENT_RANDOM_H */
