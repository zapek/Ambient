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
 * $Id: textbox.c,v 1.11 2018/08/05 16:17:16 bitrocky Exp $
 */

#include "ambient.h"

/* public */
#include <graphics/rpattr.h>
#include <proto/utility.h>
#include <proto/graphics.h>

/* private */
#include "textbox.h"
#include "gfx_alpha.h"
#include "gfx_blur.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "mui_func.h"

struct textbox_context {
	struct atextfont *afont;
	STRPTR text;
	LONG textlen;
	LONG rendermode;
	LONG color1;
	LONG color2;
	LONG addy;
	LONG xs, ys;
	LONG width;
	LONG alphaval;
	LONG style;
	struct MinList l;
};


struct splittext {
	struct MinNode n;
	CONST_STRPTR s;
	WORD offx;
	WORD len;
};


#define OFFSET_SHADOW_X 0
#define OFFSET_SHADOW_Y 0
#define OFFSET_SHADOW_XS 1
#define OFFSET_SHADOW_YS 1

#define OFFSET_OUTLINE_X 1
#define OFFSET_OUTLINE_Y 1
#define OFFSET_OUTLINE_XS 2
#define OFFSET_OUTLINE_YS 2

#define OFFSET_ALPHASHADOW_X 1
#define OFFSET_ALPHASHADOW_Y 1
#define OFFSET_ALPHASHADOW_XS 2
#define OFFSET_ALPHASHADOW_YS 1


static void render_setfont(struct textbox_context *ct, struct RastPort *rp)
{
	SetFont(rp, ct->afont->tf);
	if ( ct->style )
	{
		SetSoftStyle(rp, ct->style, FSF_BOLD | FSF_UNDERLINED | FSF_ITALIC);
	}
}

enum {
	FIND_NONSPACE,
	FIND_NONPUNCT,
	FIND_NONCAPS,
	FIND_NONCONSONANT,
};

static ULONG isconsonant(char c)
{
	switch (c)
	{
		case 'a':
		case 'e':
		case 'i':
		case 'o':
		case 'u':
		case 'y':
			return (FALSE);

		default:
			return (TRUE);
	}
}

static CONST_STRPTR find_nonchar(CONST_STRPTR s, CONST_STRPTR p, ULONG mode)
{
	while (p > s)
	{
		switch (mode)
		{
			case FIND_NONSPACE:
				if (*p != ' ') return (p);

			case FIND_NONPUNCT:
				if (!ispunct(*p)) return (p);
		
			case FIND_NONCAPS:
				if (!isupper(*p)) return (p);
		
			case FIND_NONCONSONANT:
				if (!isconsonant(*p)) return (p);
		}
		p--;
	}
	return (NULL);
}


/*
 * Returns where to split (char included)
 */
static CONST_STRPTR find_split(CONST_STRPTR s, CONST_STRPTR e)
{
	CONST_STRPTR p = e;

	/* find a space */
	while (p > s && *p != ' ') p--;

	if (p != s)
	{
		/* found a space, point to previous non space */
		D(LABELSPLIT,bug("found a space\n"));
		return (find_nonchar(s, p, FIND_NONSPACE));
	}

	p = e;

	/* find a non-punct char */
	while (p > s && !ispunct(*p)) p--;

	if (p != s)
	{
		/* found a non-punct char, point to itself if previous char is not punct */
		CONST_STRPTR q;

		D(LABELSPLIT,bug("found a non-punct char\n"));

		q = find_nonchar(s, p, FIND_NONPUNCT);

		if (q == p - 1)
		{
			/* fine, we can split just before us */
			return (p);
		}
		else
		{
			return (q);
		}
	}

	p = e;

	/* find a capital then */
	while (p > s && !isupper(*p)) p--;

	if (p != s)
	{
		D(LABELSPLIT,bug("found a capital\n"));
		return (find_nonchar(s, p, FIND_NONCAPS));
	}
	
	p = e;

	/* fuck. find a consonant then */
	while (p > s && !isconsonant(*p)) p--;

	if (p != s)
	{
		D(LABELSPLIT,bug("found a consonant\n"));
		return (find_nonchar(s, p, FIND_NONCONSONANT));
	}
	return (NULL); /* damn user from hell */
}

/* function which returns shared dbuf bitmap to which label can render. (XXX:Add semaphore protection? shouldn't be needed) */

static LONG textbox_count = 0;
static APTR textbox_bitmap = NULL;

static APTR textbox_alloc_bitmap( LONG width, LONG height )
{
	LONG old_width = 0, old_height = 0;

	if ( textbox_bitmap && gfx_bitmap_width(textbox_bitmap) >= width && gfx_bitmap_height(textbox_bitmap) >= height )
	{
		return textbox_bitmap;
	}
	else
	{
		if ( textbox_bitmap )
		{
			old_width = gfx_bitmap_width( textbox_bitmap );
			old_height = gfx_bitmap_height( textbox_bitmap );

			gfx_bitmap_delete( textbox_bitmap );
		}
		textbox_bitmap = gfx_bitmap_create( max( width, old_width ), max( height, old_height ), 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE);
	}

	return textbox_bitmap;
}

static void textbox_free_bitmap( void )
{
	if ( textbox_bitmap && textbox_count < 10 )
	{
		/* no need to store it */

		gfx_bitmap_delete( textbox_bitmap );
		textbox_bitmap = NULL;
	}
}


APTR v_textbox_create(struct atextfont *afont, struct TagItem *tags)
{
	struct textbox_context *ct;

	ASSERT(afont);

	if ( (ct = malloc(sizeof(*ct))) )
	{
		ULONG maxwidth = 0;
		ULONG addx, offx = 0;
		ULONG addy, offy = 0;
		struct splittext *stn;
		struct RastPort rp;
		struct TextExtent te;
		
		memset(ct, 0, sizeof(*ct));
		
		ct->afont = afont;
		ct->alphaval = 0xffffffff;
		ct->style = afont->style;

		FORTAG(tags)
		{
			case TEXTBOXTAG_RenderMode:
				ct->rendermode = tag->ti_Data;
				break;

			case TEXTBOXTAG_Text:
				ct->text = (STRPTR)tag->ti_Data;
				ct->textlen = strlen(ct->text);
				break;

			case TEXTBOXTAG_Color1:
				ct->color1 = tag->ti_Data;
				break;

			case TEXTBOXTAG_Color2:
				ct->color2 = tag->ti_Data;
				break;

			case TEXTBOXTAG_Width:
				maxwidth = tag->ti_Data;
				break;

			case TEXTBOXTAG_Alpha:
				ct->alphaval = tag->ti_Data;
				break;

			case TEXTBOXTAG_Italic:
				if ( tag->ti_Data )
					ct->style |= FSF_ITALIC;
				break;

			#ifdef DEBUG
			default:
				PDB(("unknown tag\n"));
			#endif
		}
		NEXTTAG

		switch (ct->rendermode)
		{
			case TBRENDER_Shadow:
				offx = OFFSET_SHADOW_X;
				offy = OFFSET_SHADOW_Y;
				break;

			case TBRENDER_Outline:
				offx = OFFSET_OUTLINE_X;
				offy = OFFSET_OUTLINE_Y;
				break;

			case TBRENDER_AlphaShadow:
				offx = OFFSET_ALPHASHADOW_X;
				offy = OFFSET_ALPHASHADOW_Y;
				break;
		}

		/*
		 * Find out the width and if we have to
		 * split up text.
		 */
		InitRastPort(&rp);
		render_setfont(ct, &rp);

		addx = offx;
		switch (ct->rendermode)
		{
			case TBRENDER_Shadow:
				addx += OFFSET_SHADOW_XS;
				break;

			case TBRENDER_Outline:
				addx += OFFSET_OUTLINE_XS;
				break;

			case TBRENDER_AlphaShadow:
				addx += OFFSET_ALPHASHADOW_XS;
				break;
		}

		addy = offy;
		switch (ct->rendermode)
		{
			case TBRENDER_Shadow:
				addy += OFFSET_SHADOW_YS;
				break;

			case TBRENDER_Outline:
				addy += OFFSET_OUTLINE_YS;
				break;

			case TBRENDER_AlphaShadow:
				addy += OFFSET_ALPHASHADOW_YS;
				break;
		}

		TextExtent(&rp, ct->text, ct->textlen, &te );
		ct->xs = te.te_Extent.MaxX - te.te_Extent.MinX + 1 + addx + 2; // bitRocky: a try to fix overlapping icontexts
		ct->ys = ct->afont->tf->tf_YSize + addy;
		ct->addy = ct->ys + 1; /* XXX: sure about that +1 ? looks nicer at least */

		NEWLIST(&ct->l);

		if (maxwidth && ct->xs > maxwidth)
		{
			ULONG cnt;
			CONST_STRPTR s; /* start of search line */
			ULONG len;
			CONST_STRPTR p, q;

			p = s = ct->text;
			len = ct->textlen;

			ct->xs = 0;
			ct->ys = 0;
			
			D(LABELSPLIT,bug("-------- splitting --------\n"));
			
			while (len)
			{
				/*
				 * Allright, splitting text then.
				 */
				
				D(LABELSPLIT,bug("remaining len: %ld\n", len));

				/* strip trailing spaces */
				while (len && *s == ' ')
				{
					s++;
					len--;
				}

				cnt = TextFit(&rp, s, len, &te, NULL, 1, maxwidth - addx, ct->afont->tf->tf_YSize);
				
				D(LABELSPLIT,bug("would display <%.*s>. cnt: %ld, len: %ld\n", (int)cnt, s, cnt, len));
				
				if (cnt < len)
				{
					p = find_split(s, s + cnt - 1);

					if (!p)
					{
						/*
						 * Damn user from hell. He has a name which is too
						 * long to fit. We split it anyway then.
						 */
						p = s + cnt - 1;
					}
				}
				else
				{
					/* one line fits all */
					p = s + cnt - 1; /* this is a special case because we have to print the whole line so +1 */
				}

				q = p;

				/* eat up trailing spaces */
				while (q > s && *q == ' ')
				{
					q--;
				}

				if ( (stn = malloc(sizeof(*stn))) )
				{
					stn->s = s;
					stn->len = q - s + 1;
					ADDTAIL(&ct->l, stn);
					D(LABELSPLIT,bug("added text <%.*s>, len: %ld\n", (int)stn->len, stn->s, stn->len));
				}
				else
				{
					PDB(("out of mem, argh\n"));
					break; /* XXX: cough */
				}

		
				TextExtent(&rp, stn->s, stn->len, &te );
				stn->offx = te.te_Extent.MaxX - te.te_Extent.MinX + 1 + addx;
				ct->xs = max(ct->xs, stn->offx);
				ct->ys += ct->addy;
				
				len -= p - s + 1;

				s += p - s + 1;
			}

			D(LABELSPLIT,bug("remaining final len: %ld\n", len));

			/*
			 * Ok, now find the centering of the
			 * text, we stored the size in stn->offx
			 * beforehand.
			 */
			ITERATELIST(stn, &ct->l)
			{
				stn->offx = (ct->xs - stn->offx) / 2;
			}
		}
		else
		{
			if ( (stn = malloc(sizeof(*stn))) )
			{
				stn->s = ct->text;
				stn->offx = offx;
				stn->len = ct->textlen;
				if ( ct->rendermode == TBRENDER_AlphaShadow ) ct->xs += 1;

				ADDTAIL(&ct->l, stn);
			}
			/* XXX */
		}
	}

	/* Special adjustment for alphashadow mode, so shadow will be visible below */

	if ( ct->rendermode == TBRENDER_AlphaShadow )
		ct->ys += 2;

	textbox_count++;
	return (ct);
}


void v_textbox_setattrs(APTR ctx, struct TagItem *tags)
{
	struct textbox_context *ct = ctx;

	ASSERT(ct);

	FORTAG(tags)
	{
		case TEXTBOXTAG_Alpha:
			ct->alphaval = tag->ti_Data;
			break;

		#ifdef DEBUG
		default:
			PDB(("unknown tag: 0x%lx\n", tag->ti_Tag));
			break;
		#endif
	}
	NEXTTAG
}


void textbox_delete(APTR ctx)
{
	struct textbox_context *ct = ctx;
	struct splittext *stn, *nextstn;
	
	ASSERT(ct);

	ITERATELISTSAFE(stn, nextstn, &ct->l)
	{
		free(stn);
	}

	textbox_count--;
	textbox_free_bitmap();

	free(ctx);
}

static void render_normal(struct textbox_context *ct, struct RastPort *rp, ULONG x, ULONG y, CONST_STRPTR s, ULONG len, ULONG ismask)
{
	render_setfont(ct, rp);

	y += ct->afont->tf->tf_Baseline;

	if (ismask)
	{
		SetRPAttrs(rp,
			RPTAG_PenMode, FALSE, /* XXX: does that work with a 1-bit plane? must ask cyfm.. */
			RPTAG_FgColor, 0x00ffffff,
			RPTAG_DrMd, JAM1,
		TAG_DONE);
	}
	else
	{
		SetRPAttrs(rp,
			RPTAG_PenMode, FALSE,
			RPTAG_FgColor, ct->color1,
			RPTAG_DrMd, JAM1,
		TAG_DONE);
	}

	Move(rp, x, y);
	Text(rp, s, len);

	SetRPAttrs(rp,
		RPTAG_PenMode, TRUE,
	TAG_DONE);
}

#if USE_SILLY_ICONPREFS

static void render_background(struct textbox_context *ct, struct RastPort *rp, ULONG x, ULONG y, CONST_STRPTR s, ULONG len)
{
	render_setfont(ct, rp);

	y += ct->afont->tf->tf_Baseline;

	SetRPAttrs(rp,
		RPTAG_PenMode, FALSE,
		RPTAG_FgColor, ct->color1,
		RPTAG_BgColor, ct->color2,
		RPTAG_DrMd, JAM2,
	TAG_DONE);

	Move(rp, x, y);
	Text(rp, s, len);

	SetRPAttrs(rp,
		RPTAG_PenMode, TRUE,
	TAG_DONE);
}

static const ULONG colarray[] = {
	0x00ff0000,
	0x0000ff00,
	0x000000ff
};

static void render_multicolor(struct textbox_context *ct, struct RastPort *rp, ULONG x, ULONG y, CONST_STRPTR s, ULONG len)
{
	ULONG xp = x;
	ULONG add;
	ULONG color;

	render_setfont(ct, rp);

	y += ct->afont->tf->tf_Baseline;

	SetRPAttrs(rp,
		RPTAG_PenMode, FALSE,
		RPTAG_DrMd, JAM1,
	TAG_DONE);

	while (len--)
	{
		add = TextLength(rp, s, 1);

		color = len % 3;

		SetRPAttrs(rp,
			RPTAG_FgColor, colarray[color],
		TAG_DONE);

		Move(rp, xp, y);
		Text(rp, s, 1);

		xp += add;
		s++;
	}
}


static void render_gradient(struct textbox_context *ct, struct RastPort *rp, ULONG x, ULONG y, CONST_STRPTR s, ULONG len)
{
	ULONG xp = x;
	ULONG add;
	ULONG color;
	ULONG r, g, b;
	ULONG rt, gt, bt;
	LONG radd, gadd, badd;

	render_setfont(ct, rp);

	y += ct->afont->tf->tf_Baseline;

	SetRPAttrs(rp,
		RPTAG_PenMode, FALSE,
		RPTAG_DrMd, JAM1,
	TAG_DONE);

	r = (ct->color1 >> 16) & 0xff;
	g = (ct->color1 >> 8) & 0xff;
	b = ct->color1 & 0xff;

	rt = (ct->color2 >> 16) & 0xff;
	gt = (ct->color2 >> 8) & 0xff;
	bt = ct->color2 & 0xff;


	radd = abs(r - rt) / len;
	gadd = abs(g - rt) / len;
	badd = abs(b - rt) / len;

	if (rt < r)
	{
		radd = -radd;
	}

	if (gt < g)
	{
		gadd = -gadd;
	}

	if (bt < b)
	{
		badd = -badd;
	}

	while (len--)
	{
		add = TextLength(rp, s, 1);

		color = (r << 16) | (g << 8) | b; /* not optimal but bah */

		SetRPAttrs(rp,
			RPTAG_FgColor, color,
		TAG_DONE);

		Move(rp, xp, y);
		Text(rp, s, 1);

		xp += add;
		r += radd;
		g += gadd;
		b += badd;
		s++;
	}
}

#endif

static void render_outline(struct textbox_context *ct, struct RastPort *rp, ULONG x, ULONG y, CONST_STRPTR s, ULONG len, ULONG ismask)
{
	render_setfont(ct, rp);
	
	y += ct->afont->tf->tf_Baseline;
	
	if (ismask)
	{
		SetRPAttrs(rp,
			RPTAG_PenMode, FALSE, /* XXX: same */
			RPTAG_FgColor, 0xffffffff,
			RPTAG_DrMd, JAM1,
		TAG_DONE);
	}
	else
	{
		SetRPAttrs(rp,
			RPTAG_PenMode, FALSE,
			RPTAG_FgColor, ct->color2,
			RPTAG_DrMd, JAM1,
		TAG_DONE);
	}

	/* NOTE: Use displacement only on one axis instead of diagonal (produced buggy result in some cases) */

	Move(rp, x - 1, y);
	Text(rp, s, len);
	Move(rp, x + 1, y);
	Text(rp, s, len);
	Move(rp, x, y - 1);
	Text(rp, s, len);
	Move(rp, x, y + 1);
	Text(rp, s, len);

	SetRPAttrs(rp,
		RPTAG_FgColor, ct->color1,
	TAG_DONE);

	Move(rp, x, y);
	Text(rp, s, len);

	SetRPAttrs(rp,
		RPTAG_PenMode, TRUE,
	TAG_DONE);
}


static void render_shadow(struct textbox_context *ct, struct RastPort *rp, ULONG x, ULONG y, CONST_STRPTR s, ULONG len)
{
	render_setfont(ct, rp);
	
	y += ct->afont->tf->tf_Baseline;

	SetRPAttrs(rp,
		RPTAG_PenMode, FALSE,
		RPTAG_FgColor, ct->color2,
		RPTAG_DrMd, JAM1,
	TAG_DONE);

	Move(rp, x + 1, y + 1);
	Text(rp, s, len);

	SetRPAttrs(rp,
		RPTAG_FgColor, ct->color1,
	TAG_DONE);

	Move(rp, x, y);
	Text(rp, s, len);

	SetRPAttrs(rp,
		RPTAG_PenMode, TRUE,
	TAG_DONE);
}


#define ITERATESTN_BEGIN ITERATELIST(stn, &ct->l) {
#define ITERATESTN_END y += ct->addy; }

void textbox_render(APTR ctx, struct RastPort *rp, ULONG x, ULONG y)
{
	struct textbox_context *ct = ctx;
	struct splittext *stn;

	struct RastPort trp;
	APTR bm = NULL;
	int alphaval;

	ASSERT(ct);

	//DB(("Rendering label:<%s>,%d,%d\n", ct->text, x,y));

	alphaval = ct->alphaval;

	/* when alpha is != 1.0 then we will need temp bitmap */

	if ( alphaval != 0xffffffff || ct->rendermode == TBRENDER_AlphaShadow )
	{
		InitRastPort(&trp);

		if ( (bm = textbox_alloc_bitmap( ct->xs + 1, ct->addy + 2 ) ) )
		{
			trp.BitMap = gfx_bitmap_bm(bm);

			SetRPAttrs(&trp,
				RPTAG_PenMode, FALSE,
				RPTAG_FgColor, 0x00000000,
				TAG_DONE
			);
		}
		else
		{
			/* try to draw something at least. */

			alphaval = 0xffffffff;
		}
	}

	/* XXX: the loop has to be inside each case! */

	#define BLITBM \
		gfx_blit(bm, rp,  \
			BLITTAG_DstType, BLITVAL_DstType_RastPort, \
			BLITTAG_DstX, x + stn->offx, \
			BLITTAG_DstY, y, \
			BLITTAG_DstWidth, ct->xs - stn->offx * 2 + 1, \
			BLITTAG_DstHeight, ct->addy + 2, \
			BLITTAG_Alpha, alphaval, \
		TAG_DONE);

	#define GETBG \
		gfx_blit(rp, bm,  \
			BLITTAG_DstType, BLITVAL_DstType_Context, \
			BLITTAG_SrcType, BLITVAL_SrcType_RastPort, \
			BLITTAG_SrcX, x + stn->offx, \
			BLITTAG_SrcY, y, \
			BLITTAG_DstX, 0, \
			BLITTAG_DstY, 0, \
			BLITTAG_DstWidth, ct->xs + 1, \
			BLITTAG_DstHeight, ct->addy + 2, \
		TAG_DONE);

	switch (ct->rendermode)
	{
		case TBRENDER_Normal:
			ITERATESTN_BEGIN
			{
				if ( alphaval == 0xffffffff )
				{
					render_normal(ct, rp, 1 + x + stn->offx, y, stn->s, stn->len, FALSE);
				}
				else
				{
					SetRPAttrs(&trp,
						RPTAG_PenMode, FALSE,
						RPTAG_FgColor, 0x00000000,
						TAG_DONE);

					RectFill(&trp, 0, 0, ct->xs, ct->addy - 1 + 2);
					render_normal(ct, &trp, 1 + 0, 0, stn->s, stn->len, TRUE);
					gfx_alpha_transfer(bm, ct->xs, ct->addy + 2, 1, ct->color1 );
					BLITBM
				}
			}
			ITERATESTN_END
			break;

		case TBRENDER_Outline:
			ITERATESTN_BEGIN
			{
				if ( alphaval == 0xffffffff )
				{
					render_outline(ct, rp, 1 + x + stn->offx + 1, y + 1, stn->s, stn->len, FALSE);
				}
				else
				{
					GETBG
					render_outline(ct, &trp, 1 + 1, 1, stn->s, stn->len, FALSE);
					gfx_alpha_set( bm, 0, 0, ct->xs + 2, ct->addy + 2 - 1, 0xff );
					BLITBM
				}
			}
			ITERATESTN_END
			break;
			
		case TBRENDER_AlphaShadow:
			{
				if ( bm )
				{
					ITERATESTN_BEGIN
					{
						ULONG width = ct->xs - stn->offx * 2 + 1;

						/*
						 * For RGB bitmaps the 0x0 minterm can't be
						 * used because it would expand col0 then.
						 * We have to RectFill().
						 */
						SetRPAttrs(&trp,
							RPTAG_PenMode, FALSE,
							RPTAG_FgColor, 0x00000000,
							TAG_DONE
						);
						RectFill(&trp, 0, 0, width, ct->addy - 1 + 2);

						render_normal(ct, &trp, 1 + 1, 2, stn->s, stn->len, TRUE);
						gfx_blur_alpha_transfer(bm, width, ct->addy + 2, 1, ct->color2);

						BLITBM

						if ( alphaval == 0xffffffff )
						{
							/*
							 * Common case.
							 */
							render_normal(ct, rp, 1 + x + stn->offx, y, stn->s, stn->len, FALSE);
						}
						else
						{
							/*
							 * Slow case used only when doing iconfade at begining.
							 */

							SetRPAttrs(&trp,
								RPTAG_PenMode, FALSE,
								RPTAG_FgColor, 0x0,
								TAG_DONE
							);

							RectFill(&trp, 0, 0, width, ct->addy - 1 + 2);
							render_normal(ct, &trp, 1 + 0, 0, stn->s, stn->len, FALSE);
							gfx_alpha_transfer(bm, width, ct->addy + 2, 1, ct->color1 );
							BLITBM
						}

					}
					ITERATESTN_END

				}
			}
			break;
		
		#if USE_SILLY_ICONPREFS

		case TBRENDER_MultiColor:
			ITERATESTN_BEGIN
			{
				if ( alphaval == 0xffffffff )
				{
					render_multicolor(ct, rp, 1 + x + stn->offx, y, stn->s, stn->len);
				}
				else
				{
					GETBG
					render_multicolor(ct, &trp, 1 + 0, 0, stn->s, stn->len);
					gfx_alpha_set( bm, 0, 0, ct->xs, ct->addy, 0xff );
					BLITBM
				}
			}
			ITERATESTN_END
			break;

		case TBRENDER_Gradient:
			ITERATESTN_BEGIN
			{
				render_gradient(ct, rp, 1 + x + stn->offx, y, stn->s, stn->len);
			}
			ITERATESTN_END
			break;

		case TBRENDER_Background:
			ITERATESTN_BEGIN
			{
				if ( alphaval == 0xffffffff )
				{
					render_background(ct, rp, 1 + x + stn->offx, y, stn->s, stn->len);
				}
				else
				{
					GETBG
					render_background(ct, &trp, 1 + 0, 0, stn->s, stn->len);
					gfx_alpha_set( bm, 0, 0, ct->xs + 1, ct->addy + 2 - 1, 0xff );
					BLITBM
				}
			}
			ITERATESTN_END
			break;

		#endif

		default:	/* Shadow */
			ITERATESTN_BEGIN
			{
				if ( alphaval == 0xffffffff )
				{
					render_shadow(ct, rp, 1 + x + stn->offx, y, stn->s, stn->len);
				}
				else
				{
					GETBG
					render_shadow(ct, &trp, 1 + 0, 0, stn->s, stn->len);
					gfx_alpha_set( bm, 0, 0, ct->xs, ct->addy + 2 - 1, 0xff );
					BLITBM
				}
			}
			ITERATESTN_END
			break;
	}

	textbox_free_bitmap();

	#undef BLITBM
	#undef GETBG
}


ULONG textbox_getwidth(APTR ctx)
{
	struct textbox_context *ct = ctx;

	ASSERT(ct);

	return (ct->xs);
}


ULONG textbox_getheight(APTR ctx)
{
	struct textbox_context *ct = ctx;

	ASSERT(ct);
	
	return (ct->ys);
}
