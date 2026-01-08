#ifndef DEBUG_H
#define DEBUG_H

/*
 * $Id: debug.h,v 1.1 2015/03/02 13:50:53 geit Exp $
 *
 * Want to add a debug flag? See debug.c, top
 *
 */

#ifdef DEBUG

#include <clib/debug_protos.h>

#ifdef __GNUC__ /* GCC */
#define __FUNC__ __FUNCTION__
#endif

struct debug_flags {
	ULONG  prefs;
	ULONG  active;

	CONST_STRPTR envvar;
};

extern struct debug_flags db_a[];

/* debug flags (new entries at the end) */
#define DB_ICONIO         0
#define DB_PROC           1
#define DB_DUMPIMAGE      2
#define DB_WBSTART        3
#define DB_WBSTARTUP      4
#define DB_REXX           5
#define DB_LAYOUT         6
#define DB_DEVICEIO       7
#define DB_DRAGDROP       8
#define DB_INIT           9
#define DB_ARGS          10
#define DB_CLASS         11
#define DB_LIB           12
#define DB_DOSLISTCACHE  13
#define DB_PATH          14
#define DB_COPY          15
#define DB_DEFICON       16
#define DB_DEFICONPOOL   17
#define DB_EXDIR         18
#define DB_DOMETHOD      19
#define DB_RECURSE       20
#define DB_SCANDIR       21
#define DB_PREFSIO       22
#define DB_LABELSPLIT    23
#define DB_SHORTCUT      24
#define DB_MIMEURI       25
#define DB_RDARGS        26
#define DB_RECOG         27
#define DB_PREFSPOOL     28
#define DB_SNDDRV        29
#define DB_SOUND         30
#define DB_NOTIFY        31
#define DB_MIMEACTIONED  32
#define DB_MIMETYPE      33
#define DB_ADVANCEDPREFS 34
#define DB_ACTIONDISP    35
#define DB_APPICON       36

/* debug actions */
#define DBA_MEMCHECK       0
#define DBA_MEMSTATS       1
#define DBA_MEMSTATS_ALL   2
#define DBA_STARTMEMRECORD 3
#define DBA_STOPMEMRECORD  4

void dprintf(char *, ...) __attribute__ ((format (printf, 1, 2)));

// If You Find a reason it should be dprintf() anyway, change this!
//#define kprintf dprintf

void dump_image(UBYTE *p, ULONG size, ULONG width);

ULONG debug_init(void);
void debug_cleanup(void);

/*
 * Straight debug
 *
 * Example: DB(("you suck %ld times\n", num));
 */
#define DB(x)   { kprintf(BASE_NAME "[%4ld]/%s() : ",(ULONG)__LINE__,__FUNC__); kprintf x ; }

/*
 * Straight debug with 1 second delay
 *
 * Example: DBD(("you suck %ld times and slowly\n", num));
 */
#define DBD(x)  { Delay(50); kprintf(BASE_NAME "[%4ld]:%s() : ",(ULONG)__LINE__,__FUNC__); kprintf x ; }

/*
 * Straight debug without prefix
 */
#define SDB(x)  { kprintf x; }

/*
 * Permanent debug
 */
#define PDB(x) { kprintf(BASE_NAME "[%4ld]/%s() : ",(ULONG)__LINE__,__FUNC__); kprintf x ; }

/*
 * Selective debug
 *
 * Example: D(db_html,bug("you suck HTML %ld times\n", num));
 */
#define D(cl,x) if (db_a[DB_##cl].active) { x; }

#define bug kprintf(BASE_NAME "[%4ld]/%s() : ",(ULONG)__LINE__,__FUNC__); kprintf

/*
 * assertions
 */
#define ASSERT(x) { if (!(x)) { kprintf("*** assertion failed at: " BASE_NAME "[%4ld]/%s(), var: %s\n",(ULONG)__LINE__, __FUNC__, #x); } }

/*
 * Serious errors (aka can't happen)
 */
#define DBS(x,y) { if (x) { kprintf("*********************** SERIOUS ERROR ***********************\n"); kprintf y ; } }

/*
 * Alignements.
 */
#define CHECKALIGN(_x,_y) if ((ULONG)(_x) & ((_y) - 1)) PDB(("not aligned\n"))

/*
 * MUI objects (those are set by MUI debug only!)
 */
struct ObjectDebugHeader {
	ULONG magic;
	APTR self;
};
struct _Object2
{
	struct MinNode  o_Node;
	struct IClass  *o_Class;
};

#define _OBJ2(o)           ((struct _Object2 *)(o))
#define _OBJECT2(o)        (_OBJ2(o)-1)

#define ODH_SIZE sizeof(struct ObjectDebugHeader)
#define ODH_MAGIC MAKE_ID('M','U','I','O')
#define ODH(_o) ((struct ObjectDebugHeader *)(((ULONG)_OBJECT2(_o)) - ODH_SIZE))

#define _ISOBJ(_o) \
	(ODH(_o)->magic == ODH_MAGIC && ODH(_o)->self == _o)

#define CHECKOBJECT(_o) { \
	if (_o) \
	{ \
		if (!_ISOBJ(_o)) \
		{ \
			PDB(("*** not an object: %p, %s\n", _o, (ODH(_o)->magic == 0x77777777) ? "disposed object" : "unknown")); \
		} \
	} \
	else \
	{ \
		PDB(("*** not an object: NULL object\n")); \
	} \
	}

extern struct Task *MainTask;

#define MAINTASK { if (FindTask(NULL) != MainTask) PDB(("not maintask\n")); }
#define THREAD { if (FindTask(NULL) == MainTask) PDB(("not a thread\n")); }

#else

#define DB(x)
#define DBD(x)
#define SDB(x)
#define D(class,x)
#define bug
#define ASSERT(x)
#define DBS(x,y)
#define PDB(x)
#define CHECKALIGN(_x,_y)
#define CHECKOBJECT(_o)
#define MAINTASK
#define THREAD

#endif /* !DEBUG */

#endif /* DEBUG_H */
