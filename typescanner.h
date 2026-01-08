#ifndef AMBIENT_TYPESCANNER_H
#define AMBIENT_TYPESCANNER_H
/*
 * $Id: typescanner.h,v 1.8 2020/08/16 03:15:18 jacadcaps Exp $
 */

#include <intuition/classusr.h>

struct typescannernode {
	struct MinNode n;
	APTR obj;
	TEXT path[0];
};

APTR gettype(CONST_STRPTR path, ULONG seconds, CONST_STRPTR mimetypepattern);
APTR typescanner_get(CONST_STRPTR path, ULONG seconds, CONST_STRPTR mimetypepattern, BOOL useCache);

ULONG tr_typescanner_scan_list(APTR obj, struct MinList *l, ULONG gen_thumbs, ULONG gen_deficons);
ULONG tr_typescanner_scan_entry(APTR obj, CONST_STRPTR path, Object *mimetype);
ULONG tr_typescanner_scan_matchfirst(APTR obj, struct MinList *l, Object *mimetype);

#endif /* AMBIENT_TYPESCANNER_H */
