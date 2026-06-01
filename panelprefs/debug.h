#ifndef DEBUG_H
#define DEBUG_H

/*
 * $Id: debug.h,v 1.1 2026/02/08 13:43:39 kronos Exp $
 */

#ifdef DEBUG

#include <clib/debug_protos.h>

#define DB_ICONIO 0

#ifdef __GNUC__ /* GCC */
#define __FUNC__ __FUNCTION__
#endif

void dprintf(char *, ...) __attribute__ ((format (printf, 1, 2)));

// If You Find a reason it should be dprintf() anyway, change this!
//#define kprintf dprintf

/*
 * Straight debug
 *
 * Example: DB(("you suck %ld times\n", num));
 */
#define DB(x)   { kprintf(__FILE__ "[%4ld]/%s() : ",__LINE__,__FUNC__); kprintf x ; }


/*
 * Straight debug with 1 second delay
 *
 * Example: DBD(("you suck %ld times and slowly\n", num));
 */
#define DBD(x)  { Delay(50); kprintf(__FILE__ "[%4ld]:%s() : ",__LINE__,__FUNC__); kprintf x ; }

/*
 * Permanent debug
 */
#define PDB(x) { kprintf(__FILE__ "[%4ld]/%s() : ",(ULONG)__LINE__,__FUNC__); kprintf x ; }

/*
 * assertions
 */
#define ASSERT(x) { if (!(x)) { kprintf("*** assertion failed at: " __FILE__ "[%4ld]/%s(), var: %s\n",(ULONG)__LINE__, __FUNC__, #x); } }

#define D(cl,x) if (DB_##cl) { x; }
#define bug kprintf(__FILE__ "[%4ld]/%s() : ",(ULONG)__LINE__,__FUNC__); kprintf

#define CHECKOBJECT(_o)
#define THREAD

#else

#define D(cl,x)
#define bug
#define ASSERT(x)
#define DB(x)
#define PDB(x)
#define DBL(lvl,x)
#define DBD(x)
#define CHECKOBJECT(_o)
#define THREAD

#endif /* !DEBUG */

#endif /* DEBUG_H */
