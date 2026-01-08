#ifndef AMBIENT_THREADTAGS_H
#define AMBIENT_THREADTAGS_H
/*
 * $Id: threadtags.h,v 1.4 2006/08/08 13:31:36 fab Exp $
 */

struct TagItem *threadtags_clone(const struct TagItem *srcti);
void threadtags_free(struct TagItem *srcti);

#endif /* AMBIENT_THREADTAGS_H */
