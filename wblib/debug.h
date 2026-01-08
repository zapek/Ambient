#ifndef DEBUG_H
#define DEBUG_H

/*
 * $Id: debug.h,v 1.3 2006/08/08 13:31:43 fab Exp $
 */
void dprintf(char *, ...) __attribute__ ((format (printf, 1, 2)));

#ifdef DEBUG

#ifdef __GNUC__ /* GCC */
#define __FUNC__ __FUNCTION__
#endif

void dprintf(char *, ...) __attribute__ ((format (printf, 1, 2)));

#define kprintf dprintf

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
 * assertions
 */
#define ASSERT(x) { if (!(x)) { kprintf("*** assertion failed at: " __FILE__ "[%4ld]/%s(), var: %s\n",(ULONG)__LINE__, __FUNC__, #x); } }

#define D(cl,x) if (DB_##cl) { x; }
#define bug kprintf(__FILE__ "[%4ld]/%s() : ",(ULONG)__LINE__,__FUNC__); kprintf

#else

#define D(cl,x)
#define bug
#define ASSERT(x)
#define DB(x)
#define DBL(lvl,x)
#define DBD(x)

#endif /* !DEBUG */

#endif /* DEBUG_H */
