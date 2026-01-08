#ifndef AMBIENT_WBLIB_CONFIG_H
#define AMBIENT_WBLIB_CONFIG_H
/*
 * $Id: config.h,v 1.3 2006/09/18 02:02:27 piru Exp $
 */

#define USE_LEGACY 0

#ifdef BETA_RELEASE
#if !USE_LEGACY
#define DEBUG 1
#endif
#endif

#endif /* AMBIENT_WBLIB_CONFIG_H */
