#ifndef AMBIENT_LAYOUT_H
#define AMBIENT_LAYOUT_H
/*
 * $Id: layout.h,v 1.8 2007/07/08 20:56:41 fab Exp $
 */

/*
 * Layout search modes (XXX: that stuff has to vanish..)
 */
//enum {
//	  LSM_LEFTRIGHT, /* default, left to right then left to right, etc.. */
//	  LSM_TOPBOTTOM, /* top to bottom then top to bottom, etc.. */
//	  LSM_GROUPED,   /* top left then grows to right bottom while trying to group as much as possible */
//};

APTR v_layout_create(APTR grp, struct TagItem *tags);
APTR layout_create(APTR grp, ...);
void layout_delete(APTR ctx);
void layout_clear(APTR ctx);

ULONG layout_icon_add(APTR ctx, APTR obj);
void layout_icon_remove(APTR ctx, APTR obj);

ULONG layout_sort(APTR ctx);

void layout_done(APTR ctx);

void layout_icon_move(APTR ctx, APTR obj, LONG x, LONG y, LONG grid);

void v_layout_setattrs(APTR ctx, struct TagItem *tags);
void layout_setattrs(APTR ctx, ...);
ULONG layout_getattr(APTR ctx, ULONG attr);

void layout_place_icons(APTR ctx);

enum {
	LAYOUTTAG_Auto = TAG_USER + 1, /* [ISG] autolayout mode (default: TRUE) */
	LAYOUTTAG_Width,               /* [IS.] width in pixels */
	LAYOUTTAG_Height,              /* [IS.] height in pixels */
	LAYOUTTAG_SortMode,            /* [IS.] sorting mode (default: LAYOUTVAL_SortMode_Name) */
	LAYOUTTAG_SortReversed,        /* [IS.] reversed sorting (default: FALSE) */
	LAYOUTTAG_Num,                 /* [IS.] number of objects (default: 0) */
	LAYOUTTAG_Changed,             /* [.SG] the layout needs refresh as it has a new number of column/order, or can be specified manually.. */
	LAYOUTTAG_OriginX,             /* [IS.] layout origin (in pixels). defaults to 0,0*/
	LAYOUTTAG_OriginY,             /* [IS.] */
	LAYOUTTAG_Strategy,            /* [IS.] positioning strategy */
	LAYOUTTAG_WBMode,              /* [IS.] */
	LAYOUTTAG_HSpacing,            /* [IS.] icon horizontal spacing */
	LAYOUTTAG_VSpacing,            /* [IS.] icon vertical spacing (minimum, can be bigger depending on label height)*/
};

enum {
	LAYOUTVAL_SortMode_Name,
	LAYOUTVAL_SortMode_Size,
	LAYOUTVAL_SortMode_Date,
};

#endif /* AMBIENT_LAYOUT_H */
