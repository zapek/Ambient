#ifndef AMBIENT_PNGLIB_GLOBALS_H
#define AMBIENT_PNGLIB_GLOBALS_H
/*
 * $Id: globals.h,v 1.2 2005/07/04 23:06:11 laire Exp $
 */

#define USE_LEGACY 1

#ifdef BETA_RELEASE
#if !USE_LEGACY
#define DEBUG 1
#endif
#endif

#include "debug.h"

#include <macros/compilers.h>
#include <exec/types.h>

#endif /* AMBIENT_PNGLIB_GLOBALS_H */
