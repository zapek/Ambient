#ifndef AMBIENT_SNAPSHOTLIST_H
#define AMBIENT_SNAPSHOTLIST_H
/*
 * $Id: snapshotlist.h,v 1.3 2006/02/22 14:48:22 fab Exp $
 */

struct MinList;

struct snapshotnode {
	struct MinNode n;
	LONG x;
	LONG y;
	TEXT name[0];
};

ULONG tr_snapshot_list(struct MinList *l);

#endif /* AMBIENT_SNAPSHOTLIST_H */
