#ifndef AMBIENT_FORMAT_H
#define AMBIENT_FORMAT_H
/*
 * $Id: format.h,v 1.4 2006/08/08 13:31:34 fab Exp $
 */

#ifdef __STDC__
struct device_info;
#endif

/*
 * Flags
 */
#define FB_ICON             0
//#define FB_FFS_INTL         1
#define FB_SFS_CASE         2
#define FB_SFS_RECYCLED     3
#define FB_SFS_SHOWRECYCLED 4

#define FF_ICON             (1L << FB_ICON)
//#define FF_FFS_INTL         (1L << FB_FFS_INTL)
#define FF_SFS_CASE         (1L << FB_SFS_CASE)
#define FF_SFS_RECYCLED     (1L << FB_SFS_RECYCLED)
#define FF_SFS_SHOWRECYCLED (1L << FB_SFS_SHOWRECYCLED)

ULONG tr_format(APTR obj, CONST_STRPTR label, struct device_info *di, ULONG mode, ULONG fs, ULONG flags);

#endif /* AMBIENT_FORMAT_H */
