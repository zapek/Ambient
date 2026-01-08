#ifndef AMBIENT_SNAPSHOT_H
#define AMBIENT_SNAPSHOT_H
/*
 * $Id: snapshot.h,v 1.4 2006/08/08 13:31:36 fab Exp $
 */

ULONG tr_snapshot_icon(CONST_STRPTR filename, LONG x, LONG y);
ULONG tr_snapshot_window(CONST_STRPTR filename, LONG x, LONG y, ULONG xs, ULONG ys, ULONG flags);

#endif /* AMBIENT_SNAPSHOT_H */
