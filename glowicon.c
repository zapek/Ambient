/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2007 Ambient Open Source Team
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 * $Id: glowicon.c,v 1.10 2025/07/24 01:26:38 geit Exp $
 */

#include "ambient.h"

#if USE_GLOWICONS || BUILD_ICONLIB

/* public */
#include <proto/asyncio.h>
#if !BUILD_ICONLIB && USE_SHARED_LIBZ
#include <proto/z.h>
#endif
#include <libraries/iffparse.h>

/* private */
#ifdef BUILD_ICONLIB
#include <exec/memory.h>
#include <proto/icon.h>
#else
#include "methodstack.h"
#include "gfx_mask.h"
#include "gfx_alpha.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "byterun1.h"
#endif
#include "classes.h"
#include "glowicon.h"
#include "iconmem.h"
#include "file_io.h"
#include "zutils.h"
#if !BUILD_ICONLIB && !USE_SHARED_LIBZ
#include "zlib.h"
#include "libs/zlib/ppcinline/z.h"

extern struct Library *ZLibBase;
#endif

/*
 * This code is fragile at best. Should add all sort of
 * error handling. ATM it'll just topple over. - piru
 */

/*
 * This code could be optimized but glowicons support is
 * not very important. - itix
 */

/*
 * Disk structures
 */
#pragma pack(2)
struct chunk_header {
	ULONG id;
	ULONG len;
};

struct form_chunk_header {
	struct chunk_header header;
	ULONG type;
};


struct face_chunk {
	UBYTE width;          /* icon width - 1 */
	UBYTE height;         /* icon height - 1 */
	UBYTE flags;          /* see below */
	UBYTE aspect;         /* aspect ratio: x is upper nibble and y is lower nibble */
	UWORD maxpalettesize; /* this value is completely useless and unreliable. I have no idea of what it can be used for */
};

#define FCF_FRAMELESS (1UL << 0) /* icon has no frame */

struct image_chunk {
	UBYTE transparentcolornum; /* number of the transparent color */
	UBYTE colornum;            /* number of colors in the palette - 1 */
	UBYTE flags;               /* see below */
	UBYTE imageformat;         /* image storage format */
	UBYTE paletteformat;       /* palette storage format */
	UBYTE depth;               /* image depth */
	UWORD imagesize;           /* in bytes - 1 */
	UWORD palettesize;         /* in bytes - 1 */

	/* image and palette data follows */
};
#pragma pack()

/*
 * Structures above are a PITA because you can
 * easily overflow their boundary with the - 1
 * arithmetic stuff. Besides LONG alignement is
 * faster anyway.
 */
struct face_chunk_sane {
	ULONG width;
	ULONG height;
	ULONG flags;
	ULONG aspect;
};

struct image_chunk_sane {
	ULONG transparentcolornum;
	ULONG colornum;
	ULONG flags;
	ULONG imageformat;
	ULONG paletteformat;
	ULONG depth;
	ULONG imagesize;
	ULONG palettesize;
};

#define ICF_TRANSPARENT (1UL << 0) /* the image has a transparent color */
#define ICF_PALETTE     (1UL << 1) /* the image has palettedata following imagedata */

#define ICFORMAT_NORMAL   0 /* no compression */
#define ICFORMAT_BYTERUN1 1 /* byterun1 compression */


#define ID_ICON MAKE_ID('I','C','O','N')
#define ID_FACE MAKE_ID('F','A','C','E')
#define ID_IMAG MAKE_ID('I','M','A','G')
#define ID_ARGB MAKE_ID('A','R','G','B')

/*
 * Brr, braindead format can have the palette anywhere.
 */
struct glowicon {
	struct face_chunk_sane fc;
	UBYTE *imagedec;   /* decompression buffer */
	UBYTE *palettedec; /* decompression buffer */
	ULONG colornum;
};


#if BUILD_ICONLIB
static ULONG glowicon_read_image(APTR fh, APTR obj UNUSED, struct glowicon *gi, ULONG mode UNUSED, struct FreeList *fl UNUSED)
#else
static ULONG glowicon_read_image(APTR fh, APTR obj UNUSED, struct glowicon *gi UNUSED, ULONG mode UNUSED)
#endif
{
	struct chunk_header ch;
	ULONG retval = FALSE;

	/*
	 * Reading chunk header.
	 */
	if (file_read(fh, &ch, sizeof(ch)))
	{
		struct image_chunk icl;

		D(ICONIO, bug("chunkheader: ch.id: 0x%lx, ch.len: 0x%lx\n", ch.id, ch.len));

		if (ch.len > sizeof(icl))
		{
			if (file_read(fh, &icl, sizeof(icl)))
			{
				#ifndef BUILD_ICONLIB
				APTR bm = NULL;
				#endif

				if (ch.id == ID_IMAG)
				{
					struct image_chunk_sane ic;
					/*
					 * Correct the braindead fields.
					 */
					ic.transparentcolornum = icl.transparentcolornum;
					ic.colornum            = icl.colornum + 1;
					ic.flags               = icl.flags;
					ic.imageformat         = icl.imageformat;
					ic.paletteformat       = icl.paletteformat;
					ic.depth               = icl.depth;
					ic.imagesize           = icl.imagesize + 1;
					ic.palettesize		   = 0; // bitRocky: set it to 0, because its nowhere initialized!?

					if (ic.flags & ICF_PALETTE)
					{
						ic.palettesize = icl.palettesize + 1;
					}

					D(ICONIO, bug("width: %ld, height: %ld, colornum: %ld, imagesize: %ld bytes, palettesize: %ld bytes\n", gi->fc.width, gi->fc.height, ic.colornum, ic.imagesize, ic.palettesize));

					#ifdef BUILD_ICONLIB
					file_seek(fh, (ic.imagesize + ic.palettesize + ((ic.imagesize + ic.palettesize) & 1 ? 1 : 0)), MODE_CURRENT);
					retval = TRUE;
					#else
					if ( ((ic.imageformat  == ICFORMAT_NORMAL || ic.imageformat   == ICFORMAT_BYTERUN1) &&
						  (ic.paletteformat == ICFORMAT_NORMAL || ic.paletteformat == ICFORMAT_BYTERUN1) ) )
					{
						ULONG calcsize;

						calcsize = sizeof(icl) + ic.imagesize;

						if (ic.flags & ICF_PALETTE)
						{
							D(ICONIO, bug("image has palette\n"));
							calcsize += ic.palettesize;
						}

						if (calcsize <= ch.len) /* should be == but some icons are buggy */
						{
							APTR mbm;

							if (ic.flags & ICF_PALETTE)
							{
								gi->colornum = ic.colornum;
							}
							else
							{
								if (!gi->colornum)
								{
									D(ICONIO, bug("argh! no palette!\n"));
									/* XXX */
								}
							}

							if (ic.imageformat == ICFORMAT_NORMAL)
							{
								D(ICONIO, bug("image not compressed (raw)\n"));
								if (file_read(fh, gi->imagedec, ic.imagesize))
								{
									D(ICONIO, bug("read properly in NORMAL mode\n"));
								}
							}
							else
							{
								UBYTE *buf;

								/*
								 * Unpack.
								 */
								if ( (buf = icon_malloc(ic.imagesize)) )
								{
									if (file_read(fh, buf, ic.imagesize))
									{
										D(ICONIO, bug("image compressed: image read (%ld bytes).. decompressing..\n", ic.imagesize));
										byterun1_decode(buf, ic.imagesize, ic.depth, gi->imagedec, gi->fc.width * gi->fc.height * sizeof(UBYTE)); /* XXX: check retcode */
										#ifdef DEBUG
										if (db_a[DB_DUMPIMAGE].active)
										{
											dump_image(gi->imagedec, gi->fc.width * gi->fc.height * sizeof(UBYTE), 32);
										}
										#endif /* DEBUG */
									}
									icon_free(buf);
								}
								else
								{
									D(ICONIO, bug("failed!\n"));
								}
							}

							if (ic.flags & ICF_PALETTE)
							{
								if (gi->palettedec)
								{
									icon_free(gi->palettedec);
								}

								if ( (gi->palettedec = icon_malloc(gi->colornum * 3 * sizeof(UBYTE))) )
								{
									if (ic.paletteformat == ICFORMAT_NORMAL)
									{
										D(ICONIO, bug("palette not compressed (raw)\n"));
										if (ic.palettesize >= gi->colornum * 3 * sizeof(UBYTE))
										{
											if (file_read(fh, gi->palettedec, ic.palettesize))
											{
												D(ICONIO, bug("palette read properly in normal mode\n"));
											}
											else
											{
												D(ICONIO, bug("failed to read the palette\n"));
												/* XXX */
											}
										}
										else
										{
											D(ICONIO, bug("no palette! argh\n"));
										}
											/* XXX: error */
									}
									else
									{
										UBYTE *buf;

										/*
										 * Unpack.
										 */
										if ( (buf = icon_malloc(ic.palettesize)) )
										{
											if (file_read(fh, buf, ic.palettesize))
											{
												D(ICONIO, bug("palette compressed: palette read (%ld bytes).. decompressing..\n", ic.palettesize));
												byterun1_decode(buf, ic.palettesize, 8, gi->palettedec, gi->colornum * 3 * sizeof(UBYTE));
												#ifdef DEBUG
												if (db_a[DB_DUMPIMAGE].active)
												{
													dump_image(gi->palettedec, gi->colornum * 3 * sizeof(UBYTE), 3);
												}
												#endif /* DEBUG */
											}
											icon_free(buf);
										}
										else
										{
											D(ICONIO, bug("failed\n"));
										}
									}
								}
								else
								{
									/* XXX: failure */
									D(ICONIO, bug("failed!\n"));
								}
							}

							/*
							 * Read 1 more byte to pad if needed as
							 * chunks need to have a 2-byte alignement.
							 */
							if ((ic.imagesize + ic.palettesize) & 1)
							{
								file_seek(fh, 1, MODE_CURRENT); /* XXX */
							}

							if ( (bm = gfx_bitmap_create(gi->fc.width, gi->fc.height, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE)) )
							{
								gfx_blit(gi->imagedec, bm,
									BLITTAG_SrcType, BLITVAL_SrcType_Array,
									BLITTAG_CMAP, gi->palettedec,
									BLITTAG_CMAPFormat, BLITVAL_CMAPFormat_RGB8,
								TAG_DONE);
								/* XXX: wrong, change if we are the 2nd imagery using palette from the first one */
							}

							if (ic.flags & ICF_TRANSPARENT)
							{
								D(ICONIO, bug("is transparent, color %ld, creating mask: width: %ld, height: %ld\n", ic.transparentcolornum, gi->fc.width, gi->fc.height));
								if ( (mbm = gfx_mask_create_chunky8(gi->imagedec, gi->fc.width, gi->fc.height, ic.transparentcolornum)) )
								{
									gfx_alpha_set_mask(bm, 0, 0, gi->fc.width, gi->fc.height, mbm, 0xff, TRUE);
									gfx_bitmap_delete(mbm);
								}
								/* XXX */
							}
							else
							{
								gfx_alpha_set(bm, 0, 0, gi->fc.width, gi->fc.height, 0xff);
							}

							retval = TRUE;
						}
					}
					#endif
				}
				#if USE_GLOWICONS32
				else if (ch.id == ID_ARGB)
				{
					ULONG imagesize, readsize;
					#ifndef BUILD_ICONLIB
					UBYTE *buf;
					#endif

					imagesize = icl.imagesize + 1;
					readsize = imagesize & 1 ? imagesize + 1 : imagesize;

					/*
					 * Unpack.
					 */
					#ifndef BUILD_ICONLIB
					if ((buf = icon_malloc(readsize)))
					{
						D(ICONIO, bug("image compressed: image read (%ld bytes).. decompressing..\n", imagesize));

						if (file_read(fh, buf, readsize))
						{
							z_stream stream;

							stream.next_in = buf;
							stream.avail_in = imagesize;
							stream.next_out = gi->imagedec;
							stream.avail_out = gi->fc.width * gi->fc.height * sizeof(ULONG);
							stream.zalloc = (alloc_func)&z_alloc;
							stream.zfree = (free_func)&z_free;

							if (inflateInit(&stream) == Z_OK)
							{
								inflate(&stream, Z_FINISH);
								inflateEnd(&stream);
							}
						}
						icon_free(buf);
					}
					else
					{
						D(ICONIO, bug("failed!\n"));
					}

					if ((bm = gfx_bitmap_create(gi->fc.width, gi->fc.height, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE)) )
					{
						gfx_blit(gi->imagedec, bm,
							BLITTAG_SrcType, BLITVAL_SrcType_Array,
							BLITTAG_SrcFormat, BLITVAL_SrcFormat_ARGB,
							TAG_DONE);
					}

					#else
					file_seek(fh, readsize, MODE_CURRENT); /* XXX */
					#endif

					retval = TRUE;
				}
				#endif

				#ifndef BUILD_ICONLIB
				if (bm)
				{
					methodstack_push_sync(obj, 4,
						MM_Icon_AddBitMap, bm, MV_Icon_BitMap_Glowicon, mode
					);
				}
				#endif
			}
		}
	}
	return (retval); /* XXX: but we should tell that the chunk is just not recognized.. */
}


#ifdef BUILD_ICONLIB
ULONG glowicon_read(APTR fh, APTR obj, struct FreeList *fl)
#else
ULONG glowicon_read(APTR fh, APTR obj)
#endif
{
	struct glowicon *gi;
	ULONG retval = FALSE;

	THREAD;
	ASSERT(fh);
	CHECKOBJECT(obj);
	#ifdef BUILD_ICONLIB
	ASSERT(fl);
	#endif

	if ( (gi = icon_malloc(sizeof(struct glowicon))) )
	{
		struct form_chunk_header fch;

		memset(gi, 0, sizeof(*gi));
		/*
		 * Search for 'FORM' chunk header with type 'ICON'.
		 */
		if (file_read(fh, &fch, sizeof(fch)))
		{
			D(ICONIO,bug("did read some data..0x%lx\n", fch.header.id));
			if (fch.header.id == ID_FORM && fch.type == ID_ICON && fch.header.len)
			{
				struct chunk_header ch;
				D(ICONIO,bug("found FORM chunk\n"));
				/*
				 * We must have a FACE chunk.
				 */
				if (file_read(fh, &ch, sizeof(ch)))
				{
					if (ch.id == ID_FACE && ch.len == sizeof(struct face_chunk))
					{
						struct face_chunk fc;
						/*
						 * Found, now read it.
						 */
						D(ICONIO, bug("found FACE chunk\n"));
						if (file_read(fh, &fc, sizeof(fc)))
						{
							/*
							 * Correct the fields. Enough of this lame storage method.
							 */
							gi->fc.width  = fc.width + 1;
							gi->fc.height = fc.height + 1;
							gi->fc.flags  = fc.flags;
							gi->fc.aspect = fc.aspect;

							D(ICONIO, bug("width %ld, height %ld\n", gi->fc.width, gi->fc.height));

							/*
							 * Allocate a decompression buffer. If no decompression is
							 * needed, the buffer is used anyway. Since both image have
							 * the same resulting size, the allocation is done here.
							 */
							if ( (gi->imagedec = icon_malloc(gi->fc.width * gi->fc.height * sizeof(ULONG))) )
							{
								#ifdef BUILD_ICONLIB /* reading the image is just needed to find the size, inefficient */
								if (glowicon_read_image(fh, obj, gi, MV_Icon_BitMap_Normal, fl))
								#else
								if (glowicon_read_image(fh, obj, gi, MV_Icon_BitMap_Normal))
								#endif
								{
									#ifdef BUILD_ICONLIB
									glowicon_read_image(fh, obj, gi, MV_Icon_BitMap_Selected, fl); /* XXX: check retval */
									#else
									glowicon_read_image(fh, obj, gi, MV_Icon_BitMap_Selected); /* XXX: check retval */
									#endif
									retval = TRUE;
								}
								else
								{
									D(ICONIO, bug("out of mem\n"));
									/* XXX */
								}
								if (gi->palettedec)
								{
									icon_free(gi->palettedec);
								}
								icon_free(gi->imagedec);
							}
							else
							{
								D(ICONIO, bug("out of mem\n"));
								/* XXX */
							}

						}
					}
				}
			} /* no error */
		}
		icon_free(gi);
	}
	else
	{
		D(ICONIO, bug("out of mem!\n"));
	}
	return (retval);
}

#endif /* USE_GLOWICONS */
