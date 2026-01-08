/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
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
 * $Id: newicon.c,v 1.6 2006/08/08 13:31:35 fab Exp $
 */

#include "ambient.h"

#if USE_NEWICONS

/* public */

/* private */
#include "classes.h"
#include "iconmem.h"
#include "methodstack.h"
#include "newicon.h"
#include "gfx_mask.h"
#include "gfx_alpha.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "iconio.h"

static const TEXT id1[] = "IM1=";
static const TEXT id2[] = "IM2=";
const TEXT newiconstart[41] = "*** DON'T EDIT THE FOLLOWING LINES!! ***";

#ifndef CTABFMT_RGB8
#define CTABFMT_RGB8 2
#endif

/*
 * Detects a newicon icon.
 * tt - tooltype string without front ULONG
 */
ULONG newicon_find(APTR fh, UBYTE *tt)
{
	THREAD;
	/*
	 * Looks for the following 2 tooltypes:
	 * " "
	 * "*** DON'T EDIT THE FOLLOWING LINES!! ***"
	 */
	if (*tt == ' ' && !*(tt + 1))
	{
		STRPTR buf = icon_read_infostring(fh, NULL);

		if (buf)
		{
			ULONG ret = FALSE;

			if (!stricmp(newiconstart, buf))
			{
				ret = TRUE;
			}
			icon_free(buf);
			return (ret);
		}
	}
	return (FALSE);
}


/*
 * Finds the next tooltype with the right ID.
 * p - tooltype string with front ULONG size specifier
 * id - ID to look for (4 bytes)
 */
static UBYTE * next_tooltype(APTR fh, CONST_STRPTR id, ULONG *len)
{
	STRPTR buf;

	while (1)
	{
		buf = icon_read_infostring(fh, len);
		
		if (buf)
		{
			if (!strncmp(buf, id, 4))
			{
				break;
			}
			icon_free(buf);
			buf = NULL;
		}
		else
		{
			break;
		}
	}
	return ((UBYTE *)buf);
}


/*
 * Decodes a newicon line. Taken from the newicon sources. This manages to trash
 * the frontwall from time to time. Why ? No idea.. frontwall byte -1 trashed with val 0x0
 */
static LONG newicon_decodeline(CONST UBYTE *from, UBYTE *to, LONG bitsperbyte, LONG maxlen)
{
	WORD inbits = 0;
	LONG inbound = 0;
	UBYTE *origto = to;
	UBYTE mask = (1 << bitsperbyte) - 1;
	LONG zerocount = 0;

	while (zerocount > 0 || *from)
	{
		LONG newbits;

		inbound <<= 7;
		if (!zerocount)
		{
			UBYTE i;

			i = *(from++) - 0x20;
			if (i >= 0x50)
			{
				i -= 0x31;
				if (i >= 0x80)
				{
					zerocount = i - 0x80;
					i = 0;
				}
				else inbound |= i;
			}
			else inbound |= i;
		}
		else zerocount--;

		inbits += 7;

		while ((newbits = inbits - bitsperbyte) >= 0)
		{
			*(to++) = (inbound >> newbits) & mask;
			if (to - origto == maxlen) return (maxlen);
			inbits = newbits;
		}
	}
	return (to - origto);
}


/*
 * Reads a newicon image.
 */
ULONG newicon_read_image(APTR fh, APTR obj, ULONG mode, ULONG ancillary)
{
	CONST_STRPTR id;
	STRPTR ptr;
	ULONG retval = FALSE;
	ULONG sz;

	THREAD;
	ASSERT(fh);
	CHECKOBJECT(obj);

	D(ICONIO, bug("getting NI image, mode: %ld..\n", mode));

	if (mode == MV_Icon_BitMap_Normal)
	{
		id = id1;
	}
	else
	{
		id = id2;
	}

	/* find image */
	ptr = next_tooltype(fh, id, &sz);

	if (ptr) /* do not set an error if there's no image */
	{
		if (*ptr && (sz - 1) > 8 && (*(ptr + 4) == 'B' || *(ptr + 4) == 'C'))
		{
			ULONG colornum, width, height, len;
			UBYTE *palette;

			if (ancillary)
			{
				methodstack_push_sync(obj, 4,
					MM_Icon_AddAncillary, (mode == MV_Icon_BitMap_Normal) ? MV_Icon_Ancillary_NewiconNormalTT : MV_Icon_Ancillary_NewiconSelectedTT, NULL, ptr
				);
			}

			colornum = (*(ptr + 7) - 0x21) * 64 + (*(ptr + 8) - 0x21);

			width = *(ptr + 5) - 0x21;
			height = *(ptr + 6) - 0x21;

			D(ICONIO, bug("colornum: %ld, width: %ld, height: %ld\n", colornum, width, height));

			len = 3 * colornum * sizeof(UBYTE);

			if (width && height && colornum && (colornum <= 256))
			{
				palette = icon_malloc(len + 7); /* 7: safety margin for newicon_decodeline(), maybe not needed */

				if (palette) 
				{
					ULONG trans;
					UBYTE *chunky;
					ULONG read;
					UBYTE *p = palette;

					/* transparency */
					if (*(ptr + 4) == 'B')
					{
						trans = TRUE; /* color 0 is transparent */
						D(ICONIO, bug("transparent\n"));
					}
					else
					{
						trans = FALSE;
						D(ICONIO, bug("non transparent\n")); /* XXX: take that into account */
					}

					/* read the palette which starts at byte 9  (first line) */
					read = newicon_decodeline(ptr + 9, p, 8, len);
				
					p += read;
					len -= read;

					icon_free(ptr);

					/* continue reading the next lines */
					while (len > 0)
					{
						ptr = next_tooltype(fh, id, NULL);
						if (ptr && *ptr)
						{
							if (ancillary)
							{
								methodstack_push_sync(obj, 4,
									MM_Icon_AddAncillary, (mode == MV_Icon_BitMap_Normal) ? MV_Icon_Ancillary_NewiconNormalTT : MV_Icon_Ancillary_NewiconSelectedTT, NULL, ptr
								);
							}
							read = newicon_decodeline(ptr + 4, p, 8, len);
							p += read;
							len -= read;
							icon_free(ptr);
						}
						else
						{
							methodstack_push(obj, 2,
								MM_Icon_ErrorString, "newicon; palette data error"
							);
							read = 0; /* marker */
							break;
						}
					}

					if (read)
					{
						/*
						 * Reading the chunky data.
						 */
						len    = width * height * sizeof(UBYTE);
						chunky = icon_malloc(len+7);              /* 7: safety margin too, maybe not needed */

						if (chunky) 
						{
							APTR bm;
							APTR mbm;
							ULONG depth = 1;
							ULONG cn = colornum - 1; /* XXX: maybe remove and process colornum directly */
							p = chunky;

							while (cn >>= 1)
							{
								depth++;
							}

							while (len > 0)
							{
								ptr = next_tooltype(fh, id, NULL);
								if (ptr && *ptr)
								{
									if (ancillary)
									{
										methodstack_push_sync(obj, 4,
											MM_Icon_AddAncillary, (mode == MV_Icon_BitMap_Normal) ? MV_Icon_Ancillary_NewiconNormalTT : MV_Icon_Ancillary_NewiconSelectedTT, NULL, ptr
										);
									}
									read = newicon_decodeline(ptr + 4, p, depth, len);
									p += read;
									len -= read;
									icon_free(ptr);
								}
								else
								{
									methodstack_push(obj, 2,
										MM_Icon_ErrorString, "newicon; chunky data error"
									);
									read = 0; /* abusing var again :) */
									break;
								}
							}

							if (read)
							{
								bm = gfx_bitmap_create(width, height, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE);
								
								if (bm)
								{
									gfx_blit(chunky, bm,
										BLITTAG_SrcType, BLITVAL_SrcType_Array,
										BLITTAG_CMAP, palette,
										BLITTAG_CMAPFormat, BLITVAL_CMAPFormat_RGB8,
									TAG_DONE);

									/* mark success */
								}
								else
								{
									methodstack_push(obj, 2,
										MM_Icon_ErrorString, "newicon; out of memory"
									);
									read = 0;
								}

								if (read)
								{
									if (trans)
									{
										mbm = gfx_mask_create_chunky8(chunky, width, height, 0);

										if (mbm)
										{
											gfx_alpha_set_mask(bm, 0, 0, width, height, mbm, 0xff, TRUE);
											gfx_bitmap_delete(mbm);
										}
										else
										{
											methodstack_push(obj, 2,
												MM_Icon_ErrorString, "newicon; out of memory"
											);
											read = FALSE;
										}
									}
									else
									{
										gfx_alpha_set(bm, 0, 0, width, height, 0xff);
									}

									if (read)
									{
										methodstack_push(obj, 4,
											MM_Icon_AddBitMap, bm, MV_Icon_BitMap_Newicon, mode
										);
										retval = TRUE;
									}
								}
							}
							icon_free(palette);
							icon_free(chunky);
						}
						else
						{
							methodstack_push(obj, 2,
								MM_Icon_ErrorString, "newicon; out of memory"
							);
						}
					}
				}
				else
				{
					methodstack_push(obj, 2,
						MM_Icon_ErrorString, "newicon; out of memory"
					);
				}
			}
			else
			{
				methodstack_push(obj, 2,
					MM_Icon_ErrorString, "newicon; bogus image data"
				);
			}
		}
		else
		{
			methodstack_push(obj, 2,
				MM_Icon_ErrorString, "newicon; data expected"
			);
		}
	}
	return (retval);
}

#endif /* USE_NEWICONS */
