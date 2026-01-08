#ifndef AMBIENT_MIMETAGS_H
#define AMBIENT_MIMETAGS_H
/*
 * $Id: mimetags.h,v 1.4 2006/08/08 13:31:35 fab Exp $
 */

#define TAGF_STRING ((ULONG)(1<<30)) /* our mimetag system recognizes strings to copy with that flag */

/*
 * Normal tags, 32-bit wide.
 */
enum {
	MIMETAG_Dummy = TAG_USER,
	MIMETAG_FSCONTEXT_PRIORITY,
	MIMETAG_FSCONTEXT_ID,
};


/*
 * String tags, to copy.
 */
enum {
	MIMETAG_DummyCopy = TAG_USER | TAGF_STRING,
	MIMETAGS_FSCONTEXT_STARTUPSTRING,
};

struct TagItem *mimetags_clone(const struct TagItem *srcti);
void mimetags_free(struct TagItem *srcti);

#endif /* AMBIENT_MIMETAGS_H */
