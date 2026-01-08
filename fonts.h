#ifndef AMBIENT_FONT_H
#define AMBIENT_FONT_H
/*
 * $Id: fonts.h,v 1.3 2006/02/22 14:48:19 fab Exp $
 */

/*
 * struct TextFont really sucks. Let's
 * extend it.
 */
struct atextfont {
	struct TextFont *tf;
	ULONG style;
};


struct atextfont * font_open(STRPTR name);
void font_close(struct atextfont *atf);
ULONG tr_font_load(APTR obj, STRPTR fontname, ULONG type);

#endif /* AMBIENT_FONT_H */
