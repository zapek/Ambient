/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2007 Ambient Open Source Team
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
 * $Id: crawlclass.c,v 1.10 2017/08/21 06:17:44 cyfm Exp $
 */

#include "ambient.h"

#if USE_CRAWLER

/* public */
#include <graphics/rpattr.h>
#include <graphics/layers.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/layers.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>

/* private */
#include "mui_func.h"
#include "smartreq.h"
#include "crypto.h"
#include "gfx_pen.h"


/*
 * The following is not enabled because:
 * 1) it misses the bottom case (easy)
 * 2) we should make MUIG_TextPrint() print in an
 *    offscreen buffer
 */
#define USE_CRAWLER_ALPHA 0

#define FONT_SPACE 1 /* 1 pixel space betwen lines */

struct Data {
	ULONG scroll;
	LONG scrolloffs; /* numbers of pixels to scroll for the next update */
	LONG font_ys;
	STRPTR text;
	STRPTR preparse;
	STRPTR textpos; /* position ptr within the text */
	ULONG paused;
	ULONG select;
	#if USE_CRAWLER_ALPHA
	struct RastPort *rp_pre;
	struct RastPort *rp_post;
	ULONG oldwidth;
	#endif
	struct MUI_InputHandlerNode ihnode;
	struct MUI_EventHandlerNode ehnode;
	ULONG textcolor;
};

static STRPTR next_line(STRPTR s)
{
	if (s)
	{
		while (*s)
		{
			if (*s == '\n' && *(s + 1))
			{
				return (s + 1);
			}
			s++;
		}
	}
	return (NULL);
}

static STRPTR prev_line(STRPTR s, STRPTR text)
{
	if (s)
	{
		if (*(s - 1) == '\n' && *(s - 2)) s -= 2;

		while (s > text && *s)
		{
			if (*s == '\n')
			{
				return (s + 1);
			}
			s--;
		}
	}
	return (NULL);
}


#if USE_CRAWLER_ALPHA
static struct RastPort * alloc_rp(ULONG xs, ULONG ys)
{
	struct BitMap *bm;
	struct RastPort *rp = NULL;

	if ((bm = AllocBitMap(xs, ys, 32, BMF_MINPLANES, NULL)))
	{
		struct Layer *l;
		struct Layer_Info *li;

		if ((li = NewLayerInfo()))
		{
			if ((l = CreateUpfrontLayer(li, bm, 0, 0, xs - 1, ys - 1, LAYERSIMPLE, NULL)))
			{
				rp = l->rp;
			}
			else
			{
				DisposeLayerInfo(li);
			}
		}
	
		if (!rp)
		{
			FreeBitMap(bm);
		}
	}
	return (rp);
}

static void free_rp(struct RastPort *rp)
{
	struct Layer_Info *li;
	struct BitMap *bm;

	li = rp->Layer->LayerInfo;
	bm = rp->BitMap;

	DeleteLayer(NULL, rp->Layer);
	DisposeLayerInfo(li);
	FreeBitMap(bm);
}
#endif


DEFNEW
{
	struct Data *data;

	obj = DoSuperNew(cl, obj,
		TAG_MORE, INITTAGS
	);

	if (!obj)
	{
		return ((ULONG)NULL);
	}

	data = INST_DATA(cl, obj);

	FORTAG(INITTAGS)
	{
		case MA_Crawl_Content:
			data->text = (STRPTR)tag->ti_Data;
			break;

		case MA_Crawl_PreParse:
			data->preparse = (STRPTR)tag->ti_Data;
			break;
	}
	NEXTTAG

	data->textpos = data->text;

	return ((ULONG)obj);
}


DEFMMETHOD(AskMinMax)
{
	DOSUPER;

	msg->MinMaxInfo->MaxWidth  = MUI_MAXMAX;
	msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;

	return (0);
}


DEFMMETHOD(Setup)
{
	GETDATA;

	if (!DOSUPER)
	{
		return (0);
	}

	data->ihnode.ihn_Object = obj;
	data->ihnode.ihn_Flags = MUIIHNF_TIMER;
	data->ihnode.ihn_Millis = 40;
	data->ihnode.ihn_Method = MM_Crawl_Tick;

	DoMethod(app, MUIM_Application_AddInputHandler, &data->ihnode);

	data->ehnode.ehn_Object = obj;
	data->ehnode.ehn_Class = cl;
	data->ehnode.ehn_Priority = 6; /* just one above virtgroup's wheel */
	data->ehnode.ehn_Events = IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY | IDCMP_DISKREMOVED;
	data->ehnode.ehn_Flags = MUI_EHF_GUIMODE;
	DoMethod(_win(obj), MUIM_Window_AddEventHandler, (ULONG)&data->ehnode);

	data->font_ys = _font(obj)->tf_YSize + FONT_SPACE;

	/*  stuntzi will hate us ;)
	 */
	{
		struct MUI_PenSpec *ps = (APTR)"m5"; /* textpen is default */
		ULONG addr;

		if (DoMethod(obj, MUIM_GetConfigItem, 0xED, &addr))
		{
			ps = (struct MUI_PenSpec *)addr;
		}
	
		data->textcolor = gfx_get_penspec_value(muiRenderInfo(obj), ps);
	}

	return (TRUE);
}


DEFMMETHOD(Cleanup)
{
	GETDATA;

	DoMethod(_win(obj), MUIM_Window_RemEventHandler, (ULONG)&data->ehnode);
	DoMethod(app, MUIM_Application_RemInputHandler, &data->ihnode);

	return (DOSUPER);
}


DEFMMETHOD(HandleEvent)
{
	if (msg->imsg)
	{
		switch (msg->imsg->Class)
		{
			case IDCMP_MOUSEBUTTONS:
			{
				switch (msg->imsg->Code)
				{
					case SELECTUP:
						{
							GETDATA;
							if (_isinobject(msg->imsg->MouseX, msg->imsg->MouseY))
							{
								if (data->select)
								{
									data->paused ^= TRUE;
								}
							}
							data->select = FALSE;
						}
						break;

					case SELECTDOWN:
						if (_isinobject(msg->imsg->MouseX, msg->imsg->MouseY))
						{
							GETDATA;
							data->select = TRUE;
						}
						break;
				}
			}
			break;

			case IDCMP_RAWKEY:
			{
				switch (msg->imsg->Code)
				{
					case NM_WHEEL_UP:
					case NM_WHEEL_DOWN:
						{
							if (_isinobject(msg->imsg->MouseX, msg->imsg->MouseY))
							{
								GETDATA;
								LONG offs;
								
								if (msg->imsg->Qualifier & IEQUALIFIER_ALTS)
								{
									offs = 48;
								}
								else if (msg->imsg->Qualifier & IEQUALIFIER_CONTROLS)
								{
									offs = 24;
								}
								else if (msg->imsg->Qualifier & IEQUALIFIER_SHIFTS)
								{
									offs = 12;
								}
								else
								{
									offs = 6;
								}
								
								if (msg->imsg->Code == NM_WHEEL_UP)
								{
									offs = -offs;
								}
								data->scrolloffs += offs;
								data->scroll = TRUE;
								MUI_Redraw(obj, MADF_DRAWOBJECT);
								return (MUI_EventHandlerRC_Eat);
							}
						}
						break;
				}
			}
			break;

			case IDCMP_DISKREMOVED:
				if (msg->imsg->Qualifier & IEQUALIFIER_ALTS &&
				    msg->imsg->Qualifier & IEQUALIFIER_SHIFTS)
				{
					if (muiRenderInfo(obj) && _win(obj))
					{
						/* "What the dell..." */
						TEXT title[] = "\x0\x57\x69\x63\x77\x24\x71\x6e\x62\x28\x6d\x6f\x67\x60\x23\x20\x21\x10";
						/* "Oups." */
						TEXT gadgets[] = "\x0\x4f\x74\x72\x70\x2a\x5";
						/* "No Easter egg here.\n\nLook somethere else to find one." */
						TEXT format[] = "\x0\x4e\x6e\x22\x46\x65\x76\x72\x62\x7a\x29\x6f\x6c\x6b\x2d\x66\x6a\x62\x74\x3c\x33\x1e\x1f\x5a\x78\x77\x72\x3a\x68\x73\x70\x7b\x6b\x48\x44\x50\x46\x4\x40\x4a\x54\x4d\x9\x5e\x44\xc\x4b\x47\x41\x54\x11\x5d\x5d\x51\x1b\x36";
						smartreq_request(obj, _win(obj),
							crypto_decrypt_txt(title),
							(ULONG)NULL,
							(LONG)NULL,
							crypto_decrypt_txt(gadgets),
							MV_Notification_Help,
							crypto_decrypt_txt(format),
							NULL
						);
					}
				}
			break;
		}
	}
	return (0);
}


DEFTMETHOD(Crawl_Tick)
{
	GETDATA;

	if (!data->paused)
	{
		data->scrolloffs++;
		data->scroll = TRUE;
		MUI_Redraw(obj, MADF_DRAWOBJECT);
	}
	return (0);
}


#define BORDERSIZE 16 /* total width to leave out of the object size for text formatting purposes */

static ULONG adjust_text(struct IClass *cl UNUSED, APTR obj, STRPTR s, STRPTR *e, ULONG len)
{
	struct TextExtent te;
	ULONG done = FALSE;

	while ( !done )
	{

		TextExtent( _rp( obj ), s, len, &te );

		if ( te.te_Width  > max(_mwidth(obj) - BORDERSIZE, BORDERSIZE) )
		{
			if (**e == '\n')
			{
				(*e)--;
			}

			while (len > 2)
			{
				(*e)--;
				len--;

				if (**e == ' ') break;
			}
		}
		else
		{
			done = TRUE;
		}
	}
	return (len + 1);
}

#define FILTERBUFSIZE 1024

static STRPTR filter_allescapecodes(STRPTR src, ULONG len)
{
	static TEXT tmp[ FILTERBUFSIZE ];
	STRPTR dst = tmp;
	int x = 0;

	ASSERT(src);

	while( len > 0 && *src && x++< (FILTERBUFSIZE-1) )
	{
		if( *src == 27 )
		{
			if ( src[ 1 ] != 0 )
			{
				src++;
				len--;
			}
		}
		else
			*dst++ = *src;

		src++;
		len--;
	}
	*dst = 0;

	return( tmp );
}

DEFMMETHOD(Draw)
{
	DOSUPER;

	if (msg->flags & MADF_DRAWOBJECT) /* full refresh as we need double buffering */
	{
		APTR cliphandle;
		ULONG i;
		STRPTR s, t;
		LONG len;
		LONG ypos;
		GETDATA;

		SetRPAttrs(_rp(obj), RPTAG_PenMode, FALSE, RPTAG_FgColor, data->textcolor, TAG_DONE);
		
		if (data->scroll)
		{
			STRPTR prev = data->textpos;
			
			if (data->scrolloffs > data->font_ys)
			{
				/* forwards */
				data->scrolloffs = 0;
				if ((data->textpos = next_line(data->textpos)))
				{
					adjust_text(cl, obj, prev, &data->textpos, data->textpos - prev - 1);
				}
				else
				{
					data->textpos = data->text; /* back to the beginning */
				}
			}
			else if (abs(data->scrolloffs) > data->font_ys)
			{
				/* backwards */
				data->scrolloffs = 0;
				if ((data->textpos = prev_line(data->textpos, data->text)))
				{
					//adjust_text(cl, obj, &data->textpos, data->textpos - prev - 1);
					/* XXX: I think it needs a new adjust_text :) yes I do.. atm the text jumps a bit then */
				}
				else
				{
					data->textpos = data->text; /* back to the beginning */
				}
			}
			data->scroll = FALSE;
		}

		s = data->textpos;
		t = next_line(s);

		for (i = 0; i <= _mheight(obj) + data->font_ys; i += data->font_ys)
		{
			if (s && t)
			{
				len = t - s - 1;

				if (len > 0)
				{
					ypos = _mtop(obj) + i - data->scrolloffs;

					#if USE_CRAWLER_ALPHA
					if (ypos < data->font_ys)
					{
						/* alpha rendering */
						if (!(data->rp_pre && (data->oldwidth == _mwidth(obj))))
						{
							data->rp_pre = alloc_rp(_mwidth(obj), data->font_ys - FONT_SPACE);
							data->oldwidth = _mwidth(obj);
						}

						if (data->rp_pre)
						{
							BltBitMapRastPort(data->rp_pre->BitMap, 0, 0, data->rp_pre, 0, 0, data->oldwidth, data->font_ys - FONT_SPACE, 0x0); /* clear */
							MUIG_TextPrint(muiRenderInfo(obj), /* XXX: wrong.. it should print in rp_pre.. */
								0,
								ypos,
								_mwidth(obj),
								data->font_ys - FONT_SPACE,
								s,
								len,
								data->preparse,
								0
							);
							/* XXX: there should go some alpha processing.. cgx sets the alpha to full for everything here */
							WritePixelArrayAlpha(data->rp_pre->BitMap->Planes[0], 0, 0, GetCyberMapAttr(data->rp_pre->BitMap, CYBRMATTR_XMOD), _rp(obj), 0, 0, data->oldwidth, data->font_ys - FONT_SPACE, 0xffffffff);
						}
					}

					if ((ypos > 0) && (ypos < _mbottom(obj) - data->font_ys))
					#endif
					{
						struct TextExtent te;
						STRPTR filtered_s;
						ULONG filtered_len;

						#if USE_CRAWLER_ALPHA
						cliphandlealpha = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj) + data->font_ys, _mwidth(obj), _mheight(obj) - data->font_ys * 2); /* XXX: not sure about the height */
						#endif
						cliphandle = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj));
						
						/*
						 * Try to format the
						 * text so it fits.
						 */
						len = adjust_text(cl, obj, s, &t, len);
						filtered_s = filter_allescapecodes( s, len );
						filtered_len = strlen( filtered_s ) - 1; /* don't ask why:) it's only temporary after all */

						TextExtent( _rp( obj ), filtered_s, filtered_len, &te );
						Move( _rp( obj ) , _mleft( obj ) + ( _mwidth( obj ) - te.te_Width ) / 2, ypos );
						Text( _rp( obj ), filtered_s, filtered_len );

						MUI_RemoveClipping(muiRenderInfo(obj), cliphandle);
						#if USE_CRAWLER_ALPHA
						MUI_RemoveClipping(muiRenderInfo(obj), cliphandlealpha);
						#endif
					}
				}
			}
			s = t;
			t = next_line(t);
		}

		SetRPAttrs(_rp(obj), RPTAG_PenMode, TRUE, TAG_DONE);	
	}

	return (0);
}


DEFSET
{
	FORTAG(INITTAGS)
	{
		case MA_Crawl_Content:
			{
				GETDATA;
				data->text = (STRPTR)tag->ti_Data;
				data->textpos = data->text;
				data->paused = FALSE;
				MUI_Redraw(obj, MADF_DRAWOBJECT);
			}
			break;
	}
	NEXTTAG

	return (DOSUPER);
}


DEFDISP
{
	#if USE_CRAWLER_ALPHA
	GETDATA;
	
	if (data->rp_pre)
	{
		free_rp(data->rp_pre);
	}

	if (data->rp_post)
	{
		free_rp(data->rp_post);
	}
	#endif

	return (DOSUPER);
}


BEGINMTABLE
DECNEW
DECDISP
DECSET
DECMMETHOD(AskMinMax)
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECMMETHOD(HandleEvent)
DECTMETHOD(Crawl_Tick)
DECMMETHOD(Draw)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Area, crawlclass)

#endif /* USE_CRAWLER */
