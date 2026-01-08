#ifndef AMBIENT_LIBPNG_Z_CALLS_H
#define AMBIENT_LIBPNG_Z_CALLS_H
/*
 * $Id: z_calls.h,v 1.2 2005/07/04 23:06:11 laire Exp $
 */

#ifdef __MORPHOS__
struct Library;
#include <exec/types.h>
#include "../../zlib/ppcinline/z.h"

#ifndef __NOLIBBASE__
extern struct Library *
#ifdef __CONSTLIBBASEDECL__
__CONSTLIBBASEDECL__
#endif /* __CONSTLIBBASEDECL__ */
ZLibBase;
#endif /* !__NOLIBBASE__ */
#endif

#endif /* AMBIENT_LIBPNG_Z_CALLS_H */
