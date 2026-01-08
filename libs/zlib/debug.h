#ifndef DEBUG_H
#define DEBUG_H

/*
 * $Id: debug.h,v 1.3 2006/08/08 13:31:41 fab Exp $
 */

#ifdef DEBUG

#ifdef __GNUC__ /* GCC */
#define __FUNC__ __FUNCTION__
#endif

#ifdef AMIGAOS
extern void kprintf(char *, ...);
#endif

#ifdef __MORPHOS__
void dprintf(char *, ... );
#define kprintf dprintf
#endif

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


#else

#define DB(x)
#define DBL(lvl,x)
#define DBD(x)

#endif /* !DEBUG */

#endif /* DEBUG_H */
