#ifndef AMBIENT_COMMON_PICTURE_H
#define AMBIENT_COMMON_PICTURE_H
/*
 * $Id: common_picture.h,v 1.5 2018/08/12 20:53:15 itix Exp $
 */

/*
 * common_picture_create() tags.
 */
enum {
	PICTAG_Screen = TAG_USER + 1, /* struct Screen *; */
	PICTAG_VMem,                  /* put it in VRAM, if possible */
	PICTAG_ARGB32,                /* use ARGB32 format */
	PICTAG_ErrorPtr,              /* LONG *; error */
	PICTAG_DataAddress,           /* pointer to file data */
	PICTAG_DataSize,              /* length of input stream */
	PICTAG_Blend                  /* defaults to TRUE */
};

enum {
	PICTURE_BITMAP,
	PICTURE_WIDTH,
	PICTURE_HEIGHT,
};

enum {  
	PICTURE_TYPE_DATATYPES,
	PICTURE_TYPE_REGGAE,
};

struct common_picture {
	APTR picture_o;
	APTR bm;
	LONG type;
	void (*picture_delete)(APTR);
	void (*picture_set_bitmap)(APTR,APTR); 
	STRPTR (*picture_errorstring)(LONG);
};

struct Screen;


APTR picture_getattr(APTR ctx, ULONG attr);
CONST_STRPTR picture_errorstring(LONG err);
void picture_delete(APTR ctx);
void picture_set_bitmap(APTR ctx, APTR bm); 
APTR picture_create(LONG width, LONG height, LONG depth, struct Screen *screen_friend, LONG vmem);


#endif /* AMBIENT_COMMON_PICTURE_H */
