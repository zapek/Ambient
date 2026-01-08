/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2016 Ambient Open Source Team
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
 * $Id: pngio.c,v 1.15.10.1 2024/01/18 18:23:23 piru Exp $
 */

#include "ambient.h"

#if USE_PNGICONS || defined(BUILD_ICONLIB)

/* public */
#include <cybergraphx/cybergraphics.h>
#include <proto/cybergraphics.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#if USE_SHARED_LIBZ
#include <proto/z.h>
#elif BUILD_ICONLIB
// Someone broke iconlib build and it was not me. -itix
ULONG crc32(ULONG crc, UBYTE *buf, ULONG len);
#endif

/* private */
#include "file_io.h"
#include "gfx_bitmap.h"
#include "pngio.h"
#include "file_io.h"
#include "pngicon_specs.h"
#include "zutils.h"
#ifndef BUILD_ICONLIB
#if !USE_SHARED_LIBZ
#include "zlib.h"
#include "libs/zlib/ppcinline/z.h"

extern struct Library *ZLibBase;
#endif
#else
#undef malloc
#undef free
#define malloc(x) AllocVec((x), MEMF_ANY)
#define free(x) FreeVec((x))
#endif

struct pngio_taglist {
	struct MinList taglist;
	ULONG totalsize;
};

struct pngio_context {
	struct MinList chunklist[2];
	APTR pool;
	struct pngio_taglist *tl;

	/* used for image encoding only */
	APTR bm;
	ULONG width;
	ULONG height;
};

struct pngio_id {
	ULONG len;
	ULONG type;
};

struct pngio_chunk {
	struct MinNode n;
	ULONG crc_status;
	ULONG size;
	ULONG id;
	UBYTE data[0];
};

#pragma pack(1)
struct pngio_ihdr {
	ULONG width;
	ULONG height;
	UBYTE depth;
	UBYTE colortype;
	UBYTE compression;
	UBYTE filter;
	UBYTE interlace;
};
#pragma pack()

#define CRC_CLEARED 0
#define CRC_OK      1
#define CRC_BAD     2


struct pngio_tag {
	struct MinNode n;
	ULONG tag;
	ULONG size;
	UBYTE data[0];
};


#define IOBUFFERSIZE (8192 * 2)

/*
 * PNG signature.
 */
static const UBYTE png_sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};

const ULONG pngicon_id = MAKE_ID('i','c','O','n');


/*
 * Support functions.
 */


/*
 * Converts all the tags of the 'icOn' taglist into a chunk.
 */
static ULONG pngio_tags_to_chunk(APTR ctx)
{
	struct pngio_context *ct = ctx;
	struct pngio_chunk *chunk;

	ASSERT (ct);

	#ifdef BUILD_ICONLIB
	/*
	 * Remove all the existing icon tags..
	 */
	{
		struct pngio_chunk *nchunk;

		for ( (chunk = FIRSTNODE(&ct->chunklist[0])); (nchunk = NEXTNODE(chunk)) ; (chunk = nchunk) )
		{
			if (chunk->id == pngicon_id)
			{
				REMOVE(chunk);
			}
		}
	}
	#endif

	if (ct->tl && ct->tl->totalsize)
	{
		if ( (chunk = AllocPooled(ct->pool, sizeof(*chunk) + ct->tl->totalsize)) )
		{
			struct pngio_chunk *ichunk;
			struct pngio_tag *t;
			UBYTE *ptr = chunk->data;
			chunk->size = ct->tl->totalsize;
			chunk->id = pngicon_id;
			chunk->crc_status = CRC_CLEARED;

			ITERATELIST(t, &ct->tl->taglist)
			{
				*(ULONG *)ptr = t->tag;
				ptr += sizeof(t->tag);
				CopyMem(t->data, ptr, t->size);
				ptr += t->size;
			}

			ichunk = REMTAIL(&ct->chunklist[0]);
			ADDTAIL(&ct->chunklist, chunk); /* insert before 'IEND' */
			ADDTAIL(&ct->chunklist, ichunk);

			return (TRUE);
		}
		/* XXX */
	}
	#ifdef DEBUG
	else
	{
		D(ICONIO,bug("there's no tag\n"));
	}
	#endif
	return (FALSE);
}


/*
 * Create a new icontag chunk.
 */
static ULONG pngio_create_taglist(APTR ctx)
{
	struct pngio_context *ct = ctx;

	ASSERT(ct);
	ASSERT(!ct->tl);

	if ( (ct->tl = AllocPooled(ct->pool, sizeof(*ct->tl))) )
	{
		ct->tl->totalsize = 0;
		NEWLIST(&ct->tl->taglist);
		return (TRUE);
	}
	return (FALSE);
}


ULONG pngio_sigvalid(CONST_STRPTR s)
{
	return (!memcmp(png_sig, s, 8));
}


/*
 * Scans a PNG file and returns a context with
 * all the chunks. The 'icOn' chunk is converted into
 * a taglist (except for iconlib builds)
 */
APTR pngio_create(CONST_STRPTR name, ULONG anyformat)
{
	APTR fh;
	APTR pool;
	struct pngio_context *ctx;
	struct pngio_id id;
	ULONG error = FALSE;

	THREAD;

	if ( (pool = CreatePool(MEMF_ANY, 16384, 8192)) )
	{
		if ( (ctx = AllocPooled(pool, sizeof(*ctx))) )
		{
			struct pngio_chunk *chunk;
			ULONG crc;
			ctx->pool = pool;
			ctx->tl = NULL;
			ctx->bm = NULL;

			NEWLIST(&ctx->chunklist[0]);
			NEWLIST(&ctx->chunklist[1]);

			if ( (fh = file_open(name, MODE_OLDFILE)) )
			{
				UBYTE sig[8];
				ULONG iend_found = FALSE;

				if (file_read(fh, sig, sizeof(sig)))
				{
					if (pngio_sigvalid((UBYTE *)&sig))
					{
						D(ICONIO,bug("is a png file\n"));

						while (file_read(fh, &id, sizeof(id)))
						{
							if (anyformat == PNGIO_NO_IMAGE && id.type != pngicon_id)
								continue;

							D(ICONIO,bug("reading type %c%c%c%c, length: %ld\n", (int)(id.type >> 24), (int)((id.type & 0xff0000) >> 16), (int)((id.type & 0xff00) >> 8), (int)(id.type & 0xff), id.len));
							if ( (chunk = AllocPooled(ctx->pool, sizeof(*chunk) + id.len + sizeof(ULONG))) ) /* crc at the end */
							{
								chunk->size = id.len;
								chunk->id = id.type;
								if (file_read(fh, chunk->data, (chunk->size + sizeof(ULONG)))) /* including crc field */
								{
									crc = crc32(0, (UBYTE *)&chunk->id, chunk->size + sizeof(chunk->id)); /* run CRC over the data and type field */

									if (crc == *(ULONG *)(chunk->data + chunk->size))
									{
										D(ICONIO,bug("added chunk\n"));
										chunk->crc_status = CRC_OK;

										#ifndef BUILD_ICONLIB /* we remove them in pngio_tags_to_chunk() for the iconlib build */
										if (id.type == pngicon_id)
										{
											if (ctx->tl)
											{
												D(ICONIO,bug("error.. ic0n tag present twice, ignoring..\n"));
												FreePooled(ctx->pool, chunk, sizeof(*chunk) + id.len + sizeof(ULONG));
											}
											else
											{
												struct TagItem *ti;
												UBYTE *data = chunk->data;
												ULONG len = chunk->size;
												ULONG sublen;

												while (len)
												{
													ti = (struct TagItem *)data;
													sublen = pngio_tag_add(ctx, ti->ti_Tag, &ti->ti_Data);

													if (sublen)
													{
														data += sublen;
														len -= sublen;
													}
													else
													{
														errormsg(ERR_NOMEM);
														error = TRUE;
														break;
													}
												}
												if (error)
												{
													break;
												}
												FreePooled(ctx->pool, chunk, sizeof(*chunk) + id.len + sizeof(ULONG));
											}
										}
										else
										#endif
										{
											ADDTAIL(&ctx->chunklist[0], chunk);
										}

										if (id.type == 0x49454e44) /* 'IEND' *must* be the last chunk */
										{
											iend_found = TRUE;
											break;
										}
									}
									else
									{
										D(ICONIO,bug("wrong crc!\n"));
										error = TRUE;
										break;
									}
								}
								else
								{
									D(ICONIO,bug("read error\n"));
									error = TRUE;
									break;
								}
							}
							else
							{
								D(ICONIO,bug("not enough memory\n"));
								error = TRUE;
								break;
							}
						}
					}
				}
				if (!iend_found && !anyformat)
				{
					/* PNGs without 'IEND' must be rejected */
					error = TRUE;
				}
				file_close(fh);
			}
			else if (!anyformat)
			{
				error = TRUE;
			}

			if (!error)
			{
				return ((APTR)ctx);
			}
		}
		DeletePool(pool);
	}
	return (NULL);
}


#ifndef BUILD_ICONLIB
static ULONG pngio_encode_image(struct pngio_context *ct, APTR fh)
{
	UBYTE *buf, *rgba;
	ULONG retval = FALSE, linesize;

	linesize = ct->width * 4 + 1;

	buf = AllocPooled(ct->pool, 16384);
	rgba = AllocPooled(ct->pool, linesize + 3);

	if (buf && rgba)
	{
		struct pngio_id id;
		QUAD pos;

		pos = file_seek(fh, 0, OFFSET_CURRENT);
		if (pos != -1LL)
		{
			id.type = MAKE_ID('I','D','A','T');

			if (file_write(fh, &id, sizeof(id)))
			{
				z_stream stream;

				stream.opaque = Z_NULL;
				stream.zalloc = (alloc_func)&z_alloc;
				stream.zfree = (free_func)&z_free;

				if (deflateInit(&stream, Z_BEST_COMPRESSION) == Z_OK)
				{
					struct RastPort rp;
					ULONG written, y, crc;
					LONG size, flush;

					InitRastPort(&rp);

					rp.BitMap = gfx_bitmap_bm(ct->bm);
					written = 0;
					y = 0;
					rgba += 3;
					*rgba = 0;        /* filter method for scanline */
					crc = 0x35af061e; /* crc32() for IDAT */

					do
					{
						ReadPixelArray(rgba + 1, 0, 0, 0, &rp, 0, y, ct->width, 1, RECTFMT_RGBA);

						y++;

						flush = y < ct->height ? Z_NO_FLUSH : Z_FINISH;

						stream.next_in = rgba;
						stream.avail_in = linesize;

						do
						{
							stream.avail_out = 16384;
							stream.next_out  = buf;

							if (deflate(&stream, flush) == Z_STREAM_ERROR)
							{
								break;
							}

							size = 16384 - stream.avail_out;

							if (size > 0)
							{
								written += size;
								crc = crc32(crc, buf, size);
								file_write(fh, buf, size);
							}
						}
						while (stream.avail_in);
					}
					while (flush != Z_FINISH);

					deflateEnd(&stream);
					file_write(fh, &crc, sizeof(crc));
					file_seek(fh, pos, OFFSET_BEGINNING);

					id.len = written;
					file_write(fh, &id.len, sizeof(id.len));
					file_seek(fh, 0, OFFSET_END);

					D(ICONIO,bug("wrote IDAT chunk, length: %ld\n", written));

					/* XXX: not really true */
					retval = TRUE;
				}
			}
		}
	}

	return retval;
}
#endif


STATIC BOOL pngio_write_chunks(APTR fh, struct pngio_context *ct, struct MinList *list)
{
	const struct pngio_chunk *chunk, *nextchunk;
	BOOL retval = ISLISTEMPTY(list);

	ITERATELISTSAFE(chunk, nextchunk, list)
	{
		ULONG crc;

		retval = FALSE;
		D(ICONIO,bug("writing type %c%c%c%c, length: %ld\n", (int)(chunk->id >> 24), (int)((chunk->id & 0xff0000) >> 16), (int)((chunk->id & 0xff00) >> 8), (int)(chunk->id & 0xff), chunk->size));

		if (file_write(fh, &chunk->size, (chunk->size + sizeof(chunk->size) + sizeof(chunk->id))))
		{
			/* write crc */
			if (chunk->crc_status == CRC_OK)
			{
				/* old one */
				crc = *(ULONG *)(chunk->data + chunk->size);
			}
			else
			{
				/* calculate a new one */
				crc = crc32(0, (UBYTE *)&chunk->id, chunk->size + sizeof(chunk->id));
			}

			if (file_write(fh, &crc, sizeof(crc)))
				retval = TRUE;
		}

		if (!retval)
			break;

		#ifndef BUILD_ICONLIB
		#warning "HACKXXX: limit changing the bm to image index 1 (part of the fix to MT#6635)"
		/* proper fix would need to have index for bm ptr as well */
		if (list == &ct->chunklist[1] &&
		    chunk->id == MAKE_ID('I','H','D','R') && ct->bm)
		{
			pngio_encode_image(ct, fh);
		}
		#endif
	}

	return retval;
}


/*
 * Saves a PNG file by writing all the
 * chunks.
 */
ULONG pngio_save(CONST_STRPTR name, APTR ctx, APTR ofh)
{
	struct pngio_context *ct = ctx;
	ULONG retval = FALSE;

	THREAD;

	D(ICONIO,bug("trying to save context..\n"));
	#ifndef BUILD_ICONLIB
	#if !USE_SHARED_LIBZ
	ASSERT(ZLibBase);
	#endif
	#endif
	if (!ISLISTEMPTY(&ct->chunklist[0]))
	{
		APTR fh;

		D(ICONIO,bug("chunklist is not empty\n"));

		if ( (fh = ofh)  || (fh = file_open(name, MODE_NEWFILE)) )
		{
			D(ICONIO,bug("file <%s> opened for writing\n", name));

			if (file_write(fh, &png_sig, sizeof(png_sig)))
			{
				ULONG i;

				pngio_tags_to_chunk(ctx); /* XXX: check for errors maybe.. */

				for (i = 0; i < 2; i++)
				{
					/* add 2nd PNG header if writing a dualpng image (fix to MT#6635) */
					if (i == 1 && !ISLISTEMPTY(&ct->chunklist[1]))
					{
						retval = file_write(fh, &png_sig, sizeof(png_sig));

						if (!retval)
							break;
					}

					retval = pngio_write_chunks(fh, ct, &ct->chunklist[i]);

					if (!retval)
						break;
				}

				//NEWLIST(&ct->chunklist);
			}
			
			if(!ofh)
			{
				file_close(fh);
			}
		}
		else
		{
			D(ICONIO,bug("file_open(%s, MODE_NEWFILE) failed, error %ld\n", name, IoErr()));
		}
	}
	else
	{
		retval = TRUE;
	}
	return (retval);
}

void pngio_delete(APTR ctx)
{
	ASSERT(ctx);

	DeletePool(((struct pngio_context *)ctx)->pool);
}


/*
 * Returns a pointer to the chunkdata.
 */
#ifdef BUILD_ICONLIB
APTR pngio_get_chunkdata(APTR ctx, ULONG id, ULONG *size)
{
	struct pngio_context *ct = ctx;
	struct pngio_chunk *chunk;
	ASSERT(ct);

	ITERATELIST(chunk, &ct->chunklist[0])
	{
		if (chunk->id == id)
		{
			*size = chunk->size;
			return ((APTR)chunk->data);
		}
	}
	return (NULL);
}
#endif


/*
 * Add a tag. Replaces existing tags, except for tooltypes.
 */
ULONG pngio_tag_add(APTR ctx, ULONG tag, CONST_APTR data)
{
	struct pngio_context *ct = ctx;
	struct pngio_tag *t;
	ULONG size;

	ASSERT(ct);
	ASSERT(tag);

	switch (tag)
	{
		case PNGICON_DefaultTool:
		case PNGICON_ToolType:
			size = strlen((STRPTR)data) + 1; /* '\0' */
			break;

		default:
			size = sizeof(ULONG);
			break;
	}

	if (!ct->tl)
	{
		pngio_create_taglist(ct);
	}

	if (ct->tl)
	{
		if ( (t = AllocPooled(ct->pool, sizeof(*t) + size)) )
		{
			t->tag = tag;
			t->size = size;

			/*
			 * Remove already existing tags,
			 * except for tooltypes.
			 */
			if (tag != PNGICON_ToolType)
			{
				pngio_tag_delete(ct, tag);
			}

			CopyMem((APTR)data, t->data, size);
			ADDTAIL(&ct->tl->taglist, t);
			ct->tl->totalsize += size + sizeof(t->tag);

			return (size + sizeof(t->tag));
		}
	}
	return (0);
}


/*
 * Deletes all the tags matching 'tag'.
 */
void pngio_tag_delete(APTR ctx, ULONG tag)
{
	struct pngio_context *ct = ctx;
	struct pngio_tag *td, *tdn;

	ASSERT(ct);

	if (ct->tl)
	{
		for ( (td = FIRSTNODE(&ct->tl->taglist)) ; (tdn = NEXTNODE(td)) ; (td = tdn) )
		{
			if (td->tag == tag)
			{
				ct->tl->totalsize -= td->size + sizeof(td->tag);
				REMOVE(td);
				FreePooled(ct->pool, td, sizeof(*td) + td->size);
			}
		}
	}
}


STATIC ULONG pngio_set_iend(APTR ctx, struct MinList *list)
{
	struct pngio_context *ct = ctx;
	struct pngio_chunk *ichunk = NULL, *ch, *nextch;

	ITERATELISTSAFE(ch, nextch, list)
	{
		switch (ch->id)
		{
			case MAKE_ID('I','E','N','D'):
				REMOVE(ch);

				if (ichunk)
					FreePooled(ct->pool, ichunk, sizeof(*ichunk) + sizeof(ULONG));

				ichunk = ch;
				break;
		}
	}

	if (ichunk || (ichunk = AllocPooled(ct->pool, sizeof(*ichunk) + sizeof(ULONG))))
	{
		ichunk->crc_status = CRC_OK;
		ichunk->size   = 0;
		ichunk->id     = MAKE_ID('I','E','N','D');
		*(ULONG *)(ichunk + 1) = 0xae426082; /* precalculated crc */
		ADDTAIL(list, ichunk);
	}

	return (ULONG)ichunk;
}

#ifndef BUILD_ICONLIB
ULONG pngio_finalize_iconless(APTR ctx)
{
	struct pngio_context *ct = ctx;
	return pngio_set_iend(ctx, &ct->chunklist[0]);
}

ULONG pngio_has_image(APTR ctx)
{
	struct pngio_context *ct = ctx;
	struct pngio_chunk *ch;
	int count = 0;

	ITERATELIST(ch, &ct->chunklist[0])
	{
		switch (ch->id)
		{
			case MAKE_ID('I','H','D','R'):
			case MAKE_ID('I','D','A','T'):
				count++;
				break;
		}
	}

	return count == 2 ? TRUE : FALSE;
}
#endif

/*
 * Attach a bitmap to a png icon
 */
ULONG pngio_add_bitmap(APTR ctx, APTR bm, ULONG image_number)
{
	struct pngio_context *ct = ctx;
	struct pngio_chunk *chunk, *ch, *nextch;
	struct pngio_ihdr *hdr;
	ULONG rc = FALSE;

	ASSERT(ct);
	ASSERT(image_number < 2);

	/* can't replace image 0 since pngio_write_chunks HACKXXX */
	if (image_number == 0)
	{
		return rc;
	}

	if ((chunk = AllocPooled(ct->pool, sizeof(*chunk) + sizeof(*hdr))))
	{
		struct MinList *list = &ct->chunklist[image_number];

		if ((pngio_set_iend(ctx, list)))
		{
			/* XXX: Strip PNG chunks if present, should probably strip everything...
			 * PLTE, tRNS, whatever... keeping them makes no sense when creating new
			 * imagery. Consult PNG manual some day.
			 */

			ITERATELISTSAFE(ch, nextch, list)
			{
				switch (ch->id)
				{
					case MAKE_ID('I','H','D','R'):
					case MAKE_ID('I','D','A','T'):
					case MAKE_ID('g','A','M','A'): /* strip gAMA just case */
						REMOVE(ch);
						break;
				}
			}

			hdr = (APTR)(chunk + 1);

			ct->bm = bm;
			ct->width = gfx_bitmap_width(bm);
			ct->height = gfx_bitmap_height(bm);

			chunk->crc_status = CRC_CLEARED;
			chunk->size   = sizeof(*hdr);
			chunk->id     = MAKE_ID('I','H','D','R');

			hdr->width  = gfx_bitmap_width(bm);
			hdr->height = gfx_bitmap_height(bm);
			hdr->depth  = 8;      /* 8 bits per sample */
			hdr->colortype = 6;   /* RGBA */
			hdr->compression = 0;
			hdr->filter      = 0;
			hdr->interlace   = 0;

			ADDHEAD(list, chunk);
		}

		rc = TRUE;
	}

	return rc;
}

#endif /* USE_PNGICONS */
