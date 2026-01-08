#ifndef AMBIENT_ALTIVEC_H
#define AMBIENT_ALTIVEC_H
/*
 * $Id: altivec.h,v 1.1 2015/03/02 13:50:55 geit Exp $
 */

#include "../library.h"

ULONG altivec_init   ( struct AmbientSupportBase *AmbientSupportBase );
void  altivec_cleanup( struct AmbientSupportBase *AmbientSupportBase );

#endif /* AMBIENT_ALTIVEC_H */
