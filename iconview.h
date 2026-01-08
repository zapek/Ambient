#ifndef AMBIENT_ICONVIEW_H
#define AMBIENT_ICONVIEW_H
/*
 * $Id: iconview.h,v 1.4 2007/11/13 22:23:23 fab Exp $
 */

enum { /* MV_Icon_ViewMode_* depends on this */
	IVM_NONE,
	IVM_LISTER,
	IVM_ICON,
	IVM_SHOWALL,
	IVM_THUMBS,
};

enum {
	IVS_NAME = 1,
	IVS_SIZE,
	IVS_TYPE,
	IVS_DATE,
};

enum {
	IVSO_INCREMENTAL,
	IVSO_DECREMENTAL,
};

ULONG tr_iconview_showpreview(APTR obj, APTR entry);

#endif /* AMBIENT_ICONVIEW_H */
