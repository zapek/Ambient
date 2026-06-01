#ifndef AMBIENT_PREFSPOOL_H
#define AMBIENT_PREFSPOOL_H
/*
 * $Id: prefspool.h,v 1.5 2025/07/05 12:31:05 kronos Exp $
 */

APTR prefspool_create(ULONG uid);
void prefspool_delete(APTR ctx);
void prefspool_flush(APTR ctx);

ULONG prefspool_uid(APTR ctx);

APTR prefspool_item_add(APTR ctx, APTR pitem, ULONG id, CONST_APTR data, ULONG size);
void prefspool_item_remove(APTR ctx, APTR pitem, ULONG id);
void prefspool_item_id_change(APTR ctx, APTR pitem, ULONG old_id,ULONG new_id);

APTR prefspool_item_get(APTR ctx, APTR pitem, ULONG id, APTR *p, ULONG *size);

/* normal return codes */
#define PREFSPOOL_IO_OK             1 /* [RW] all fine */
#define PREFSPOOL_IO_PROTECTEDMEDIA 2 /* [.W] media is read-only */
#define PREFSPOOL_IO_MISSING        3 /* [R.] prefs file doesn't exist */

/* fatal return codes */
#define PREFSPOOL_IO_ERROR          -1 /* [RW] read/write error */
#define PREFSPOOL_IO_OUTOFMEM       -2 /* [RW] out of memory within prefs functions */
#define PREFSPOOL_IO_INCONSISTENCY  -3 /* [RW] preference file is corrupt (R), data to write is meaningless (W) */
#define PREFSPOOL_IO_NOOPENWRITE    -4 /* [.W] unable to open the file in write mode */
#define PREFSPOOL_IO_NOCREATEDIR    -5 /* [.W] failed to create the requested prefs directory */

LONG prefspool_read(APTR ctx, CONST_STRPTR filename, ULONG prefsid, ULONG report);
LONG prefspool_write(APTR ctx, CONST_STRPTR filename, ULONG prefsid, ULONG report);

ULONG prefspool_copy(APTR ctx_src, APTR ctx_dst, void (*fp)(ULONG id));
APTR prefspool_duplicate(APTR ctx);

#endif /* AMBIENT_PREFSPOOL_H */
