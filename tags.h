#ifndef AMBIENT_TAGS_H
#define AMBIENT_TAGS_H
/*
 * $Id: tags.h,v 1.4 2006/08/08 13:31:36 fab Exp $
 */

struct TagItem *tags_clone(const struct TagItem *srcti);
void tags_free(struct TagItem *srcti);
ULONG tags_nth_tagdata(Tag tagval, ULONG defaultval, const struct TagItem *taglist, ULONG num);

#endif /* AMBIENT_TAGS_H */
