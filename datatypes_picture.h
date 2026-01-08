#ifndef AMBIENT_DATATYPES_PICTURE_H
#define AMBIENT_DATATYPES_PICTURE_H
/*
 * $Id: datatypes_picture.h,v 1.7 2014/01/13 20:54:00 rzookol Exp $
 */

APTR datatypes_picture_create(CONST_STRPTR filename, ...);
APTR v_datatypes_picture_create(CONST_STRPTR filename, struct TagItem *tags);
APTR datatypes_picture_getattr(APTR ctx, ULONG attr);
void datatypes_picture_set_bitmap(APTR ctx, APTR bm);
void datatypes_picture_delete(APTR ctx);
STRPTR datatypes_picture_errorstring(LONG err);
APTR datatypes_picture_clone_bm(APTR ctx);

#endif /* AMBIENT_DATATYPES_PICTURE_H */
