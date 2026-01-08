#ifndef AMBIENT_DOSLISTCACHE_H
#define AMBIENT_DOSLISTCACHE_H
/*
 * $Id: doslistcache.h,v 1.8 2022/12/27 19:39:59 jacadcaps Exp $
 */

#include <dos/dos.h>

extern struct MinList dlclist;

struct dlcnode {
	struct MinNode n;
	STRPTR name;
	LONG type;
	LONG disktype;
	ULONG is_fs;      /* TRUE if it's a filesystem */
	ULONG blocksize;  /* blocksize, filesystem only */
	ULONG flags;
	ULONG state;
	ULONG updcnt;
	ULONG medium;     /* DCL_MEDIUM_#? */
	struct MsgPort *mp;
	struct DateStamp volumedate;
	STRPTR drivername;
	ULONG unitnum;
	ULONG unitflags;
	QUAD size;        /* */
	QUAD free;
};

/* flags */
#define DLF_READONLY    (1 << 0UL) /* NYI */
#define DLF_REMOVABLE   (1 << 1UL) /* Handle SCSI eject commands */
#define DLF_AUXILLARY   (1 << 2UL) /* Ram Disks, floppies */
#define DLF_HIGHSPEED   (1 << 3UL)
#define DLF_DUMDEVICE   (1 << 4UL) /* dummy device node */
#define DLF_UNMOUNTABLE (1 << 5UL) /* Can be killed with ACTION_DIE */

/* is_fs status */
enum {
	DLC_FS_UNKNOWN,
	DLC_FS_FILESYSTEM,
	DLC_FS_OTHER,
};

enum {
	DLC_NEW,
	DLC_ADDED,
	DLC_DEAD,
	DLC_REMOVED,
};

enum {
	DLC_MEDIUM_OTHER,	/* anything else but cdrom */
	DLC_MEDIUM_CDROM,
};

struct dlc_ptr {
	ULONG h;
	ULONG used;
	struct dlcnode *dlcn;
};

/*
 * The following can be used in threads or the main thread. It won't lock.
 */
#define ITERATEDLC(node) doslistcache_update(); for(node=FIRSTNODE(&dlclist);NEXTNODE(node);node=NEXTNODE(node))

/*
 * This routine is like the one above, except that it's safe even when node
 * is potentially removed while walking the list.
 */
#define ITERATEDLCSAFE(node,node2) doslistcache_update(); for(node=FIRSTNODE(&dlclist);(node2=NEXTNODE(node));node=node2)

ULONG doslistcache_init(void);
void doslistcache_cleanup(void);
ULONG doslistcache_update(void);

/*
 * Those must be used exclusively in the main
 * thread.
 */
struct dlcnode * doslistcache_find_dlcdevice(struct MsgPort *mp);
struct dlcnode * doslistcache_find_dlcvolume_by_devicename(CONST_STRPTR name);
struct dlcnode * doslistcache_find_dlcdevice_by_volumename(CONST_STRPTR name);
struct dlcnode * doslistcache_find_dlcdevice_by_devicename(CONST_STRPTR name);

ULONG doslistcache_fastdevice(CONST_STRPTR path);

/*
 * If using the previous from threads is desired,
 * then locking must be done.
 */
void doslistcache_lock(void);
void doslistcache_unlock(void);

#endif /* AMBIENT_DOSLISTCACHE_H */
