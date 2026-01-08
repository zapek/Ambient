#ifndef AMBIENT_PNGICON_PNG_CALLS_H
#define AMBIENT_PNGICON_PNG_CALLS_H
/*
 * $Id: png_calls.h,v 1.3 2006/02/22 14:48:22 fab Exp $
 */

#ifdef __MORPHOS__
struct Library;
#include <exec/types.h>
#if USE_SHARED_LIBPNG
#include <ppcinline/png.h>
#else
#include "libs/pnglib/ppcinline/png.h"
#endif

#ifndef __NOLIBBASE__
extern struct Library *
#ifdef __CONSTLIBBASEDECL__
__CONSTLIBBASEDECL__
#endif /* __CONSTLIBBASEDECL__ */
PNGLibBase;
#endif /* !__NOLIBBASE__ */
#endif

#endif /* AMBIENT_LIBPNG_PNG_CALLS_H */
