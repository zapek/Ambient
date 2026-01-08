#ifndef AMBIENT_DEFICONPOOL_H
#define AMBIENT_DEFICONPOOL_H
/*
 * $Id: deficonpool.h,v 1.6 2007/07/08 20:56:41 fab Exp $
 */

ULONG deficonpool_init(void);
void deficonpool_cleanup(void);
void deficonpool_flush(void);

/*
 * When passed as mimetype value will force it to be checked first.
 * NOTE: Not looked up in cache, so don't use if no needed.
 */

#define DEFICON_MIMETYPE_RECOGNIZE (APTR)(-1UL)

/* */

ULONG deficonpool_apply_default_icon(APTR obj, ULONG type, STRPTR path, APTR mimetype);
STRPTR deficonpool_get_icon_name(ULONG viewid, STRPTR path, ULONG * isdefault);

/* Lowlevel call. subject to change so use with care. Doesn't work for devices yet. */
STRPTR new_deficonpool_build_name(ULONG viewid, STRPTR path_info, APTR mimetype, ULONG *refine, STRPTR buff, ULONG buff_size);

void deficonpool_delete_mime_list(ULONG viewid);
void deficonpool_flush_mime_list(ULONG viewid);


#endif /* AMBIENT_DEFICONPOOL_H */
