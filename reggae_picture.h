#ifndef AMBIENT_REGGAE_PICTURE_H
#define AMBIENT_REGGAE_PICTURE_H
/*
 * $Id: reggae_picture.h,v 1.3 2015/08/14 22:42:40 itix Exp $
 */


APTR reggae_picture_create(CONST_STRPTR filename, ...);
APTR v_reggae_picture_create(CONST_STRPTR filename, struct TagItem *tags);
APTR reggae_picture_getattr(APTR ctx, ULONG attr);
STRPTR reggae_picture_errorstring(LONG err);
APTR reggae_picture_clone_bm(APTR ctx);

#endif /* AMBIENT_reggae_PICTURE_H */
