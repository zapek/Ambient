#ifndef AMBIENT_SUBDATA_H
#define AMBIENT_SUBDATA_H
/*
 * $Id: subdata.h,v 1.4 2007/02/11 22:35:30 fab Exp $
 */

enum {
	SUBDATA_Type = TAG_USER + 1,
	SUBDATA_Description,
};

APTR subdata_create(void);
void subdata_delete(APTR ctx);
void v_subdata_setattrs(APTR ctx, struct TagItem *tags);
void subdata_setattrs(APTR ctx, ...);
APTR subdata_getattr(APTR ctx, ULONG attr);

#endif /* AMBIENT_SUBDATA_H */
