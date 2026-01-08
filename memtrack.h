#ifndef AMBIENT_MALLOC_H
#define AMBIENT_MALLOC_H
/*
 * $Id: memtrack.h,v 1.4 2006/08/08 13:31:35 fab Exp $
 */

enum {
	MFUNC_MALLOC,
	MFUNC_ICONMEM,

	MFUNC_MAXSYS, /* *** DO NOT INSERT ANYTHING AFTER THAT *** */
};

#define NUMMFUNC MFUNC_MAXSYS

APTR memtrack_malloc(STRPTR s, STRPTR f, ULONG line, ULONG subsys, size_t size);
void memtrack_free(STRPTR s, STRPTR f, ULONG line, ULONG subsys, APTR ptr);
#if USE_MEMTRACK_MEMLIST
void memcheck(void);
enum {
	MEMSTATS_ALL,
	MEMSTATS_TOTAL,
};
void memstats(ULONG mode);
#endif
#if USE_MEMTRACK_RECORD
void memtrack_record_start(void);
void memtrack_record_stop(void);
#endif
ULONG memtrack_init(void);
void memtrack_cleanup(void);

#endif /* AMBIENT_MALLOC_H */
