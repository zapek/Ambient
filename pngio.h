#ifndef AMBIENT_PNGIO_H
#define AMBIENT_PNGIO_H
/*
 * $Id: pngio.h,v 1.11 2016/08/11 11:36:54 itix Exp $
 */

extern const ULONG pngicon_id;

void cleanup_zlib(void);

ULONG pngio_sigvalid(CONST_STRPTR s);

enum
{
	PNGIO_STRICT = 0,
	PNGIO_ANYFILE,
	PNGIO_NO_IMAGE
};

APTR pngio_create(CONST_STRPTR name, ULONG anyformat);
ULONG pngio_save(CONST_STRPTR name, APTR ctx, APTR ofh);
void pngio_delete(APTR ctx);

ULONG pngio_tag_add(APTR ctx, ULONG tag, CONST_APTR data);
void pngio_tag_delete(APTR ctx, ULONG tag);

ULONG pngio_add_bitmap(APTR ctx, APTR bm, ULONG image_number);

#ifdef BUILD_ICONLIB
//ULONG crc32(ULONG crc, UBYTE *buf, ULONG len);
APTR pngio_get_chunkdata(APTR ctx, ULONG id, ULONG *size);
#endif

#ifndef BUILD_ICONLIB
ULONG pngio_finalize_iconless(APTR ctx);
ULONG pngio_has_image(APTR ctx);
#endif

#endif /* AMBIENT_PNGIO_H */
