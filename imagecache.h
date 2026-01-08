
#ifndef AMBIENT_IMAGECACHE_H
#define AMBIENT_IMAGECACHE_H

ULONG imagecache_init( void );
void imagecache_cleanup( void );
APTR imagecache_getbitmap( CONST_STRPTR name, ULONG height );
void imagecache_releasebitmap(APTR bitmap);


#endif /* AMBIENT_IMAGECACHE_H */
