#ifndef AMBIENT_THUMBS_H
#define AMBIENT_THUMBS_H
/*
 * $Id: thumbs.h,v 1.6 2008/05/05 18:46:35 kiero Exp $
 */

#if USE_THUMBS

#define TCF_CACHE  (1 << 0UL)
#define TCF_CREATE (1 << 1UL)

struct thumbnode {
	struct MinNode n;
	APTR o;
	TEXT path[0];
};

ULONG thumb_validate(STRPTR path);
ULONG picture_validate(STRPTR path, APTR mimetype);

APTR tr_thumb_createbitmap(STRPTR path, APTR mimetype, ULONG flags);
ULONG tr_thumb_createicon(APTR obj, APTR o, STRPTR path, ULONG flags);
ULONG tr_thumb_createicon_list(APTR obj, struct MinList *l, ULONG flags);

ULONG thumb_init(void);
void thumb_cleanup(void);

#endif

#endif /* AMBIENT_THUMBS_H */
