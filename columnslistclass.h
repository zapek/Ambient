#ifndef AMBIENT_DEVICELISTCLASS_H
#define AMBIENT_DEVICELISTCLASS_H
/*
 * $Id: columnslistclass.h,v 1.1 2006/04/12 14:01:53 fab Exp $
 */

/* describe a listclass column, partially (only used attributes are given : COL, MIW, MAW, H) */
struct column_entry {
	ULONG  col;            /* column number */
	ULONG  msgid;          /* locale id number */
	STRPTR name;           /* column name, must refer to a static value */
	ULONG  hasminwidth;    /* is minwidth set ? */
	LONG   minwidth;
	ULONG  hasmaxwidth;    /* is maxwidth set ? */
	LONG   maxwidth;
	ULONG  hidden;         /* is it hidden ? */
	ULONG  sort_column;    /* is this column used to sort ? */
	LONG   sort_direction; /* sort direction */
};

#endif
