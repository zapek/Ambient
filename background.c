/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2018 Ambient Open Source Team
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
 * $Id: background.c,v 1.24 2019/12/05 21:58:14 jacadcaps Exp $
 */

#include "ambient.h"

/* public */
#include <dos/dos.h>
#include <intuition/extensions.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <datatypes/pictureclass.h>
#include <graphics/rpattr.h>
#include <intuition/extensions.h>
#include <intuition/monitorclass.h>

/* private */
#include "background.h"
#include "datatypes_picture.h"
#include "common_picture.h"
#include "classes.h"
#include "methodstack.h"
#include "smartreq.h"
#include "screen.h"
#include "background.h"
#include "gfx_bitmap.h"
#include "gfx_scale.h"
#include "gfx_blit.h"
#include "recurse.h"
#include "random.h"

/************************************************************************/

struct bgfilenode {
	struct MinNode n;
	TEXT           path[0];
};

struct bgfilectx {
	struct MinList l;
	ULONG          cnt;
	ULONG          dircnt;
};

/************************************************************************/

static ULONG enterdir( APTR obj UNUSED, CONST_STRPTR path UNUSED, APTR userdata )
{
	struct bgfilectx *bgctx = (struct bgfilectx *) userdata;

	if( bgctx->dircnt )
	{
		return( RECURSE_SKIP ); /* don't go into subdirs */
	}
	bgctx->dircnt++;

	return( TRUE );
}

/************************************************************************/

static ULONG addfile( APTR obj UNUSED, CONST_STRPTR path, LONG type, ULONG prot UNUSED, UQUAD size, APTR userdata, CONST_STRPTR comment UNUSED )
{
	struct bgfilectx *bgctx = (struct bgfilectx *) userdata;

	if( type == ST_FILE && size )
	{
		struct bgfilenode *bgfn;

		if ( ( bgfn = malloc( sizeof(*bgfn) + strlen( path ) + 1 ) ) )
		{
			strcpy( bgfn->path, path );
			bgctx->cnt++;
			ADDTAIL( &bgctx->l, bgfn );
		} else {
			return( FALSE );
		}
	}
	return( TRUE ); /* XXX: we should really have some new retcode or so */
}

/************************************************************************/

/*
 * Normally called from backfillhooks to copy in the background image into a
 * given rastport.
 */
void background_blit( ULONG bgmode, APTR bm, ULONG bgpen, int xoffs, int yoffs, struct RastPort *rp, WORD minx, WORD miny, WORD maxx, WORD maxy)
{
	if( bm )
	{
		struct Rectangle bounds = { minx, miny, maxx, maxy };

		gfx_blit_tiled( bm, xoffs, yoffs, rp->BitMap, &bounds );
	} else {
		struct Layer *ol;

		ol = rp->Layer;
		rp->Layer = NULL;
		if( bgmode == BGRENDER_Color )
		{
			SetRPAttrs( rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, bgpen, TAG_DONE );
		} else {
			SetRPAttrs( rp, RPTAG_PenMode, TRUE, RPTAG_APen, 0, TAG_DONE );
		}
		RectFill( rp, minx, miny, maxx, maxy );
		rp->Layer = ol;
	}
}

/************************************************************************/

STATIC ULONG background_load( APTR obj, ULONG type, ULONG mode, CONST_STRPTR filename, BPTR lock, struct Screen *screen );

/*
 * Loads a background. Cannot be aborted due to the datatype's
 * design.
 */
ULONG tr_background_load( APTR obj, ULONG type, ULONG mode, CONST_STRPTR filename )
{
	ULONG rc = TRUE;
	THREAD;
	CHECKOBJECT(obj);

	if( mode != BGRENDER_Color )
	{
		if( filename && *filename )
		{
			BPTR l;
			
			if ( ( l = Lock( filename, ACCESS_READ ) ) )
			{
				struct Screen *screen = screen_lock();

				if (screen)
				{
					rc = background_load(obj, type, mode, filename, l, screen);
					screen_unlock(screen);
				}

				UnLock(l);
			}
			else
			{
				smartreq_info("Ambient Background", MV_Notification_Error, "Background %s not found.\nEither restore the path/file or change\nSettings/Ambient settings../Backgrounds", filename);
				return( FALSE );
			}
		}
	}

	return rc;
}		

STATIC ULONG background_load( APTR obj, ULONG type, ULONG mode, CONST_STRPTR filename, BPTR lock, struct Screen *screen )
{
				TEXT file[ PATH_SIZE ];
				APTR dtp;
				ULONG retval = FALSE;
				ULONG showerror = TRUE;
				D_S(struct FileInfoBlock, fib);

				if( Examine( lock, fib ) )
				{
					ULONG compositing = FALSE;

					if( fib->fib_DirEntryType > 0 )
					{
						struct bgfilectx bgctx;
						/*
						 * User specified a directory. Try to get
						 * a random file from it.
						 */

						NEWLIST( &bgctx.l );
						showerror = FALSE; /* disable annoying errors if loading fails */
						bgctx.cnt = 0;
						bgctx.dircnt = 0;

						recurse( obj, filename, "~(#?.(info|txt|readme))", enterdir, NULL, addfile, &bgctx ); /* XXX: retcode.. ? */

						if( bgctx.cnt )
						{
							ULONG randnum;
							struct bgfilenode *bgfn, *nextbgfn;

							randnum = random_ulong() % bgctx.cnt;

							bgfn = FIRSTNODE( &bgctx.l );

							while( randnum-- )
							{
								bgfn = NEXTNODE( bgfn );
							}

							stccpy( file, bgfn->path, sizeof(file) );
							filename = file;

							/* XXX: we could try to Lock() it also? bah.. */

							ITERATELISTSAFE( bgfn, nextbgfn, &bgctx.l )
							{
								free( bgfn );
							}
						}
					}

					GetAttr( SA_CompositingLayers, (Object *) screen,&compositing );

					if( mode == BGRENDER_Tiled )
					{
						if ( ( dtp = datatypes_picture_create( filename, PICTAG_Screen, screen,
																		 PICTAG_VMem, compositing ? FALSE : TRUE,
																		 TAG_DONE ) ) )
						{
							#ifdef DEBUG
							if( !compositing )
								gfx_bitmap_check_vmem( picture_getattr( dtp, PICTURE_BITMAP ) );
							#endif

							methodstack_push_sync( obj, 3, MM_Application_AddBackground, type, dtp );
							retval = TRUE;
						}
					}
					else if( mode == BGRENDER_Scaled || mode == BGRENDER_Stretched )
					{
						ULONG txs, tys;

						txs = screen->Width;
						tys = screen->Height;// - (GetSkinInfoAttr(GetScreenDrawInfo(screen), SI_ScreenTitlebarHeight, TAG_DONE));

						if ( ( dtp = datatypes_picture_create( filename, PICTAG_ARGB32, TRUE, TAG_DONE ) ) )
						{
							/*
							 * BitMapScale() is stupid and uses filtering only if the destination is
							 * ARGB32, something our internal scaler does better. So unfortunately we have
							 * to allocate bitmaps 3 times.
							 */
							APTR bm;
							
							ULONG w = (ULONG) picture_getattr( dtp, PICTURE_WIDTH  );
							ULONG h = (ULONG) picture_getattr( dtp, PICTURE_HEIGHT );

							if( mode == BGRENDER_Scaled )
							{
								gfx_scale_calc_aspect_constraints( w, h, &txs, &tys );
							}

							if ( ( bm = gfx_bitmap_create( txs, tys, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE ) ) )
							{
								APTR tbm;

								if ( gfx_scale( picture_getattr( dtp, PICTURE_BITMAP), bm, txs, tys,
														SCALETAG_Nearest, TRUE,
														SCALETAG_Bilinear, TRUE,
														SCALETAG_Average, TRUE,
														TAG_DONE ) )
								{
									if ( ( tbm = gfx_bitmap_create(txs, tys, BITMAPDEPTH_Clone, BITMAPTAG_VMem, compositing ? FALSE : TRUE, BITMAPTAG_ScreenFriend, screen, TAG_DONE ) ) )
									{
										gfx_blit( bm, tbm, TAG_DONE );

										#ifdef DEBUG
										if (!compositing)
											gfx_bitmap_check_vmem( tbm );
										#endif
									
										picture_set_bitmap( dtp, tbm );
										methodstack_push_sync( obj, 3, MM_Application_AddBackground, type, dtp );
										retval = TRUE;
									}
								}
								gfx_bitmap_delete( bm );
							}
								
							if( !retval )
							{
								picture_delete( dtp );
							}
						}
					}
					else if( mode == BGRENDER_Zoomed )
					{
						ULONG txs, tys;
						ULONG swidth, sheight;

						/*
						 * In this mode we scale image so it will fill screen without borders and after that
						 * we crop additional space which was produced in a process.
						 */

						swidth  = txs = screen->Width;
						sheight = tys = screen->Height - (GetSkinInfoAttr( GetScreenDrawInfo(screen), SI_ScreenTitlebarHeight, TAG_DONE ) );

						if ( ( dtp = datatypes_picture_create(filename, PICTAG_ARGB32, TRUE, TAG_DONE ) ) )
						{
							/*
							 * BitMapScale() is stupid and uses filtering only if the destination is
							 * ARGB32, something our internal scaler does better. So unfortunately we have
							 * to allocate bitmaps 3 times.
							 */

							APTR bm;

							ULONG w = (ULONG) picture_getattr( dtp, PICTURE_WIDTH  );
							ULONG h = (ULONG) picture_getattr( dtp, PICTURE_HEIGHT );

							ULONG txs1 = txs , tys1 = 9999; /* give it 'infinite' space horizontaly */
							ULONG txs2 = 9999, tys2 = tys;  /* same verticaly */

							gfx_scale_calc_aspect_constraints( w, h, &txs1, &tys1 );
							gfx_scale_calc_aspect_constraints( w, h, &txs2, &tys2 );

							/* choose bigger image to make sure it fills the screen completely */

							if( txs1 > txs2 )
							{
								txs = txs1; tys = tys1;
							} else {
								txs = txs2; tys = tys2;
							}

							if ( ( bm = gfx_bitmap_create( txs, tys, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE ) ) )
							{
								APTR tbm;

								if( gfx_scale( picture_getattr( dtp, PICTURE_BITMAP ), bm, txs, tys,
										SCALETAG_Nearest, TRUE,
										SCALETAG_Bilinear, TRUE,
										SCALETAG_Average, TRUE,
										TAG_DONE ) )
								{
									ULONG memory = 0;
									Object *monitor = NULL;
									GetAttr(SA_MonitorObject, (Object *)screen, (IPTR)&monitor);
									GetAttr(MA_MemorySize, monitor, &memory);

									// On systems with lots of vmem, allow the bitmap to vmem in this case to allow for accelerated transitions 
									if (memory > MINIMUM_TRANSITIONS_VMEM)
										compositing = FALSE;
									
									if ( ( tbm = gfx_bitmap_create( swidth, sheight, BITMAPDEPTH_Clone, BITMAPTAG_VMem, compositing ? FALSE : TRUE,BITMAPTAG_ScreenFriend, screen, TAG_DONE ) ) )
									{
										LONG ox = 0;
										LONG oy = 0;

										/* when blitting, cut off additional borders (limit to screen dimensions) if neded */

										if (txs > swidth)
										{
											ox = ( txs - swidth  ) / 2;
										}
										if (tys > sheight)
										{
											oy = ( tys - sheight ) / 2;
										}

										gfx_blit( bm, tbm,
											BLITTAG_DstWidth, swidth,
											BLITTAG_DstHeight, sheight,
											BLITTAG_SrcX, ox,
											BLITTAG_SrcY, oy,
											TAG_DONE );

										#ifdef DEBUG
										if (!compositing)
											gfx_bitmap_check_vmem( tbm );
										#endif

										picture_set_bitmap( dtp, tbm );
										methodstack_push_sync( obj, 3, MM_Application_AddBackground, type, dtp );
										retval = TRUE;
									}
								}
								gfx_bitmap_delete( bm );
							}

							if( !retval )
							{
								picture_delete( dtp );
							}
						}
					}
					else if ( mode == BGRENDER_Centered )
					{
						if ( ( dtp = datatypes_picture_create( filename,
								PICTAG_Screen, screen,
								PICTAG_ARGB32, TRUE,
								TAG_DONE ) ) ) /* some datatypes return formats that disturb cgx so we convert */
						{
							methodstack_push_sync(obj, 3, MM_Application_AddBackground, type, dtp);
							retval = TRUE;
						}
					}
				}
				
				if (!retval)
				{
					if( showerror )
					{
						smartreq_info("Ambient Background", MV_Notification_Warning, "Background %s\ncouldn't be loaded by the datatypes subsystem.\nCheck that you have a datatype installed for the kind of image format you're\ntrying to use as a background.", filename);
					}
					return( FALSE );
				}

	return( TRUE );
}

