#ifndef AMBIENT_ICON_GLOBALS_H
#define AMBIENT_ICON_GLOBALS_H
/*
 * $Id: globals.h,v 1.7 2017/09/06 00:09:04 nadir Exp $
 */

//#define USE_LEGACY 0
#include "../config.h"

#ifdef BETA_RELEASE
#if !USE_LEGACY
#define DEBUG 1
#endif
#endif

#define USE_ICONLIB_JUMPTABLE 1
#define USE_ICONLIB_PNG 1
#define USE_ICONLIB_MORONCATCHER 0
#define USE_ICONLIB_PNGLIB 1
#define USE_ICONLIB_SVG 1

#include "debug.h"


#include <macros/compilers.h>
#include <exec/types.h>

#endif /* AMBIENT_ICON_GLOBALS_H */
