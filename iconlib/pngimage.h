#ifndef AMBIENT_ICONLIB_PNGIMAGE_H
#define AMBIENT_ICONLIB_PNGIMAGE_H
/*
 * $Id
 */

#if USE_ICONLIB_PNGLIB

struct OwnDiskObject;

ULONG pngimage_create(STRPTR name, struct OwnDiskObject *odo);
void pngimage_delete(struct OwnDiskObject *odo);

#endif

#endif /* AMBIENT_ICONLIB_PNGIMAGE_H */
