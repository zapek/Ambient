#ifndef AMBIENT_WBSTARTLIB_H
#define AMBIENT_WBSTARTLIB_H
/*
 * $Id: wbstartlib.h,v 1.3 2006/02/22 14:48:23 fab Exp $
 */

#if USE_WBSTARTLIB

#define WBStart_Name           (TAG_USER + 1)  /* const char *       */
#define WBStart_DirectoryName  (TAG_USER + 2)  /* const char *       */
#define WBStart_DirectoryLock  (TAG_USER + 3)  /* BPTR               */
#define WBStart_Stack          (TAG_USER + 4)  /* ULONG              */
#define WBStart_Priority       (TAG_USER + 5)  /* LONG               */
#define WBStart_ArgumentCount  (TAG_USER + 6)  /* ULONG              */
#define WBStart_ArgumentList   (TAG_USER + 7)  /* struct WBArg *     */

ULONG wbstartlib_init(void);
void wbstartlib_cleanup(void);

ULONG preclose_wbstartlib(void);

#endif

#endif /* AMBIENT_WBSTARTLIB_H */
