
#ifndef AMBIENT_CACHE_H
#define AMBIENT_CACHE_H
/*
 * $Id: cache.h,v 1.3 2021/12/23 01:17:47 piru Exp $
 */


enum {
	CACHETAG_MIMETYPE = 0,      /* stores pointer to internalmimetypenode struct */
	CACHETAG_THUMBNAIL,         /* stores pointer to internal bitmap struct */
	CACHETAG_MAX,
};

#define CACHETAG_ALL -1         /* only for invalidation */

ULONG cache_init( void );
void cache_cleanup( void );

void cache_set( ULONG hash, LONG tag, APTR val );
LONG cache_get( ULONG hash, LONG tag, APTR *val );
void cache_unlock( void );
void cache_invalidate( ULONG hash, LONG tag );
void cache_deleteunused( void );


#endif
