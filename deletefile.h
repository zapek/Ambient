#ifndef AMBIENT_DELETEFILE_H
#define AMBIENT_DELETEFILE_H
/*
 * $Id: deletefile.h,v 1.6 2007/02/11 22:35:28 fab Exp $
 */

ULONG deletefile(APTR obj, APTR refwin, CONST_STRPTR path, APTR progressobj, ULONG * filecount, BOOL noicon, ULONG *flags);

#endif /* AMBIENT_DELETEFILE_H */

