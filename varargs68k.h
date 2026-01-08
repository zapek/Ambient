#ifndef AMBIENT_VARARGS68K_H
#define AMBIENT_VARARGS68K_H

/*
 * Add 68k varargs function prototypes to this file and the build system
 * automatically generates new stubs.
 *
 * $Id: varargs68k.h,v 1.2 2014/01/08 19:26:54 rzookol Exp $
 */

/* action.h */
void actionnode_setattrs( APTR actionnode, ... );
void commandnode_setattrs(APTR commandnode, ...);

/* datatypes_picture.h */
APTR datatypes_picture_create(CONST_STRPTR filename, ...);


APTR reggae_picture_create(CONST_STRPTR filename, ...);
/* errorreq.h */
LONG errorreq(CONST_STRPTR title, CONST_STRPTR body, CONST_STRPTR gadgets, ...);

/* file_func.h */
LONG systemtags(CONST_STRPTR cmd, ...);

/* gfx_bitmap.h */
APTR gfx_bitmap_create(ULONG width, ULONG height, ULONG depth, ...);

/* gfx_blit.h */
void gfx_blit(APTR src, APTR dst, ...);

/* gfx_scale.h */
ULONG gfx_scale(APTR sbm, APTR tbm, ULONG txs, ULONG tys, ...);

/* history.h */
void history_setattrs( APTR history, ... );
ULONG historynode_setattrs( APTR node, ... );

/* iconio.h */
ULONG icon_read(STRPTR filename, APTR obj, ...);

/* layout.h */
APTR layout_create(APTR grp, ...);
void layout_setattrs(APTR ctx, ...);

/* mimetype.h */
void mimetype_setattrs(APTR ctx, ...);

/* mimeuri.h */
ULONG mimeuri_gather(APTR ctx, CONST_STRPTR uri, ...);
void mimeuri_setattrs(APTR ctx, ...);

/* notify.h */
ULONG notify_register(APTR ctx, ...);
void notify_unregister(APTR ctx, ...);

/* sound.h */
APTR sound_create(ULONG mode, ...);

/* subdata.h */
void subdata_setattrs(APTR ctx, ...);

/* textbox.h */
APTR textbox_create(struct atextfont *afont, ...);
void textbox_setattrs(APTR ctx, ...);

/* threads.h */
ULONG do_action(APTR obj, int action, ...);
ULONG do_action_sync(APTR obj, int action, ...);

/* wbstart.h */
ULONG wbstart(CONST_STRPTR filename, ...);

#endif /* AMBIENT_VARARGS68K_H */
