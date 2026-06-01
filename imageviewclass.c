/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2018 Ambient Open Source Team
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
 * $Id: imageviewclass.c,v 1.30 2025/07/24 01:26:38 geit Exp $
 */

#include "ambient.h"

#if USE_VIEW_IMAGE

//#define BENCHMARK
#define BENCHTIMES 50

/* public */
#include <graphics/rpattr.h>
#include <devices/rawkeycodes.h>
#include <intuition/extensions.h>
#include <proto/cybergraphics.h>
#include <proto/multimedia.h>
#include <proto/dos.h> /* XXX: FilePart() */
#include <proto/graphics.h> /* XXX: for EraseRect().. */
#include <proto/intuition.h>
#include <proto/keymap.h> /* XXX: make it cleaner */

/* private */
#include "ambient_cat.h"
#include "imageview.h"
#include "mui_func.h"
#include "print.h"
#include "threads.h"
#include "datatypes_picture.h"
#include "reggae_picture.h"
#include "common_picture.h"
#include "gfx_scale.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "gfx_rotate.h"
#include "recurse.h"
#include "methodstack.h"
#include "keymap.h"
#include "dragdrop.h"
#include "name.h"
#include "command.h"
#include "rexx.h"
#include "storage.h"
#include "readargs.h"
#if USE_DBUF
#include "gfx_dbuf.h"
#endif
#ifdef BENCHMARK
#include "time_func.h"
#endif

struct picnode {
	struct MinNode n;
	TEXT name[0];
};

struct windowpos {
	LONG *left, *top;
	LONG *width, *height;
};

enum {
	IVM_SCALED,
	IVM_NORMAL
};

enum {
	PICLIST_NONE,
	PICLIST_SCANNING,
	PICLIST_SCANNED
};

enum {
	PICSTATE_IDLE, /* XXX: that one sucks a bit */
	PICSTATE_LOADING,
	PICSTATE_SCALING,
	PICSTATE_SCALED, /* XXX: that one too */
};

enum {
	MODE_NORMAL,
	MODE_FULLSCREEN,
};

struct Data {
	APTR dtp;
	APTR scalebm;
	ULONG mode;
	ULONG state;
	ULONG screen_width;
	ULONG screen_height;
	ULONG wanted_xs;
	ULONG wanted_ys;
	ULONG picliststate;
	ULONG picsnum;
	ULONG picpos;
	struct MUI_EventHandlerNode ehnode;
	struct MinList piclist;
	struct picnode *pn;
	ULONG has_scrollnotify;
	ULONG virt_x; /* negative offset */
	ULONG virt_y; /* negative offset */
	ULONG firstshow;
	ULONG pendingscale;
	ULONG viewmode;
	#if USE_DBUF
	ULONG use_dbuf;
	struct doublebuf *dbuf;
	#endif
	#ifdef BENCHMARK
	ULONG bench;
	ULONG timer;
	#endif

	LONG mouse_x;
	LONG mouse_y;

	STRPTR pendingpath;

	ULONG printing;
};

/*
 * XXX: add a way to switch autoscale/non-autoscale (the dbuf should be allocated and freed from there)
 */

#if USE_DBUF
static void fill_dbuf(struct doublebuf *dbuf)
{
	ASSERT(dbuf);
	FillPixelArray(dbuf->rp, 0, 0, gfx_bitmap_width(dbuf->bm), gfx_bitmap_height(dbuf->bm), 0);
}


static void blit_dbuf(APTR bm, struct doublebuf *dbuf)
{
	ASSERT(bm);
	ASSERT(dbuf);

	gfx_blit(bm, dbuf->rp,
		BLITTAG_DstType, BLITVAL_DstType_RastPort,
		BLITTAG_DstX, gfx_bitmap_width(dbuf->bm) / 2 - gfx_bitmap_width(bm) / 2,
		BLITTAG_DstY, gfx_bitmap_height(dbuf->bm) / 2 - gfx_bitmap_height(bm) / 2,
	TAG_DONE);
}
#endif


DEFNEW
{
	struct Data *data;

	/* XXX: this can be optimized/changed later I think.. */
	obj = DoSuperNew(cl, obj,
		MUIA_CustomBackfill, TRUE,
		MUIA_ShortHelp, GSI( MSG_IMAGEVIEW_CLASS_HOTKEYS_HELP ),
		TAG_MORE, INITTAGS
	);

	if (!obj)
	{
		return ((ULONG)NULL);
	}

	data = INST_DATA(cl, obj);
	NEWLIST(&data->piclist);
	data->viewmode = IVM_SCALED;
	data->pendingpath = NULL;
	#if USE_DBUF
	data->use_dbuf = TRUE;
	#endif

	return ((ULONG)obj);
}


DEFMMETHOD(Setup)
{
	ULONG rc;
	GETDATA;

	if ( (rc = DOSUPER) )
	{
		if (muiRenderInfo(obj) && _win(obj))
		{
			struct DrawInfo *di;

			if ( (di = GetScreenDrawInfo(_screen(obj))) )
			{
				/* XXX: sigh.. that's not fullproof.. user can configure the border where he wants.. ask stuntzi for an extention or so */
				data->screen_width = _screen(obj)->Width - GetSkinInfoAttr(di, SI_BorderLeft, TAG_DONE) - GetSkinInfoAttr(di, SI_BorderRightSize, TAG_DONE);
				data->screen_height = _screen(obj)->Height - GetSkinInfoAttr(di, SI_BorderTopTitle, TAG_DONE) - GetSkinInfoAttr(di, SI_BorderBottomSize, TAG_DONE);
			}

			data->ehnode.ehn_Object = obj;
			data->ehnode.ehn_Class = cl;
			data->ehnode.ehn_Events = IDCMP_RAWKEY | IDCMP_MOUSEBUTTONS;
			data->ehnode.ehn_Flags = MUI_EHF_GUIMODE;
			DoMethod(_win(obj), MUIM_Window_AddEventHandler, (ULONG)&data->ehnode);
		}
		/* XXX */

		#if USE_DBUF
		if (data->use_dbuf)
		{
			if ((data->dbuf = gfx_dbuf_alloc(data->screen_width, data->screen_height, DBUF_DISPLAYABLE | DBUF_CLIPPED, _screen(obj)->RastPort.BitMap)))
			{
				fill_dbuf(data->dbuf);
			}
		}
		else
		{
			/* fallback */
			data->use_dbuf = FALSE;
			PDB(("not enough memory for double buffering in imageview\n"));
		}
		#endif

		/* initial window position */

        {
			STRPTR buff = NULL;

			storage_get(STORAGE_IMAGEVIEW_SNAPSHOT, STORAGE_STRING, (APTR*)&buff);
			if (buff != NULL)
			{
				struct windowpos wp;
				struct RDArgs *rda;

				memset(&wp, 0, sizeof(wp));
				rda	= readargsstring(buff, "LEFT/N,TOP/N,WIDTH/N,HEIGHT/N", (ULONG*)&wp);

				if (rda != NULL)
				{
					if (wp.left != 0 && wp.top != NULL)
					{
						set(_win(obj), MUIA_Window_LeftEdge, *wp.left);
						set(_win(obj), MUIA_Window_TopEdge, *wp.top);
					}

					freeargsstring(rda);
				}
			}
		}
	}
	return (rc);
}


/*
 * That method *must* be called between MUIM_Show/MUIM_Hide.
 */
DEFTMETHOD(Imageview_Scale)
{
	GETDATA;

	if ( ( muiRenderInfo(obj) ) && data->viewmode == IVM_SCALED)
	{
		if (data->dtp)
		{
			APTR bm = picture_getattr(data->dtp, PICTURE_BITMAP);
			ULONG xs, ys;

			xs = _mwidth(obj);
			ys = _mheight(obj);

			gfx_scale_calc_aspect_constraints(gfx_bitmap_width(bm), gfx_bitmap_height(bm), &xs, &ys);

			if ((xs != gfx_bitmap_width(bm)) || (ys != gfx_bitmap_height(bm)))
			{
				if (!data->scalebm || (xs != gfx_bitmap_width(data->scalebm)) || (ys != gfx_bitmap_height(data->scalebm)))
				{
					if (data->state == PICSTATE_SCALING)
					{
						/*
						 * Already scaling. Schedule a new scale then.
						 */
						data->pendingscale = TRUE;
					}
					else
					{
						/* needs rescale with proper aspect ratio */
						data->state = PICSTATE_SCALING;

						DoSuperMethod(cl, obj, MM_View_SetStatus, MF_View_SetStatus_Window | MF_View_SetStatus_Bar | MF_View_SetStatus_Busy, "Scaling image..");

						do_action(obj, TA_Imageview_Scale,
							/*TT_Priority, 1,*/
							TT_Imageview_Scale_Dtp, data->dtp,
							TT_Imageview_Scale_XS, xs,
							TT_Imageview_Scale_YS, ys,
						TAG_DONE); /* XXX */
					}
					/* XXX: we could draw something temporary perhaps or the background takes care of it.. */
				}
			}
			else
			{
				data->state = PICSTATE_SCALED;

				#if USE_DBUF
				if (data->dbuf)
				{
					if (data->dtp)
					{
						fill_dbuf(data->dbuf);
						blit_dbuf(picture_getattr(data->dtp, PICTURE_BITMAP), data->dbuf);
					}
				}
				#endif
			}
		}
	}
	return (0);
}


DEFMMETHOD(Show)
{
	ULONG rc;

	if ( (rc = DOSUPER) )
	{
		/*
		 * Setup initial states.
		 */

		DoMethod( obj, MM_Imageview_AddImage, NULL );
	}
	return (rc);
}


DEFSMETHOD(Imageview_Scroll)
{
	GETDATA;

	data->virt_x = minmax( 0, (LONG)msg->pos_x, (LONG)data->wanted_xs - _mwidth(obj) ) ;
	data->virt_y = minmax( 0, (LONG)msg->pos_y, (LONG)data->wanted_ys - _mheight(obj) ) ;

	{
		APTR hs = (APTR)getv(obj, MUIA_Scrollgroup_VertBar);

		if (hs)
		{
			SetAttrs(hs,
				MUIA_NoNotify, TRUE,
				MUIA_Prop_First, data->virt_y,
			TAG_DONE);
		}
	}

	{
		APTR hs = (APTR)getv(obj, MUIA_Scrollgroup_HorizBar);

		if (hs)
		{
			SetAttrs(hs,
				MUIA_NoNotify, TRUE,
				MUIA_Prop_First, data->virt_x,
			TAG_DONE);
		}
	}

	MUI_Redraw(obj, MADF_DRAWUPDATE);

	return (0);
}


DEFSMETHOD(Imageview_ScrollX)
{
	GETDATA;
	DoMethod( obj, MM_Imageview_Scroll, msg->pos_x, data->virt_y );
	return (0);
}


DEFSMETHOD(Imageview_ScrollY)
{
	GETDATA;
	DoMethod( obj, MM_Imageview_Scroll, data->virt_x, msg->pos_y );
	return (0);
}

DEFMMETHOD(AskMinMax)
{
	GETDATA;

	DOSUPER;

	msg->MinMaxInfo->MinWidth += 4; /* XXX: hm.. */
	msg->MinMaxInfo->MinHeight += 4;

	if (data->wanted_xs && data->wanted_ys)
	{
		msg->MinMaxInfo->DefWidth = max(msg->MinMaxInfo->MinWidth, data->wanted_xs);
		msg->MinMaxInfo->DefHeight = max(msg->MinMaxInfo->MinHeight, data->wanted_ys);
	}
	else
	{
		STRPTR buff = NULL;

		storage_get(STORAGE_IMAGEVIEW_SNAPSHOT, STORAGE_STRING, (APTR*)&buff);
		if (buff != NULL)
		{
			struct windowpos wp;
			struct RDArgs *rda;

			memset(&wp, 0, sizeof(wp));
			rda	= readargsstring(buff, "LEFT/N,TOP/N,WIDTH/N,HEIGHT/N", (ULONG*)&wp);

			if (rda != NULL)
			{
				msg->MinMaxInfo->DefWidth = *wp.width != 0 ? *wp.width : 640;
				msg->MinMaxInfo->DefHeight = *wp.height != 0 ? *wp.height : 480;

				freeargsstring(rda);
			}

		}
		else
		{
			msg->MinMaxInfo->DefWidth = 640;
			msg->MinMaxInfo->DefHeight = 480;
		}
	}

	msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
	msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;

	return (0);
}


DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_View_NeedsBackfill:
			*msg->opg_Storage = TRUE;	/* XXX: Just for now until i figure how to change it on the fly */
			return (TRUE);

		case MA_View_HasBackground:
			*msg->opg_Storage = FALSE;
			return (TRUE);

		case MA_View_ModeIndex:
			*msg->opg_Storage = data->viewmode == IVM_SCALED ? 0 : 1;
			return (TRUE);

		case MA_View_Type:
			*msg->opg_Storage = MV_View_Type_Image;
			return (TRUE);

		case MA_View_NewWin:
			*msg->opg_Storage = FALSE;//TRUE;
			return (TRUE);

		case MA_View_ShowDevices:
			*msg->opg_Storage = FALSE;
			return (TRUE);
	}
	return (DOSUPER);
}

DEFSET
{
	GETDATA;

	FORTAG(INITTAGS)
	{
		case MA_View_ModeIndex:

			if ( tag->ti_Data == 0 )
			{
				if ( data->viewmode != IVM_SCALED )
				{
					/* change to Scaled */

					data->viewmode = IVM_SCALED;
					DoMethod(obj, MM_Imageview_AddImage, NULL);
				}
			}
			else if ( tag->ti_Data == 1 )
			{
				if ( data->viewmode != IVM_NORMAL )
				{
					/* change to Normal */

					data->viewmode = IVM_NORMAL;
					DoMethod(obj, MM_Imageview_AddImage, NULL);

				}
			}
			break;
	}
	NEXTTAG

	return (DOSUPER);
}

static LONG setqual(LONG val, ULONG quals, LONG dir, ULONG stride)
{
	if (quals & IEQUALIFIER_CONTROLS)
	{
		val += dir * 10000; /* begin/end */
	}
	else if (quals & IEQUALIFIER_SHIFTS)
	{
		val += dir * stride;
	}
	else if (quals & IEQUALIFIER_ALTS)
	{
		val += dir * 64;
	}
	else
	{
		val += dir * 4;
	}

	if (quals & IEQUALIFIER_REPEAT)
	{
		val += dir * 32;
	}
	return (val);
}


DEFSMETHOD(Imageview_SetPos)
{
	LONG val = val; /* shut up gcc */
	GETDATA;

	switch (msg->dir)
	{
		case MV_Imageview_SetPos_TopIncrease:
			val = setqual(data->virt_y, msg->quals, 1, _mheight(obj));
			break;

		case MV_Imageview_SetPos_TopDecrease:
			val = setqual(data->virt_y, msg->quals, -1, _mheight(obj));
			break;

		case MV_Imageview_SetPos_LeftIncrease:
			val = setqual(data->virt_x, msg->quals, 1, _mwidth(obj));
			break;

		case MV_Imageview_SetPos_LeftDecrease:
			val = setqual(data->virt_x, msg->quals, -1, _mwidth(obj));
			break;
	}

	if ((msg->dir == MV_Imageview_SetPos_TopIncrease) || (msg->dir == MV_Imageview_SetPos_TopDecrease))
	{
		APTR hs = (APTR)getv(obj, MUIA_Scrollgroup_VertBar);

		data->virt_y = minmax(0, val, (LONG)data->wanted_ys - _mheight(obj));

		if (hs)
		{
			SetAttrs(hs,
				MUIA_NoNotify, TRUE,
				MUIA_Prop_First, data->virt_y,
			TAG_DONE);
		}
	}
	else
	{
		APTR vs = (APTR)getv(obj, MUIA_Scrollgroup_HorizBar);

		data->virt_x = minmax(0, val, (LONG)data->wanted_xs - _mwidth(obj));

		if (vs)
		{
			SetAttrs(vs,
				MUIA_NoNotify, TRUE,
				MUIA_Prop_First, data->virt_x,
			TAG_DONE);
		}
	}

	MUI_Redraw(obj, MADF_DRAWUPDATE);

	return (0);
}

DEFMMETHOD(HandleEvent)
{
	GETDATA;

	if (msg->imsg)
	{
		switch (msg->imsg->Class)
		{
			case IDCMP_RAWKEY:
			{
				LONG next = 1;

				switch (msg->imsg->Code)
				{
					case RAWKEY_PRTSCREEN:
						if (!data->printing && data->dtp)
						{
							APTR bm, newbm;

							bm = picture_getattr(data->dtp, PICTURE_BITMAP);
							newbm = gfx_bitmap_create(gfx_bitmap_width(bm), gfx_bitmap_height(bm), 24, BITMAPTAG_Format, BITMAPVAL_Format_RGB24, TAG_DONE);

							if (newbm)
							{
								gfx_blit(bm, newbm, TAG_DONE);

								if (do_action(obj, TA_Print, TT_Print_Source, newbm, TT_Print_Type, PRT_BITMAP, TAG_DONE))
								{
									data->printing = 1;
								}
								else
								{
									gfx_bitmap_delete(newbm);
								}
							}
						}
						break;

					case RAWKEY_UP:
						DoMethod(obj, MM_Imageview_SetPos, MV_Imageview_SetPos_TopDecrease, msg->imsg->Qualifier);
						break;

					case RAWKEY_DOWN:
						DoMethod(obj, MM_Imageview_SetPos, MV_Imageview_SetPos_TopIncrease, msg->imsg->Qualifier);
						break;

					case RAWKEY_HOME:
						DoMethod(obj, MM_Imageview_SetPos, MV_Imageview_SetPos_TopDecrease, IEQUALIFIER_CONTROLS);
						break;

					case RAWKEY_END:
						DoMethod(obj, MM_Imageview_SetPos, MV_Imageview_SetPos_TopIncrease, IEQUALIFIER_CONTROLS);
						break;

					case RAWKEY_LEFT:
						DoMethod(obj, MM_Imageview_SetPos, MV_Imageview_SetPos_LeftDecrease, msg->imsg->Qualifier);
						break;

					case RAWKEY_RIGHT:
						DoMethod(obj, MM_Imageview_SetPos, MV_Imageview_SetPos_LeftIncrease, msg->imsg->Qualifier);
						break;

					case NM_WHEEL_UP:
						if (_isinobject(msg->imsg->MouseX, msg->imsg->MouseY))
						{
							DoMethod(obj, MM_Imageview_SetPos, MV_Imageview_SetPos_TopDecrease, IEQUALIFIER_ALTS | msg->imsg->Qualifier);
						}
						break;

					case NM_WHEEL_DOWN:
						if (_isinobject(msg->imsg->MouseX, msg->imsg->MouseY))
						{
							DoMethod(obj, MM_Imageview_SetPos, MV_Imageview_SetPos_TopIncrease, IEQUALIFIER_ALTS | msg->imsg->Qualifier);
						}
						break;

					case RAWKEY_PAGEUP:
						next = 0;
					case RAWKEY_PAGEDOWN:
						{
							if (msg->imsg->Qualifier & IEQUALIFIER_REPEAT)
							{
								if (data->state == PICSTATE_LOADING || data->state == PICSTATE_SCALING)
								{
									DB(("Break\n"));
									break;
								}
							}

							if (data->picliststate == PICLIST_NONE)
							{
								data->picsnum = 0;
								data->picpos = 0;
								if (do_action(obj, TA_Imageview_ScanPics,
									TT_Imageview_ScanPics_Path, getv(obj, MA_View_Path),
									TT_Imageview_ScanPics_Mode,	next ? TV_Imageview_ScanPics_Mode_Next : TV_Imageview_ScanPics_Mode_Prev,
								TAG_DONE))
								{
									data->picliststate = PICLIST_SCANNING;
								}
							}
							else if (data->picliststate == PICLIST_SCANNED)
							{
								TEXT t[PATH_SIZE];

								if ( !ISLISTEMPTY(&data->piclist) )
								{
									if ( next )
									{
										if ((data->pn = NEXTNODE(data->pn)) && NEXTNODE(data->pn))
										{
											data->picpos++;
										}
										else
										{
											data->pn = FIRSTNODE(&data->piclist);
											data->picpos = 0;
										}
									}
									else
									{
										if ((data->pn = PREVNODE(data->pn)) && PREVNODE(data->pn))
										{
											data->picpos--;
										}
										else
										{
											data->pn = LASTNODE(&data->piclist);
											data->picpos = data->picsnum - 1;
										}
									}

									ASSERT(data->pn);

									stccpy(t, (STRPTR)getv(obj, MA_View_Path), sizeof(t));
									*FilePart(t) = '\0';
									AddPart(t, data->pn->name, sizeof(t));

									/* XXX: prevent from moving until the image is loaded or something.. */
									{
										STRPTR cmd = malloc( sizeof("LoadURI") -1 + strlen( t ) + 32 );
										if ( cmd )
										{
											sprintf( cmd, "LoadURI \"%s\" VIEWID %ld", t, (LONG)getv( _win(obj), MA_Window_ID ) );
											data->state = PICSTATE_LOADING;
											execute_command(NULL, AC_INTERNAL, cmd, NULL);
											free( cmd );
										}
									}
								}
							}
							break;

						}
						break;

					default: /* VANILLAKEY */
						switch (keymap_vanilla(msg->imsg))
						{
							case 'l':
								{
									/*
									 * Rotate left.
									 */

									APTR newbm = gfx_rotate( picture_getattr( data->dtp, PICTURE_BITMAP ), ROTATE_270 );

									if ( newbm )
									{
										picture_set_bitmap( data->dtp, newbm );
										DoMethod( obj, MM_Imageview_AddImage, NULL );
									}
								}
								break;

							case 'r':
								{
									/*
									 * Rotate right.
									 */

									APTR newbm = gfx_rotate( picture_getattr( data->dtp, PICTURE_BITMAP ), ROTATE_90 );

									if ( newbm )
									{
										picture_set_bitmap( data->dtp, newbm );
										DoMethod( obj, MM_Imageview_AddImage, NULL );
									}
								}
								break;
						}
				}
			}
			break;

			case IDCMP_MOUSEBUTTONS:
			{
				if ( _isinobject(msg->imsg->MouseX, msg->imsg->MouseY) )
				{
					if ( msg->imsg->Code == SELECTDOWN || msg->imsg->Code == MIDDLEDOWN )
					{
						if ( data->viewmode != IVM_SCALED )
						{
							data->mouse_x = msg->imsg->MouseX;
							data->mouse_y = msg->imsg->MouseY;
							data->ehnode.ehn_Events |= IDCMP_MOUSEMOVE;
						}
					}
				}

				if ( msg->imsg->Code == SELECTUP || msg->imsg->Code == MIDDLEUP )
				{
					if ( data->viewmode != IVM_SCALED )
					{
						data->ehnode.ehn_Events &= ~IDCMP_MOUSEMOVE;
					}
				}
			}
			break;

			case IDCMP_MOUSEMOVE:
			{

				LONG dx = msg->imsg->MouseX - data->mouse_x;
				LONG dy = msg->imsg->MouseY - data->mouse_y;

				DoMethod(obj, MM_Imageview_Scroll, data->virt_x - dx, data->virt_y - dy);

				data->mouse_x = msg->imsg->MouseX;
				data->mouse_y = msg->imsg->MouseY;
			}
			break;

		}
	}
	return (0);
}


DEFMMETHOD(Backfill)
{
	#if USE_DBUF
	GETDATA;

	if (!data->dbuf)
	#endif
	{
		SetRPAttrs(_rp(obj),
			RPTAG_PenMode, FALSE,
			RPTAG_FgColor, 0xff000000,
		TAG_DONE);

		RectFill(_rp(obj), msg->left, msg->top, msg->right, msg->bottom);

		SetRPAttrs(_rp(obj),
			RPTAG_PenMode, TRUE,
		TAG_DONE);

		//EraseRect(_rp(obj), msg->left, msg->top, msg->right, msg->bottom);
	}
	return (0);
}


DEFSMETHOD(Imageview_LoadImage)
{
	GETDATA;

	data->state = PICSTATE_LOADING;

	/*
	 * We are first waiting for previous one to finish.
	 * In Thread_Finished we will begin loading pending image.
	 */

	data->pendingpath = name_build( msg->path );
	threads_abort(obj, TA_Imageview_Scale, TAG_DONE);

	DoSuperMethod(cl, obj, MUIM_Set, MA_View_Path, msg->path); /* set the new path */

	DoSuperMethod(cl, obj, MM_View_SetStatus, MF_View_SetStatus_Window | MF_View_SetStatus_Bar | MF_View_SetStatus_Busy, "Loading image..");

	return (0);
}


DEFTMETHOD(View_LoadURI)
{
	/*
	 * Load new image into view.
	 */

	DB(("LoadImage:%s\n",getv(obj, MA_View_Path)));
	DoMethod(obj, MM_Imageview_LoadImage, getv(obj, MA_View_Path));

	return (0);
}


DEFSMETHOD(Imageview_AddImage)
{
	STRPTR p;
	ULONG pic_xs, pic_ys;
	APTR hs, vs; // horiz/vert scoller
	GETDATA;
	
	if ( msg->dtp )
	{
		gfx_bitmap_delete(data->scalebm);
		data->scalebm = NULL;

		PDB(("Deleting old... 0x%08lx\n", data->dtp));
		if (data->dtp)
		{
			picture_delete(data->dtp);
		}

		data->firstshow = TRUE;
		data->dtp = msg->dtp;
	}

	if (!data->dtp)
	{
		return (0);
	}

	pic_xs = (ULONG)picture_getattr(data->dtp, PICTURE_WIDTH);
	pic_ys = (ULONG)picture_getattr(data->dtp, PICTURE_HEIGHT);
	
	/*
	 * If the image is bigger than the current screen,
	 * check the aspect ratio to have a window of the
	 * proper size.
	 */

	if (!data->viewmode == IVM_SCALED || ((pic_xs < data->screen_width) && (pic_ys < data->screen_height)))
	{
		data->wanted_xs = pic_xs;
		data->wanted_ys = pic_ys;
	}
	else
	{
		data->wanted_xs = data->screen_width;
		data->wanted_ys = data->screen_height;

		gfx_scale_calc_aspect_constraints(pic_xs, pic_ys, &data->wanted_xs, &data->wanted_ys);
	}

	/* force a resize (XXX: that sucks.. is there some better way? ask stuntzi). actually ChangeWindowBox() would work :) */
	if (data->mode == MODE_NORMAL)
	{
		/*
		 * Removed. These cause a call to Cleanup method
		 * which sets <savewin> pointer which then prohibits any further change
		 * of viewobject.
		 * TODO: Handle it in some nicer way maybe. This picture viewer is a mess
		 */
		//set(win, MUIA_Window_Open, FALSE);
		//set(win, MUIA_Window_Open, TRUE);
	}

	if (data->viewmode == IVM_SCALED)
	{
		DoMethod(obj, MM_Imageview_Scale);
	}
	else
	{
		/* spare up some memory */

		gfx_bitmap_delete(data->scalebm);
		data->scalebm = NULL;

		data->state = PICSTATE_IDLE;

		if (data->firstshow)
		{
			/* center */
			data->virt_x = (data->wanted_xs - _mwidth(obj)) / 2;
			data->virt_y = (data->wanted_ys - _mheight(obj)) / 2;

			data->firstshow = FALSE;
		}

		if (data->virt_x > data->wanted_xs - _mwidth(obj))
		{
			data->virt_x = data->wanted_xs - _mwidth(obj);
		}

		if (data->virt_y > data->wanted_ys - _mheight(obj))
		{
			data->virt_y = data->wanted_ys - _mheight(obj);
		}
	}

	/*
	 * Set scrollers. now for both viewmodes
	 */

	hs = (APTR)getv(obj, MUIA_Scrollgroup_HorizBar);
	vs = (APTR)getv(obj, MUIA_Scrollgroup_VertBar);

	ASSERT(hs);
	ASSERT(vs);

	SetAttrs(hs,
		MUIA_NoNotify, TRUE,
		MUIA_Prop_First, data->viewmode == IVM_SCALED ? 0 : data->virt_x,
		MUIA_Prop_Entries, data->viewmode == IVM_SCALED ? 0 : data->wanted_xs,
		MUIA_Prop_Visible, data->viewmode == IVM_SCALED ? 0 : _width(obj),
	TAG_DONE);

	SetAttrs(vs,
		MUIA_NoNotify, TRUE,
		MUIA_Prop_First, data->viewmode == IVM_SCALED ? 0 : data->virt_y,
		MUIA_Prop_Entries, data->viewmode == IVM_SCALED ? 0 : data->wanted_ys,
		MUIA_Prop_Visible, data->viewmode == IVM_SCALED ? 0 : _height(obj),
	TAG_DONE);

	if (data->viewmode != IVM_SCALED)
	{
		if (!data->has_scrollnotify) /* XXX: killnotify when switching scroll or not.. but I had problems there */
		{
			DoMethod(hs, MUIM_Notify,
				MUIA_Prop_First, MUIV_EveryTime,
				obj,
				3, MM_Imageview_ScrollX, MUIV_TriggerValue
			);
			DoMethod(vs, MUIM_Notify,
				MUIA_Prop_First, MUIV_EveryTime,
				obj,
				3, MM_Imageview_ScrollY, MUIV_TriggerValue
			);
			data->has_scrollnotify = TRUE;
		}
	}

	/*
	 * Status update (only if we were given some new image).
	 */

	if ( msg->dtp && (p = (STRPTR)getv(obj, MA_View_Path)) )
	{
		/* XXX: should be better.. and why not standarize the path cooking function also in iconviewclass ? */

		if (data->picliststate == PICLIST_SCANNED)
		{
			DoSuperMethod(cl, obj, MM_View_SetStatus, MF_View_SetStatus_Window, "%s (%lu x %lu) [%lu / %lu]", FilePart(p), data->wanted_xs, data->wanted_ys, data->picpos + 1, data->picsnum);
		}
		else
		{
			DoSuperMethod(cl, obj, MM_View_SetStatus, MF_View_SetStatus_Window, "%s (%lu x %lu)", FilePart(p), data->wanted_xs, data->wanted_ys);
		}
	}

	#if USE_DBUF
	if (data->dbuf && data->viewmode != IVM_SCALED)
	{
		if (data->dtp)
		{
			blit_dbuf(picture_getattr(data->dtp, PICTURE_BITMAP), data->dbuf);
		}
	}
	#endif

	MUI_Redraw(obj, MADF_DRAWUPDATE);
	return (0);
}


DEFSMETHOD(Imageview_AddError)
{
	DoSuperMethod(cl, obj, MM_View_SetStatus, MF_View_SetStatus_Window, "Datatype error: %s", picture_errorstring(msg->err));

	return (0);
}

/*
 * Called from scaling thread.
 */

DEFSMETHOD(Imageview_AddImageScaled)
{
	STRPTR p;
	GETDATA;

	ASSERT(msg->bm);

	data->state = PICSTATE_SCALED;

	/*
	 * That is because of imperfect threads handling and stuff.
	 * We might get here while view is in dispose-pending state.
	 */

	if ( !muiRenderInfo(obj) )
	{
		gfx_bitmap_delete(msg->bm);
		return 0;
	}

	/*
	 * We missed a scaling. Need to
	 * do it again.
	 */
	if (data->pendingscale)
	{
		#if USE_DBUF
		if (data->dbuf)
		{
			fill_dbuf(data->dbuf);
			blit_dbuf(msg->bm, data->dbuf);
		}
		#endif
		gfx_bitmap_delete(msg->bm);

		data->pendingscale = FALSE;
		DoMethod(obj, MM_Imageview_Scale);

		return (0);
	}

	gfx_bitmap_delete(data->scalebm);

	data->scalebm = msg->bm;

	if ( (p = (STRPTR)getv(obj, MA_View_Path)) )
	{
		/* XXX: should be better.. and why not standarize the path cooking function also in iconviewclass ? (XXX: also used above!) */

		if (data->picliststate == PICLIST_SCANNED)
		{
			DoSuperMethod(cl, obj, MM_View_SetStatus, MF_View_SetStatus_Window, "%s (%lu x %lu -> %lu x %lu) [%lu / %lu]", FilePart(p), picture_getattr(data->dtp, PICTURE_WIDTH), picture_getattr(data->dtp, PICTURE_HEIGHT), gfx_bitmap_width(data->scalebm), gfx_bitmap_height(data->scalebm), data->picpos + 1, data->picsnum);
		}
		else
		{
			DoSuperMethod(cl, obj, MM_View_SetStatus, MF_View_SetStatus_Window, "%s (%lu x %lu -> %lu x %lu)", FilePart(p), picture_getattr(data->dtp, PICTURE_WIDTH), picture_getattr(data->dtp, PICTURE_HEIGHT), gfx_bitmap_width(data->scalebm), gfx_bitmap_height(data->scalebm));
		}
	}

	#ifdef BENCHMARK
	if (!data->bench)
	{
		data->timer = timedm();
	}

	if (data->bench++ < BENCHTIMES)
	{
		data->state = PICSTATE_SCALING;

		do_action(obj, TA_RESCALEIMAGE,
			TT_DTP, data->dtp,
			TT_XS, gfx_bitmap_width(data->scalebm),
			TT_YS, gfx_bitmap_height(data->scalebm),
		TAG_DONE);

		return (0);
	}
	else if (data->bench == BENCHTIMES + 1)
	{
		ULONG time = timedm();

		PDB(("scaled %ld times. elapsed time: %ld milisecs\n", data->bench - 1, time - data->timer));
	}
	#endif

	#if USE_DBUF
	if (data->dbuf && data->viewmode == IVM_SCALED)
	{
		if (data->scalebm)
		{
			fill_dbuf(data->dbuf);
			blit_dbuf(data->scalebm, data->dbuf);
		}
	}
	#endif
	MUI_Redraw(obj, MADF_DRAWOBJECT);

	return (0);
}


DEFMMETHOD(Draw)
{
	GETDATA;

	/* XXX */
	if (!(msg->flags & MADF_DRAWUPDATE))
	{
		DOSUPER;
	}

	if (msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE))
	{
		#if USE_DBUF
		if (data->viewmode == IVM_SCALED && data->use_dbuf)
		{
			ASSERT(data->dbuf);

			gfx_blit(data->dbuf->bm, _rp(obj),
				BLITTAG_DstType, BLITVAL_DstType_RastPort,
				BLITTAG_SrcX, gfx_bitmap_width(data->dbuf->bm) / 2 - _mwidth(obj) / 2,
				BLITTAG_SrcY, gfx_bitmap_height(data->dbuf->bm) / 2 - _mheight(obj) / 2,
				BLITTAG_DstX, _mleft(obj),
				BLITTAG_DstY, _mtop(obj),
				BLITTAG_DstWidth, _mwidth(obj),
				BLITTAG_DstHeight, _mheight(obj),
			TAG_DONE);
		}
		else
		#endif
		{
			if (data->dtp)
			{
				APTR bm = picture_getattr(data->dtp, PICTURE_BITMAP);

				if (data->viewmode == IVM_SCALED)
				{
					if (data->scalebm)
					{
						bm = data->scalebm;
					}

					if (data->state == PICSTATE_SCALING)
					{
						/* draw nothing */
					}
					else
					{
						gfx_blit(bm, _rp(obj),
							BLITTAG_DstType, BLITVAL_DstType_RastPort,
							BLITTAG_DstX, _mleft(obj) + (_mwidth(obj) - gfx_bitmap_width(bm)) / 2,
							BLITTAG_DstY, _mtop(obj) + (_mheight(obj) - gfx_bitmap_height(bm)) / 2,
						TAG_DONE);
					}
				}
				else
				{
					/*
					 * Since window may be bigger than image, we have to center image in it and fill borders.
					 */

					int ox = 0;
					int oy = 0;
					int vx = data->virt_x;
					int vy = data->virt_y;

					if	( gfx_bitmap_width(bm) < _mwidth(obj) )
					{
						ox -= ( (int)gfx_bitmap_width(bm) - (int)_mwidth(obj) ) / 2;
						vx = 0;
					}
					if	( gfx_bitmap_height(bm) < _mheight(obj) )
					{
						oy -= ( (int)gfx_bitmap_height(bm) - (int)_mheight(obj) ) / 2;
						vy = 0;
					}


					gfx_blit(bm, _rp(obj),
						BLITTAG_DstType, BLITVAL_DstType_RastPort,
						BLITTAG_SrcX, vx,
						BLITTAG_SrcY, vy,
						BLITTAG_DstX, _mleft(obj) + ox,
						BLITTAG_DstY, _mtop(obj) + oy,
						BLITTAG_DstWidth, min( _mwidth(obj), gfx_bitmap_width(bm) ),
						BLITTAG_DstHeight, min( _mheight(obj), gfx_bitmap_height(bm) ),
					TAG_DONE);


					/* fill borders if they exist */

					SetRPAttrs(_rp(obj),
						RPTAG_PenMode, FALSE,
						RPTAG_FgColor, 0xff000000,
					TAG_DONE);

					/* left and right */

					if	( ox > 0 )
					{
						int bl = ox;
						int br = _mwidth(obj) - gfx_bitmap_width(bm) - bl;

						RectFill(_rp(obj), _mleft(obj), _mtop(obj), _mleft(obj)+bl-1, _mtop(obj) + _mheight(obj) );
						RectFill(_rp(obj), _mleft(obj) + _mwidth(obj) - br, _mtop(obj), _mleft(obj) + _mwidth(obj), _mtop(obj) + _mheight(obj) );
					}

					/* top and bottom */

					if	( oy > 0 )
					{
						int bt = oy;
						int bb = _mheight(obj) - gfx_bitmap_height(bm) - bt;

						RectFill(_rp(obj), _mleft(obj), _mtop(obj), _mleft(obj) + _mwidth(obj),bt + _mtop(obj)-1 );
						RectFill(_rp(obj), _mleft(obj), _mtop(obj) + _mheight(obj) - bb, _mleft(obj) + _mwidth(obj), _mtop(obj) + _mheight(obj) );
					}

					/* restore rastport params */

					SetRPAttrs(_rp(obj),
						RPTAG_PenMode, TRUE,
					TAG_DONE);
				}
			}
		}
	}

	/* XXX: nothing hu? hm.. */

	return (0);
}


DEFSMETHOD(View_Refresh)
{
	return (0);
}


DEFMMETHOD(Cleanup)
{
	GETDATA;

	#if USE_DBUF
	if (data->dbuf)
	{
		gfx_dbuf_free(data->dbuf);
		data->dbuf = NULL;
	}
	#endif

	gfx_bitmap_delete(data->scalebm);
	data->scalebm = NULL;

	if (muiRenderInfo(obj) && _win(obj))
	{
		DoMethod(_win(obj), MUIM_Window_RemEventHandler, (ULONG)&data->ehnode);
	}
	return (DOSUPER);
}


DEFDISPOSE
{
	struct picnode *pn, *nextpn;
	GETDATA;

	ITERATELISTSAFE(pn, nextpn, &data->piclist) /* XXX: wrong.. should be in thread abort or so.. or not? hm.. */
	{
		free(pn);
	}

	if (data->dtp)
	{	
		picture_delete(data->dtp);
	}

	gfx_bitmap_delete(data->scalebm);
	return (DOSUPER);
}


DEFSMETHOD(Imageview_AddImageNode)
{
	struct picnode *pn;
	STRPTR p;

	p = FilePart(msg->path);

	if ( (pn = malloc(sizeof(*pn) + strlen(p) + 1)) )
	{
		GETDATA;

		strcpy(pn->name, p);

		ADDTAIL(&data->piclist, pn);

		data->picsnum++;

		return (TRUE);
	}
	/* XXX */
	return (FALSE);
}


static int cmp_name(struct picnode **pn1, struct picnode **pn2)
{
	return (Stricmp((*pn1)->name, (*pn2)->name));
}


DEFSMETHOD(Thread_Finished)
{
	GETDATA;

	if (msg->status == MV_Thread_Finished_Abort)
	{
		ULONG callsuper = TRUE;

		switch(msg->action)
		{
			case TA_Print:
				data->printing = 0;
				callsuper = FALSE;
				break;

			case TA_Imageview_Scale:
				{
					threads_abort(obj, TA_Imageview_Load, NULL );

					callsuper = FALSE;

					break;
				}

			case TA_Imageview_Load:
				{

					/*
					 * previous loading finished. we have to check if new one is needed.
					 */

					if ( data->pendingpath )
					{
						do_action(obj, TA_Imageview_Load,
							TT_Imageview_Load_Path, data->pendingpath,
						TAG_DONE);

						name_delete( data->pendingpath );
						data->pendingpath = NULL;
					}

					break;
				}
		}

		if ( callsuper )
			return (DOSUPER);
		else
			return 0;
	}
	else
	{
		switch (msg->action)
		{
			case TA_Print:
				data->printing = 0;
				break;

			case TA_Imageview_ScanPics:
				{
					GETDATA;
					DB(("Scandir finished!\n"));
					if (msg->status == MV_Thread_Finished_Ok)
					{
						struct picnode **a;
						/* XXX: check msg->status.. or ? */

						/*
						 * Sort the list.
						 */

						DB(("Picsnum:%d\n",data->picsnum));

						if ( (a = malloc(sizeof(struct picnode *) * data->picsnum)) )
						{
							struct picnode **pptr = a;
							struct picnode *pn;
							ULONG picsnum = data->picsnum;

							ITERATELIST(pn, &data->piclist)
							{
								*pptr++ = pn;
							}

							qsort(a, data->picsnum, sizeof(struct picnode *), (const void *)cmp_name);

							NEWLIST(&data->piclist);

							pptr = a;

							while (picsnum)
							{
								ADDTAIL(&data->piclist, *pptr++);
								picsnum--;
							}
							free(a);
						}

						data->picliststate = PICLIST_SCANNED;
						if (ISLISTEMPTY(&data->piclist))
						{
							data->pn = NULL;
						}
						else
						{
							TEXT t[PATH_SIZE]; /* XXX: duplicate.. change the loadimage schemantic ? */
							struct picnode *pn;

							data->pn = NULL;

							/*
							 * Position within the current picture.
							 * The file might NOT be on the list.
							 */

							ITERATELIST( pn, &data->piclist )
							{
								if ( 0 == stricmp(pn->name, FilePart((STRPTR)getv(obj, MA_View_Path))) )
								{
									data->pn = pn;
									break;
								}
								data->picpos++;
							}

							if ( !data->pn )
							{
								/* XXX: Should fix scanning instead of this one, but can be done later. */
								data->pn = FIRSTNODE( &data->piclist );
							}

							/*
							 * And go to the next one or the previous one
							 * depending on what the user pressed.
							 */
							switch (GetTagData(TT_Imageview_ScanPics_Mode, TV_Imageview_ScanPics_Mode_Next, msg->taglist))
							{
								case TV_Imageview_ScanPics_Mode_Next:
									if ((data->pn = NEXTNODE(data->pn)) && NEXTNODE(data->pn))
									{
										data->picpos++;
									}
									else
									{
										data->pn = FIRSTNODE(&data->piclist);
										data->picpos = 0;
									}
									break;

								case TV_Imageview_ScanPics_Mode_Prev:
									if ((data->pn = PREVNODE(data->pn)) && PREVNODE(data->pn))
									{
										data->picpos--;
									}
									else
									{
										data->pn = LASTNODE(&data->piclist);
										data->picpos = data->picsnum - 1;
									}
									break;

								#ifdef DEBUG
								default:
									PDB(("unknown mode\n"));
									break;
								#endif
							}

							stccpy(t, (STRPTR)getv(obj, MA_View_Path), sizeof(t));
							*FilePart(t) = '\0';
							AddPart(t, data->pn->name, sizeof(t));

							//DoMethod(obj, MM_Imageview_LoadImage, t);
							//bitRocky: using LoadURI instead MM_Imageview_LoadImage, also updates the path for the first image after PageDown/Up
							/* XXX: prevent from moving until the image is loaded or something.. */
							{
								STRPTR cmd = malloc( sizeof("LoadURI") -1 + strlen( t ) + 32 );
								if ( cmd )
								{
									sprintf( cmd, "LoadURI \"%s\" VIEWID %ld", t, (LONG)getv( _win(obj), MA_Window_ID ) );
									data->state = PICSTATE_LOADING;
									execute_command(NULL, AC_INTERNAL, cmd, NULL);
									free( cmd );
								}
							}
						}
					}
					else
					{
						struct picnode *pn, *nextpn;

						/* hm.. there was some error (XXX: tell so) */
						data->picliststate = PICLIST_NONE;

						ITERATELISTSAFE(pn, nextpn, &data->piclist)
						{
							free(pn);
						}
						NEWLIST(&data->piclist);
					}
				}
				break;
			case	TA_Imageview_Load:
				{
					/*
					 * This one is called after image is loaded.
					 */
				}
				break;
		}
		return (0);
	}
}

DEFMMETHOD(DragQuery)
{

	/* i wonder if numsel == 0 still happens */
	if ( getv(msg->obj, MA_View_NumSelected) == 0 )
	{
		/* single file dropped */

		LONG ft = getv(msg->obj, MA_Icon_FileType);

		if ( ft == MV_Icon_FileType_File )
		{
			return (MUIV_DragQuery_Accept);
		}
	}
	else if ( getv(msg->obj, MA_View_NumSelected) >= 1 )
	{
		/* multiple files dropped. check only first one (XXX: wasting resources to get all selected files) */
		struct MinList ml;
		struct dragdropnode *ddn, *nextddn;
		ULONG ret = MUIV_DragQuery_Accept;

		NEWLIST(&ml);
		DoMethod(_view(msg->obj), MM_View_GetSelectionList, &ml);

		ITERATELISTSAFE(ddn, nextddn, &ml)
		{
			if ( ddn->type != MV_Icon_FileType_File )
			{
				ret = MUIV_DragQuery_Refuse;
				/* NOTE: no break since we must free all dragdropnodes regardless */
			}
			free(ddn);
		}

		return (ret);
	}

	return (MUIV_DragQuery_Refuse);
}


DEFMMETHOD(DragBegin)
{
	/*
	 * MUI really sucks at redrawing the frame (flicker galore)
	 */

	/* XXX: implement own frame drawing */

	return (0);
}

DEFMMETHOD(DragDrop)
{
	GETDATA;
	STRPTR path = NULL;

	/* i wonder if numsel == 0 still happens */
	if ( getv(msg->obj, MA_View_NumSelected) == 0 )
	{
		path = name_build( (STRPTR)getv( msg->obj, MA_Icon_Path ) );
	}
	else if ( getv(msg->obj, MA_View_NumSelected) >= 1 )
	{
		struct MinList ml;
		struct dragdropnode *ddn, *nextddn;

		NEWLIST(&ml);
		DoMethod(_view(msg->obj), MM_View_GetSelectionList, &ml);

		ITERATELISTSAFE(ddn, nextddn, &ml)
		{
			if ( path == NULL )
			{
				path = name_build( ddn->path );
			}
			free(ddn);
		}
	}

	if ( path != NULL )
	{
		STRPTR cmd = malloc( sizeof("LoadURI") -1 + strlen( path ) + 32 );
		if ( cmd != NULL )
		{
			sprintf( cmd, "LoadURI \"%s\" VIEWID %ld", path, (LONG)getv( _win(obj), MA_Window_ID ) );
			data->state = PICSTATE_LOADING;
			execute_command(NULL, AC_INTERNAL, cmd, NULL);
			free(cmd);
		}

		free(path);
	}

	return (0);
}

DEFSMETHOD(Rexx_Snapshot)
{
	LONG left, top;
	LONG width, height;
	TEXT buff[128];

	left = getv(_win(obj), MUIA_Window_LeftEdge);
	top	= getv(_win(obj), MUIA_Window_TopEdge);
	width = _width(obj);
	height = _height(obj);

	snprintf(buff, sizeof(buff), "%ld %ld %ld %ld", left, top, width, height);
	storage_set(STORAGE_IMAGEVIEW_SNAPSHOT, STORAGE_STRING, buff);

	return (0);
}

DEFSMETHOD(Rexx_Unsnapshot)
{
	/* XXX: Note to self: add storage_remove() */
	storage_set(STORAGE_IMAGEVIEW_SNAPSHOT, STORAGE_STRING, "");
	return (DOSUPER);
}

BEGINMTABLE
DECNEW
DECDISPOSE
DECSMETHOD(Imageview_Scroll)
DECSMETHOD(Imageview_ScrollX)
DECSMETHOD(Imageview_ScrollY)
DECGET
DECSET
DECMMETHOD(Show)
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECMMETHOD(Backfill)
DECMMETHOD(AskMinMax)
DECSMETHOD(Imageview_SetPos)
DECMMETHOD(HandleEvent)
DECMMETHOD(Draw)
DECSMETHOD(Imageview_LoadImage)
DECTMETHOD(View_LoadURI)
DECSMETHOD(View_Refresh)
DECTMETHOD(Imageview_Scale)
DECSMETHOD(Imageview_AddImage)
DECSMETHOD(Imageview_AddError)
DECSMETHOD(Imageview_AddImageScaled)
DECSMETHOD(Imageview_AddImageNode)
DECSMETHOD(Thread_Finished)
DECMMETHOD(DragQuery)
DECMMETHOD(DragBegin)
DECMMETHOD(DragDrop)
DECSMETHOD(Rexx_Snapshot)
DECSMETHOD(Rexx_Unsnapshot)
ENDMTABLE

DECSUBCLASSPTR_NC(viewclass, imageviewclass)


static ULONG enterdir(APTR obj UNUSED, CONST_STRPTR path UNUSED, APTR userdata)
{
	ULONG *cnt = (ULONG *)userdata;

	if (*cnt)
	{
		return (RECURSE_SKIP); /* don't go into subdirs */
	}
	(*cnt)++;

	return (TRUE);
}


static ULONG addfile(APTR obj, CONST_STRPTR path, LONG type UNUSED, ULONG prot UNUSED, UQUAD size UNUSED, APTR userdata UNUSED, CONST_STRPTR comment UNUSED)
{
	return (methodstack_push_sync(obj, 2, MM_Imageview_AddImageNode, path));
}


ULONG tr_scanforpics(APTR obj, CONST_STRPTR path)
{
	ULONG cnt = 0;
	TEXT t[PATH_SIZE];

	THREAD;

	stccpy(t, path, sizeof(t));
	*FilePart(t) = '\0';

	return (recurse(obj, t, "#?.(jpg|jpeg|iff|tiff|gif|png|ilbm|brush|tga|jfif|bmp|pic|pcx|pcd|ico|lbm|tif|ppm)", enterdir, NULL, addfile, &cnt));
}


#endif /* USE_VIEW_IMAGE */
