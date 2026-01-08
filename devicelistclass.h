#ifndef AMBIENT_DEVICELISTCLASS_H
#define AMBIENT_DEVICELISTCLASS_H
/*
 * $Id: devicelistclass.h,v 1.1 2006/02/22 14:48:19 fab Exp $
 */

struct volume_entry {
	STRPTR name;
	BOOL   hidden;
};

struct volume_entry * volumeentry_build(STRPTR name);
void   volumeentry_delete(struct volume_entry * ve);

#endif
