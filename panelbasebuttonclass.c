
#include "ambient.h"
#if USE_INTERNAL_PANELS
/* public */
#include <cybergraphx/cybergraphics.h>
#include <datatypes/pictureclass.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <clib/datatypes_protos.h>
#include <proto/timer.h>
#include <graphics/rpattr.h>
#include <libraries/asl.h>


/* private */
#include "ambient_cat.h"
#include "rexx.h"
#include "command.h"
#include "contextmenu.h"
#include "iconio.h"
#include "name.h"
#include "gfx_scale.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "gfx_pen.h"
#include "prefs.h"
#include "wbstart.h"
#include "threads.h"
#include "file_func.h"
#include "dragdrop.h"
#include "legacy.h"
#include "paneltags.h"
//#include "panellib.h"

#include "deficonpool.h"
#include "methodstack.h"
#include "deficon_getpath.h"
#include "viewwatcher_arrow_logo.h"

/************************************************************************/

#define STATUS_SELECTED 	0x00000001
#define STATUS_HIGHLIGHT 	0x00000002
#define STATUS_DROPTARGET 	0x00000004

#define PANEL_MESSAGE_COUNT 10 

/************************************************************************/

struct Data {
	APTR cmenu;
	STRPTR imagepath;
	STRPTR uri;
	ULONG directory;
	ULONG oldwidth;
	ULONG oldheight;
	BOOL init;
	ULONG drawmode;
	/* image highlighting */
	ULONG status,oldstatus;
	ULONG current_effect, desired_effect;
	APTR tempbm,selbm;
	APTR bm,alt_bm;
	APTR sclbm,alt_sclbm;
	ULONG scalemode,altbm_rwidth,altbm_rheight; 
//	ULONG altbm_pos;
	APTR app_object;
	ULONG callbackmode;
	BOOL callback_enabled;

	//APTR sclbm;
	//ULONG scalemode; 
	ULONG altbm_pos,altbm_rsize;
	ULONG win_x,win_y,win_xs,win_ys;
	ULONG viewmode;
	ULONG sortmode;

	LONG icontype;
//	struct PanelMessage *messages;
//	ULONG message_ct;
	
	
	APTR messenger;
	struct TimeVal lastclick;
};

/************************************************************************/

static void load_icon(APTR obj UNUSED, struct Data *data, APTR *bm, APTR *selbm)
{
	APTR o = NULL;
	APTR bm2 = NULL, bm3 = NULL;

	if ( (o = (APTR)methodstack_push_sync(app, 3, MM_Application_CreateIcon, TRUE, MV_ViewID_Panel)))
	{
		if (deficonpool_apply_default_icon(o, data->icontype, NULL, NULL))
		{
			APTR bm1;

			bm1 = (APTR)getv(o, MA_Icon_ImageNormal);
			if (bm1 != NULL)
			{
				bm2 = gfx_bitmap_create(gfx_bitmap_width(bm1), gfx_bitmap_height(bm1), gfx_bitmap_depth(bm1),
											BITMAPTAG_Clear, TRUE, BITMAPTAG_Friend, bm1, TAG_DONE);

				bm3 = gfx_bitmap_create(gfx_bitmap_width(bm1), gfx_bitmap_height(bm1), gfx_bitmap_depth(bm1),
											BITMAPTAG_Clear, TRUE, BITMAPTAG_Friend, bm1, TAG_DONE);

				if (bm2 != NULL && bm3 != NULL)
				{
					APTR scaled_arrow, arrow;
					struct RastPort rp;

                    gfx_blit(bm1, bm2, TAG_DONE);
					gfx_blit(bm1, bm3, TAG_DONE);

					arrow = gfx_bitmap_create(VIEWWATCHER_ARROW_WIDTH, VIEWWATCHER_ARROW_HEIGHT, 32,
											BITMAPTAG_Clear, TRUE, BITMAPTAG_Friend, bm1, TAG_DONE);

					gfx_blit(viewwatcher_arrow, arrow, BLITTAG_SrcType, BLITVAL_SrcType_Array,
													 BLITTAG_SrcFormat, BLITVAL_SrcFormat_ARGB,
													 BLITTAG_DstWidth, VIEWWATCHER_ARROW_WIDTH,
													 BLITTAG_DstHeight, VIEWWATCHER_ARROW_HEIGHT,
													 BLITTAG_Alpha, 0xffffffff,
													 TAG_DONE);

					scaled_arrow = gfx_bitmap_create((ULONG)gfx_bitmap_width(bm1)/3, (ULONG)gfx_bitmap_height(bm1)/3, gfx_bitmap_depth(bm1),
											BITMAPTAG_Clear, TRUE, BITMAPTAG_Friend, bm1, TAG_DONE);

					gfx_scale(arrow, scaled_arrow, gfx_bitmap_width(scaled_arrow), gfx_bitmap_height(scaled_arrow),
						SCALETAG_Scale2x, TRUE,
						SCALETAG_Bilinear, TRUE,
						SCALETAG_Average, TRUE,
						SCALETAG_AspectX, NULL,
						SCALETAG_AspectY, NULL,
						TAG_DONE);


					gfx_blit(scaled_arrow, bm2,  BLITTAG_DstX, gfx_bitmap_width(bm2) - gfx_bitmap_width(scaled_arrow),
												 BLITTAG_Modulo, gfx_bitmap_width(scaled_arrow) * 32 / 8,
												 BLITTAG_DstWidth, gfx_bitmap_width(scaled_arrow),
												 BLITTAG_DstHeight, gfx_bitmap_height(scaled_arrow),
												 BLITTAG_Alpha, 0xffffffff,
												 TAG_DONE);

					InitRastPort(&rp);
					rp.BitMap = gfx_bitmap_bm(scaled_arrow);
					ProcessPixelArray(&rp, 0, 0, gfx_bitmap_width(scaled_arrow), gfx_bitmap_height(scaled_arrow), POP_BRIGHTEN, 0x20, NULL);

					gfx_blit(scaled_arrow, bm3,  BLITTAG_DstX, gfx_bitmap_width(bm3) - gfx_bitmap_width(scaled_arrow),
												 BLITTAG_Modulo, gfx_bitmap_width(scaled_arrow) * 32 / 8,
												 BLITTAG_DstWidth, gfx_bitmap_width(scaled_arrow),
												 BLITTAG_DstHeight, gfx_bitmap_height(scaled_arrow),
												 BLITTAG_Alpha, 0xffffffff,
												 TAG_DONE);

					gfx_bitmap_delete(arrow);
					gfx_bitmap_delete(scaled_arrow);
				}
			}
        }
		methodstack_push(o, 1, OM_RELEASE);
	}

	*bm = bm2;
	*selbm = bm3;
}

/************************************************************************/

static void doset( APTR obj, struct Data *data, struct TagItem *tags )
{
	FORTAG( tags )
	{
		case MA_Panel_URI:
			if( data->uri )
			{
				name_delete( data->uri );
			}
			/* Kronos, please check this. It will try to transform any
			** path containing a ":", so it may trigger something else,
			** if this is not a file name. In worst case it will pop up
			** a volume requester, but who knows.
			*/
			data->uri = name_build_sysify( (STRPTR) tag->ti_Data );
			data->directory = isdir( data->uri );
			break;

		case MA_Panel_Highlighted:
			if( tag->ti_Data )
			{
				data->status = data->status | STATUS_HIGHLIGHT;
			} else {
				data->status = data->status & ~STATUS_HIGHLIGHT;
			}
			
			MUI_Redraw( obj, MADF_DRAWOBJECT );
			break;

		case MUIA_Selected:
			if( tag->ti_Data)
			{
				data->status = data->status | STATUS_SELECTED;
			} else {
				data->status = data->status & ~STATUS_SELECTED;
			}
			break;
		case MA_Panel_Imagepath:
			if( data->imagepath )
			{
				name_delete( data->imagepath );
			}
			data->imagepath = name_build_sysify( (STRPTR) tag->ti_Data );
			if( strlen( data->imagepath ) )
			{
				icon_read( data->imagepath, obj,
								ICONTAG_Position, FALSE,
								ICONTAG_Deficon, TRUE,
								TAG_DONE );
				} else {
				if( data->icontype )
				{
					load_icon( obj, data, &data->bm, &data->selbm );
				}
			}
			MUI_Redraw( obj, MADF_DRAWALL );
			break;
		case MA_Icon_IsDefault:
			switch(data->icontype)
			{
				case MV_Icon_Type_Disk:
					set( obj, MA_Panel_Imagepath, "env:sys/def_disk.info" );
					break;
				case MV_Icon_Type_Drawer:
					set( obj, MA_Panel_Imagepath, "env:sys/def_drawer.info" );
					break;
				default:			
					set( obj, MA_Panel_Imagepath, "env:sys/def_tool.info" );
			}
			break;
		case MA_Icon_Type:
			data->icontype = tag->ti_Data;
			break;
/* old panellib functionality disabled*/
#if 0
		case MA_Panelbutton_ExtraBitmap:
			data->alt_bm = (APTR)tag->ti_Data;
			data->drawmode =  PanelObject_DrawMode_Replace;
			if( !data->alt_bm ) data->drawmode = PanelObject_DrawMode_None;
			MUI_Redraw( obj, MADF_DRAWOBJECT );
			break;
		case PanelObject_DrawMode:
			data->drawmode = tag->ti_Data;
			MUI_Redraw( obj, MADF_DRAWOBJECT );
			break;
		case PanelObject_ScaleMode:
			data->scalemode = tag->ti_Data;
			MUI_Redraw( obj, MADF_DRAWOBJECT );
			break;
		case PanelObject_RelSize:
			data->altbm_rsize = tag->ti_Data;
			MUI_Redraw( obj, MADF_DRAWOBJECT );
			break;
		case PanelObject_Position:
			data->altbm_pos = tag->ti_Data;
			MUI_Redraw( obj, MADF_DRAWOBJECT );
			break;
		case MA_Panel_Messenger:
			data->messenger = (APTR) tag->ti_Data;	
			break;
#endif
		case MA_Icon_WindowTop:
			data->win_y = tag->ti_Data;
			break;
/* old panellib functionality disabled*/
#if 0
		case PanelObject_CallbackMode:
			data->callbackmode = tag->ti_Data;
			if ((! data->callback_enabled ) && ( data->callbackmode))
			{
		
				data->callback_enabled = TRUE;
			}
#endif
		case MA_Icon_WindowLeft:
			data->win_x = tag->ti_Data;

			break;
		case MA_Icon_WindowWidth:
			data->win_xs = tag->ti_Data;
			break;
		case MA_Icon_WindowHeight:
			data->win_ys = tag->ti_Data;
			break;
		case MA_Icon_ViewMode:
			data->viewmode = tag->ti_Data;
			break;
		case MA_Icon_SortMode:
			data->sortmode = tag->ti_Data;
			break;
	
	}
	NEXTTAG
}

/************************************************************************/
DEFNEW
{
	struct Data *data;
	struct TagItem *imagetag;
	obj= (Object*) DoSuperNew( cl, obj,
			MUIA_ContextMenu, 0, 		
			TAG_MORE, INITTAGS
			);
	data = (struct Data*) INST_DATA( cl, obj );
	data->oldwidth = 0;
	data->oldheight = 0;
	data->sclbm = 0;
	data->alt_bm = 0;
    data->tempbm = NULL;
	data->init = TRUE;
	data->uri = 0;
	data->icontype = 0;
/*	data->drawmode =  PanelObject_DrawMode_None;
	data->scalemode =  PanelObject_ScaleMode_Fit;
	data->altbm_pos = PanelObject_Position_Mid;
*/
	data->altbm_rwidth = 100;
	data->altbm_rheight = 100; 
	data->app_object = NULL;
	data->callback_enabled = FALSE;

	data->altbm_rsize = 100;
	data->messenger = NULL;
	data->viewmode = MV_Icon_ViewMode_None;

	SetAttrs( obj, MUIA_InputMode, MUIV_InputMode_RelVerify, MUIA_ShowSelState, FALSE, TAG_DONE );
	DoMethod( obj, MUIM_Notify   , MUIA_Pressed, FALSE, obj, 1, MM_Panelbutton_Launch );

	if (TimerBase->lib_Version >= 52)
		GetUTCSysTime( &data->lastclick );
	else
		GetSysTime( &data->lastclick );
	if( ( !( imagetag = FindTagItem( MA_Panel_Imagepath, msg->ops_AttrList ) ) || ( 2 > strlen( (char*) imagetag->ti_Data ) ) ) )
	{
		if( ( data->icontype = GetTagData( MA_PanelPopup_IconType, MV_Icon_Type_View, INITTAGS ) ) )
		{
			load_icon( obj, data, &data->bm, &data->selbm );
		} else {
			ULONG defaultImage = 0;
			if( get( obj, MA_Panelbutton_DefaultImagePath, &defaultImage ) )
			{
				set( obj, MA_Panel_Imagepath, defaultImage );
			} else {
				set( obj, MA_Panel_Imagepath, "env:sys/def_tool.info" );
			}
		}
	}
	
	doset( obj, data, INITTAGS );
	data->init = FALSE;
	data->status = 0;
	return( (ULONG) obj );
}

/************************************************************************/

DEFDISP 
{
	GETDATA;
	if(data->messenger)DoMethod(data->messenger,MM_Panelmessenger_PanelObjectDisposed);
	gfx_bitmap_delete( data->sclbm );

	gfx_bitmap_delete( data->bm );

	if( data->uri )
	{
		name_delete( data->uri );
	}

	

	if( data->imagepath )
	{
		name_delete( data->imagepath );
	}

	gfx_bitmap_delete( data->tempbm );

	return( DOSUPER );
}

/************************************************************************/

DEFMMETHOD(AskMinMax)
{
	ULONG size = getv( _parent(obj), MA_Panelgroup_Size );
	ULONG w,h;
	APTR parent;
    GETDATA;
	DOSUPER;

	ASSERT(size);

	GetAttr( MUIA_Parent, obj, (ULONG*) &parent );
	if( ( data->bm ) && (parent) )
	{
		ULONG horiz;
		GetAttr( MA_Panelgroup_Horiz, parent, &horiz );
		w = gfx_bitmap_width( data->bm );
		h = gfx_bitmap_height( data->bm );
		if( ( w == h ) || ( getprefslong( DSI_PANEL_LAYOUT_GRID ) ) )
		{
			msg->MinMaxInfo->MinWidth = size;
			msg->MinMaxInfo->MinHeight = size;
			msg->MinMaxInfo->MaxWidth = size;
			msg->MinMaxInfo->MaxHeight = size;
		} else {
			if( horiz )
			{
				FLOAT fac, wf;
				fac = size * 1000 / h;
				wf = ( fac * w ) / 1000 + 0.9;
				msg->MinMaxInfo->MinWidth  = wf;
				msg->MinMaxInfo->MinHeight = size;
				msg->MinMaxInfo->MaxWidth  = wf;
				msg->MinMaxInfo->MaxHeight = size;
			} else {
				FLOAT fac, hf;
				fac = size * 1000 / w;
				hf = ( fac * h ) / 1000 + 0.9;
				msg->MinMaxInfo->MinWidth  = size;
				msg->MinMaxInfo->MinHeight = hf;
				msg->MinMaxInfo->MaxWidth  = size;
				msg->MinMaxInfo->MaxHeight = hf;
			}
		}
	}
	/* XXX: beware.. we don't use += here because MUI adds too much frames atm.. experiment later */
//	  else
	{
		msg->MinMaxInfo->MinWidth  = size;
		msg->MinMaxInfo->MinHeight = size;
		msg->MinMaxInfo->MaxWidth  = size;
		msg->MinMaxInfo->MaxHeight = size;
	}
	return (0);
}

/************************************************************************/

DEFGET
{
	GETDATA;
	ULONG result = TRUE;

	switch( msg->opg_AttrID )
	{
		case MA_Panelbutton_DefaultImagePath:
			*msg->opg_Storage = 0;
			break;
		case MA_Panel_Type:
			*msg->opg_Storage = MV_Panel_Type_Button;
			break;
		case MA_Panel_Imagepath:
			*msg->opg_Storage = (ULONG) data->imagepath;
			break;
		case MA_Panel_URI:
			*msg->opg_Storage = (ULONG) data->uri;
			break;
		case MA_Panel_Extern_DisplayName:
			*msg->opg_Storage = (ULONG) "Button";
			break;
		case MA_Panel_Extern_Revision:
		case MA_Panel_Extern_Version:
			*msg->opg_Storage = 1;
			break;
		case MA_Panel_Extern_Author:
			*msg->opg_Storage = (ULONG) "Ambient Open Source Team";
			break;
		case MA_Panel_Extern_Description:
			*msg->opg_Storage = (ULONG) "None";
			break;
		case MA_PanelZipLock:
			*msg->opg_Storage = 0;
			break;
		case MA_Icon_WindowTop:
			*msg->opg_Storage = data->win_y;
			break;
		case MA_Icon_WindowLeft:
			*msg->opg_Storage = data->win_x;
			break;
		case MA_Icon_WindowWidth:
			*msg->opg_Storage = data->win_xs;
			break;
		case MA_Icon_WindowHeight:
			*msg->opg_Storage = data->win_ys;
			break;
		case MA_Icon_ViewMode:
			*msg->opg_Storage = data->viewmode;
			break;
		default:
			result = DOSUPER;
			break;
	}
	return( result );
}

/************************************************************************/

DEFSET
{
	GETDATA;
	
	doset( obj, data, INITTAGS );
	if(data->messenger)	DoMethod(data->messenger,MM_Panelbutton_Callback,INITTAGS );
	return( DOSUPER );
}

/************************************************************************/

DEFMMETHOD(ContextMenuBuild)
{
	GETDATA;

	if( data->cmenu )
	{
		MUI_DisposeObject( data->cmenu );
	}

	if ( ( data->cmenu = contextmenu_build( CM_PANELBUTTON, 0) ) ) /* XXX: we should honor locked mode perhaps */
	{
		return( (ULONG) data->cmenu );
	}
	errormsg( ERR_NOMEM );

	return( 0 );
}

/************************************************************************/

DEFMMETHOD(ContextMenuAdd)
{
	GETDATA;
	ULONG flags = 0;

	if (!getv( _parent( obj ), MA_Panelgroup_Locked ) )
	{
		flags |= AS_PANEL_UNLOCKED;
	}

	data->cmenu	= contextmenu_build_simple( CM_PANELBUTTON, flags );

	if( data->cmenu )
	{
		DoMethod( msg->menustrip, OM_ADDMEMBER, data->cmenu );
	}

	return( DOSUPER );
}

/************************************************************************/

DEFMMETHOD(ContextMenuChoice)
{
	GETDATA;
	struct command_menu *cm;
	ULONG rc = 0;

	if ( data->cmenu && DoMethod( data->cmenu, MUIM_FindObject, msg->item ) )
	{
		if ( ( (cm = (struct command_menu *) getv( msg->item, MA_Menuitem_Command ) ) ) )
		{
			if( cm->name && *cm->name )
			{
				execute_command( _parent( obj ), cm->type, cm->args, NULL );
			}
		}
	} else {
		rc = DOSUPER;
	}


	data->cmenu = NULL;

	return( rc );
}

/************************************************************************/

DEFSMETHOD(Icon_AddBitMap)
{
	GETDATA;
	data->oldwidth = 0;
	data->oldheight = 0;

	gfx_bitmap_delete( data->tempbm );
	data->tempbm = NULL;


	/*
	 * We only keep one bitmap.
	 */
	switch( msg->type )
	{
		case MV_Icon_BitMap_Standard:
			switch( msg->state )
			{
				case MV_Icon_BitMap_Normal:
					gfx_bitmap_delete( data->bm );
					data->bm = msg->bm;
					break;

				case MV_Icon_BitMap_Selected:
					gfx_bitmap_delete( msg->bm );
					break;
			}
			break;

		case MV_Icon_BitMap_Newicon:
			switch( msg->state )
			{
				case MV_Icon_BitMap_Normal:
					gfx_bitmap_delete( data->bm );
					data->bm = msg->bm;
					break;

				case MV_Icon_BitMap_Selected:
					gfx_bitmap_delete( msg->bm );
					break;
			}
			break;

		case MV_Icon_BitMap_Glowicon:
			switch( msg->state )
			{
				case MV_Icon_BitMap_Normal:
					gfx_bitmap_delete( data->bm );
					data->bm = msg->bm;
					break;

				case MV_Icon_BitMap_Selected:
					gfx_bitmap_delete( msg->bm );
					break;
			}
			break;

		#if USE_PNGICONS
		case MV_Icon_BitMap_PNGicon:
			switch( msg->state )
			{
				case MV_Icon_BitMap_Normal:
					gfx_bitmap_delete( data->bm );
					data->bm = msg->bm;
					break;

				case MV_Icon_BitMap_Selected:
					gfx_bitmap_delete( msg->bm );
					break;
			}
			break;
		#endif
		
		#if USE_SVGICONS
		case MV_Icon_BitMap_SVGicon:
			switch( msg->state )
			{
				case MV_Icon_BitMap_Normal:
					gfx_bitmap_delete( data->bm );
					data->bm = msg->bm;
					break;

				/*SVG icons are one-state for the time being*/
				/*case MV_Icon_BitMap_Selected:
					gfx_bitmap_delete( msg->bm );
					break;*/
			}
			break;
		#endif

		#if USE_DTICONS
		case MV_Icon_BitMap_DTicon:
			data->bm = msg->bm;
			break;
		#endif

		#ifdef DEBUG
		default:
			PDB(("out of bound bitmap: val: %ld\n", msg->type));
			break;
		#endif
	}
	return( 0 );
}
    

/************************************************************************/

DEFSMETHOD(Panelbutton_AddIcon)
{
	GETDATA;
	STRPTR path;

	ASSERT( msg->obj );
	if ( get( msg->obj, MA_Icon_PathInfo, &path ) )
	{
		if( data->imagepath )
		{
			name_delete( data->imagepath );
		}

		/*
		 * Try to put the executable's filename in
		 * there.
		 * XXX: handle more URIs later on
		 */
		if( data->uri )
		{
			name_delete( data->uri );
			data->uri = NULL;
		}

		data->uri = name_build( (STRPTR) getv( msg->obj, MA_Icon_Path ) );
		data->directory = isdir( data->uri );
		if ( ( data->imagepath = name_build( path ) ) )
		{
			set( obj, MA_Panel_Imagepath, path );
		}
		/* XXX */
	}
	return (0);
}

/************************************************************************/

static VOID clear_bitmap(APTR bm)
{
	struct RastPort rp;
	ASSERT( bm );

	InitRastPort( &rp );
	rp.BitMap = gfx_bitmap_bm( bm );
	FillPixelArray(&rp,0,0,gfx_bitmap_width( bm ),gfx_bitmap_height( bm ),0x00000000);
}

/************************************************************************/

static VOID process_image(APTR obj, struct IClass *cl, APTR dstbm)
{
	struct RastPort rp;
	ULONG color     = 0x00ff0000;
	ULONG colorfade = 0;
	ULONG brighten  = 0;
	ULONG darken    = 0;

	GETDATA;

	ASSERT( dstbm );

	InitRastPort( &rp );
	rp.BitMap = gfx_bitmap_bm( dstbm );

	if( data->status & STATUS_DROPTARGET )
	{
		brighten = getprefslong( DSI_PANEL_DRAGDROP_BRIGHTEN );
		darken   = getprefslong( DSI_PANEL_DRAGDROP_DARKEN );
		color     = gfx_get_penspec_value( muiRenderInfo( obj ), getprefs( DSI_PANEL_DRAGDROP_TINT ) );
		colorfade = gfx_get_penspec_value( muiRenderInfo( obj ), getprefs( DSI_PANEL_DRAGDROP_TINTFADE ) );
	}
	else if( data->status & STATUS_SELECTED )
	{
		brighten = getprefslong( DSI_PANEL_SELECTED_BRIGHTEN );
		darken   = getprefslong( DSI_PANEL_SELECTED_DARKEN );
		color     = gfx_get_penspec_value( muiRenderInfo( obj ), getprefs( DSI_PANEL_SELECTED_TINT ) );
		colorfade = gfx_get_penspec_value( muiRenderInfo( obj ), getprefs( DSI_PANEL_SELECTED_TINTFADE ) );
	}
	else if( data->status & STATUS_HIGHLIGHT )
	{
		brighten = getprefslong( DSI_PANEL_HIGHLIGHT_BRIGHTEN );
		darken   = getprefslong( DSI_PANEL_HIGHLIGHT_DARKEN );
		color     = gfx_get_penspec_value( muiRenderInfo( obj ), getprefs( DSI_PANEL_HIGHLIGHT_TINT ) );
		colorfade = gfx_get_penspec_value( muiRenderInfo( obj ), getprefs( DSI_PANEL_HIGHLIGHT_TINTFADE ) );
	}
	switch ( data->desired_effect )
	{
		case PANEL_EFFECT_CLONE_ICONVIEW:   /*this one needs special care */
		if( data->status & STATUS_DROPTARGET )
		{
			ProcessPixelArray( &rp, 0, 0, gfx_bitmap_width( dstbm ), gfx_bitmap_height( dstbm ), POP_TINT, gfx_get_penspec_value( muiRenderInfo( obj ), &_conf( dragdrop_tintval ) ), NULL );
		}
		else if( data->status & STATUS_SELECTED )
		{
			ProcessPixelArray( &rp, 0, 0, gfx_bitmap_width( dstbm ), gfx_bitmap_height( dstbm ), POP_TINT, gfx_get_penspec_value( muiRenderInfo( obj ), &_conf( icon_tintval ) ), NULL );
		}
		else if( data->status & STATUS_HIGHLIGHT )
		{
			ProcessPixelArray( &rp, 0, 0, gfx_bitmap_width( dstbm ), gfx_bitmap_height( dstbm ),  POP_BRIGHTEN, 0x20, NULL );
		}
	
		break;
		case PANEL_EFFECT_TINT:
			ProcessPixelArray( &rp, 0, 0, gfx_bitmap_width( dstbm ), gfx_bitmap_height( dstbm ), POP_TINT, color, NULL );
		break;
	
		case PANEL_EFFECT_BLUR:
			ProcessPixelArray( &rp, 0, 0, gfx_bitmap_width( dstbm ), gfx_bitmap_height( dstbm ), POP_BLUR, 0, NULL );
		break;

		case PANEL_EFFECT_BRIGHTEN:
			ProcessPixelArray( &rp, 0, 0, gfx_bitmap_width( dstbm ), gfx_bitmap_height( dstbm ), POP_BRIGHTEN, brighten, NULL );
		break;

		case PANEL_EFFECT_DARKEN:
			ProcessPixelArray( &rp, 0, 0, gfx_bitmap_width( dstbm ), gfx_bitmap_height( dstbm ), POP_DARKEN, darken, NULL );
		break;

		case PANEL_EFFECT_GREY:
			ProcessPixelArray( &rp, 0, 0, gfx_bitmap_width( dstbm ), gfx_bitmap_height( dstbm ), POP_COLOR2GREY, 0, NULL );
		break;

		case PANEL_EFFECT_NEGATIVE:
			ProcessPixelArray( &rp, 0, 0, gfx_bitmap_width( dstbm ), gfx_bitmap_height( dstbm ), POP_NEGATIVE, 0, NULL );
		break;

		case PANEL_EFFECT_NEGFADE:
			ProcessPixelArray( &rp, 0, 0, gfx_bitmap_width( dstbm ), gfx_bitmap_height( dstbm ), POP_NEGFADE, 0, NULL );
		break;

		case PANEL_EFFECT_TINTFADE:
			ProcessPixelArray( &rp, 0, 0, gfx_bitmap_width( dstbm ), gfx_bitmap_height( dstbm ), POP_TINTFADE, colorfade, NULL );
		break;
	}
}

/************************************************************************/

DEFMMETHOD(Draw)
{
	GETDATA;

	DOSUPER;
	BOOL sclbm_used = FALSE;
	if( data->bm && msg->flags & ( MADF_DRAWOBJECT | MADF_DRAWUPDATE ) )
	{
		ULONG mleft, mtop, mwidth, mheight;
		APTR tbm;
		ULONG scalewidth;
		ULONG scaleheight;
		mleft   = _mleft( obj );
		mtop    = _mtop( obj );
		scalewidth = mwidth  = _mwidth( obj );
		scaleheight = mheight = _mheight( obj );
		
		if( data->status & STATUS_DROPTARGET )
		{
			data->desired_effect =  getprefslong( DSI_PANEL_DRAGDROP_EFFECT );
		}
		else if( data->status & STATUS_SELECTED )
		{
			data->desired_effect =  getprefslong( DSI_PANEL_SELECTED_EFFECT ); 
		}
		else if( data->status & STATUS_HIGHLIGHT )
		{
			data->desired_effect =  getprefslong( DSI_PANEL_HIGHLIGHT_EFFECT );
		}
		else
		{
			data->desired_effect = PANEL_EFFECT_NONE;
		}
		if(    ( data->oldwidth       == mwidth              ) && ( data->oldheight == mheight ) && ( data->status && data->oldstatus )
			&& ( data->current_effect == data->desired_effect) && (!( msg->flags & ( MADF_DRAWOBJECT )) ) )
		return ( 0 ); /*nothing changed, nothing to be drawn */
		
		if( ( data->oldwidth != mwidth ) && ( data->oldheight != mheight ) ) /* size changed throw awaw bitmaps */
		{
			if( data->tempbm ) gfx_bitmap_delete( data->tempbm ); 
			data->tempbm = NULL;
			if ( data->sclbm)	gfx_bitmap_delete( data->sclbm ); 
			data->sclbm = NULL;
		}
		else
		{
			if( data->tempbm ) clear_bitmap(data->tempbm);
			if( data->sclbm ) clear_bitmap(data->sclbm);
		}
        if ( !data->sclbm) /* (re)allocate bitmaps */
		{
			data->sclbm = gfx_bitmap_create( mwidth, mheight, 32, BITMAPTAG_Clear, TRUE, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE );
		}
		if( !data->tempbm )   
		{
		
			data->tempbm = gfx_bitmap_create( mwidth,mheight, 32, BITMAPTAG_Format,BITMAPTAG_Clear, TRUE, BITMAPVAL_Format_ARGB32, TAG_DONE );
			if( !data->tempbm )
			{
				return( 0 );
			}
		}
		
		DoMethod( _parent(obj), MM_Panelgroup_RefreshRect, mleft, mtop, mwidth, mheight, _rp( obj ) );
		
		//if( data->drawmode !=  PanelObject_DrawMode_Replace )
		{
			/* default graphics aka icon needs to be drawn */
			
			tbm = data->bm;
			gfx_scale_calc_aspect_constraints(
										gfx_bitmap_width( data->bm ),
										gfx_bitmap_height( data->bm ),
										&scalewidth,
										&scaleheight
										);
			
			if(  data->sclbm )
			{
				gfx_scale( data->bm, data->sclbm, scalewidth, scaleheight,
							SCALETAG_Scale2x, TRUE,
							SCALETAG_Bilinear, TRUE,
							SCALETAG_Average, TRUE,
							SCALETAG_AspectX, NULL,
							SCALETAG_AspectY, NULL,
							TAG_DONE);
				tbm = data->sclbm;
				sclbm_used = TRUE;
			}
				
			if( ( data->desired_effect != PANEL_EFFECT_NONE ) && ( data->desired_effect != PANEL_EFFECT_LASSO ) )
			{
				if( (data->desired_effect != data->current_effect ) || ( data->status != data->oldstatus ) || ( msg->flags & ( MADF_DRAWOBJECT ) ) )
				{
					process_image( obj, cl, tbm );
				}
			}
			gfx_blit( tbm, _rp( obj ),
					BLITTAG_DstType, BLITVAL_DstType_RastPort,
					BLITTAG_DstX   ,  mleft  + (mwidth - scalewidth) / 2,
					BLITTAG_DstY   ,  mtop   + (mheight - scaleheight) / 2 ,
					BLITTAG_Alpha  , 0xffffffff,
					TAG_DONE );
	
		}
/* old panellib functionality disabled*/
#if 0 
		if( ( data->alt_bm )&& ( data->drawmode !=  PanelObject_DrawMode_None) )
		{
			ULONG x_pos,y_pos;
			scalewidth = mwidth * data->altbm_rsize / 100;
			scaleheight = mheight * data->altbm_rsize / 100;
			if( sclbm_used )
			{
				clear_bitmap(data->sclbm);
				
			}
			gfx_scale_calc_aspect_constraints(
											gfx_bitmap_width( data->alt_bm ),
											gfx_bitmap_height( data->alt_bm ),
											&scalewidth,
											&scaleheight
											);
			gfx_scale( data->alt_bm, data->sclbm, scalewidth, scaleheight,
								SCALETAG_Scale2x, TRUE,
								SCALETAG_Bilinear, TRUE,
								SCALETAG_Average, TRUE,
								SCALETAG_AspectX, NULL,
								SCALETAG_AspectY, NULL,
								TAG_DONE);
				tbm = data->sclbm;
				
				if(data->altbm_pos & PanelObject_Position_Left) x_pos = mleft;
				else if(data->altbm_pos & PanelObject_Position_Right) x_pos = mleft   + (mwidth - scalewidth);
				else x_pos = mleft   + (mwidth - scalewidth) / 2;
				
				if(data->altbm_pos & PanelObject_Position_Top) y_pos = mtop;
				else if(data->altbm_pos & PanelObject_Position_Bottom) 	y_pos = mtop   + (mheight - scaleheight);
				else 	y_pos = mtop   + (mheight - scaleheight) / 2;
		
				if( data->drawmode ==  PanelObject_DrawMode_Replace) 				
				{				
					if( ( data->desired_effect != PANEL_EFFECT_NONE ) && ( data->desired_effect != PANEL_EFFECT_LASSO ) )
					{
						if( (data->desired_effect != data->current_effect ) || ( data->status != data->oldstatus ) || ( msg->flags & ( MADF_DRAWOBJECT ) ) )
						{
							process_image( obj, cl, tbm );
						}
					}
					
				}
				gfx_blit( tbm, _rp( obj ),
					BLITTAG_DstType, BLITVAL_DstType_RastPort,
					BLITTAG_DstX   , x_pos,
					BLITTAG_DstY   , y_pos ,
					BLITTAG_Alpha  , 0xffffffff,
					TAG_DONE );
				
		}
#endif
		data->oldwidth = mwidth;
		data->oldheight = mheight;
		if( data->desired_effect == PANEL_EFFECT_LASSO )
		{
			struct tPoint box[4];
			WORD oldpattern;
			box[0].x = box[3].x	= _mleft( obj ) + 1;
			box[0].y = box[1].y	= _mtop( obj ) + 1;
			box[1].x = box[2].x = _mright( obj ) - 1;
			box[2].y = box[3].y = _mbottom( obj ) - 1;
			SetRPAttrs( _rp( obj ), RPTAG_PenMode, FALSE, RPTAG_FgColor, 0xFFffffff, RPTAG_BgColor, 0xFF000000, TAG_DONE );
			oldpattern = _rp( obj )->LinePtrn;
			SetDrMd( _rp( obj ), JAM2 );
			_rp( obj )->LinePtrn = 0x3333;
			Move( _rp( obj ), box[3].x, box[3].y );
			PolyDraw( _rp( obj ), 4, (WORD*) box );
			_rp( obj )->LinePtrn = oldpattern;
		}
		data->current_effect = data->desired_effect;
		data->oldstatus      = data->status;
	}
	return( 0 );
}

/************************************************************************/

DEFMMETHOD(DragQuery)
{
	if( getv( _parent( obj ), MA_Panelgroup_Locked ) )
	{
		return( MUIV_DragQuery_Accept );
	}
	return( MUIV_DragQuery_Refuse );
}

/************************************************************************/

DEFTMETHOD(Panelbutton_Launch)
{
	GETDATA;
	ULONG doubleclick;
	struct TimeVal currenttime;

	if (TimerBase->lib_Version >= 52)
		GetUTCSysTime( &currenttime );
	else
		GetSysTime( &currenttime );
	doubleclick = DoubleClick( data->lastclick.tv_secs, data->lastclick.tv_micro, currenttime.tv_secs, currenttime.tv_micro );
	data->lastclick = currenttime;

	return( doubleclick );
}

/************************************************************************/

DEFMMETHOD(DragFinish)
{
	GETDATA;
	ULONG rc = DOSUPER;

	data->status = data->status & ~STATUS_DROPTARGET;
	MUI_Redraw( obj, MADF_DRAWUPDATE );

	return( rc );
}

/************************************************************************/

DEFMMETHOD(DragBegin)
{
	GETDATA;
	ULONG rc = DOSUPER;

	data->status = data->status | STATUS_DROPTARGET;
	MUI_Redraw( obj, MADF_DRAWUPDATE );
	return( rc );
}

/************************************************************************/

#define BUTTONPATTERN_SIZEOF 10
UBYTE buttonpattern[ BUTTONPATTERN_SIZEOF ] = "#?.info";

DEFTMETHOD(Panel_Settings_Group)
{
	GETDATA;
	APTR group;
	APTR iconasl, iconasl_str, icon_modus;
	STRPTR def_path;
	static STRPTR cyc_iconmodes[ 3 ];
	cyc_iconmodes[ 0 ] = GSI(MSG_PANELBASEBUTTON_ICON_DEFAULT);
	cyc_iconmodes[ 1 ] = GSI(MSG_PANELBASEBUTTON_ICON_CUSTOM);
	cyc_iconmodes[ 2 ] = 0;
	get(obj,MA_Panelbutton_DefaultImagePath,&def_path);
		
	if( ( group = HGroup,
		Child, NSLabel1(MSG_PANELBASEBUTTON_ICON),
		Child, icon_modus = CycleObject,
					MUIA_Cycle_Entries, cyc_iconmodes,
					End,
		Child, iconasl = PopaslObject, MUIA_ShowMe, FALSE,
											ASLFR_InitialPattern, &buttonpattern,
											MUIA_Popstring_String, iconasl_str = KeyString( data->imagepath, 60, "n" ),
											MUIA_Popstring_Button, PopButton(MUII_PopUp),
											End,
		
	End ) ) {

		DoMethod( iconasl_str, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj    , 3, MUIM_Set, MA_Panel_Imagepath, MUIV_TriggerValue );
		DoMethod( icon_modus , MUIM_Notify, MUIA_Cycle_Active   , 0             , iconasl, 3, MUIM_Set, MUIA_ShowMe, FALSE );
		DoMethod( icon_modus , MUIM_Notify, MUIA_Cycle_Active   , 1             , iconasl, 3, MUIM_Set, MUIA_ShowMe, TRUE );
		DoMethod( icon_modus , MUIM_Notify, MUIA_Cycle_Active   , 0             , obj    , 3, MUIM_Set, MA_Panel_Imagepath,( ( def_path)? def_path : (STRPTR)"" ) );
		set( icon_modus, MUIA_Cycle_Active, ( ( ( ( def_path) && ( data->imagepath ) && strlen( data->imagepath ) && !strcmp( data->imagepath,def_path ) )|| ( data->imagepath == 0 )|| !strlen( data->imagepath) )? 0 : 1 ) );
	}
	return( (ULONG) group );
}

/************************************************************************/

DEFSMETHOD(Panel_SaveConfig)
{
	APTR pl, pi; /* list, item */
	ASSERT( msg->pctx );

	if( !( pl = prefspool_item_get( msg->pctx, NULL, DSI_LISTPOOL_PANEL, NULL, NULL ) ) )
	{
		pl = prefspool_item_add( msg->pctx, NULL, DSI_LISTPOOL_PANEL, NULL, 0 );
	}
	if (pl)
	{
		if (!(pi = prefspool_item_get( msg->pctx, pl, msg->index | DSF_LISTPOOL, NULL, NULL ) ) )
		{
			pi = prefspool_item_add( msg->pctx, pl, msg->index | DSF_LISTPOOL, NULL, 0 );
		}
		if( pi )
		{
			STRPTR imagepath,uri;
			if( get( obj, MA_Panel_Imagepath, &imagepath ) )
			{
				setprefsstr_lp( msg->pctx, pi, DSI_LISTPOOL_PANEL_IMAGEPATH, imagepath ? imagepath : (STRPTR)"" );
			}
			if( get( obj, MA_Panel_URI, &uri ) )
			{
				setprefsstr_lp( msg->pctx, pi, DSI_LISTPOOL_PANEL_URI, uri ? uri : (STRPTR)"" );
			}
		}
		/* XXX */
	}
	/* XXX */
	return( 0 ); /* XXX: what about some useful result ? */
}


DEFSMETHOD(Panelbutton_GetAttr)
{
	APTR group; //,win;
	//PDB(("getattr %d\n",msg->target));
	switch ( msg->attrID)
	{
		case MA_AmbientPanel_Group_Size:
		case MA_AmbientPanel_Group_Horiz:
			if(GetAttr(MUIA_Parent,obj,(ULONG*)&group) && (group))
			{
				BOOL ret = GetAttr(msg->attrID,group,msg->storage);
				PDB(("getattr %x %d %d\n",msg->storage,*msg->storage,ret));
				return ret;
			}
			break;
		default:
			return GetAttr(msg->attrID,obj,msg->storage);
			break;
		
		
		/*case t_Window:
		PDB(("getattr \n"));
			if(GetAttr(MUIA_WindowObject,obj,(ULONG*)&win) && (win))
			{
				BOOL ret = GetAttr(msg->attrID,win,msg->storage);
				PDB(("getattr %x %d %d\n",msg->storage,*msg->storage,ret));
				return ret;
			}		
			break;*/
	}
	PDB(("\n"));
	return 0;
}


#if 0
DEFSMETHOD(Panelbutton_Callback)
{
	GETDATA;
	struct TagItem *tstate = msg->ops_AttrList, *tag;
	APTR destObj;
//	PDB(("%x d %x %x\n",obj,tstate->ti_Data,tstate->ti_Tag,MA_Panelgroup_Size));
	//return 0;
	if ( (!data->app_object) || ( data->callbackmode == PanelObject_CallbackMode_None ) )return 0;
	if ( ( data->callbackmode == PanelObject_CallbackMode_MUIApp ) ) destObj = data->app_object;
	if ( destObj )
	while ( (tag = (struct TagItem *) NextTagItem(&tstate) ) )
	{
		switch (tag->ti_Tag)
		{	
			case MA_Panelgroup_Size:
			case MA_Panelgroup_Horiz:
			case MA_Panel_Highlighted:
				DoMethod(data->app_object,MUIM_Application_PushMethod,destObj,3,MUIM_Set,tag->ti_Tag,tag->ti_Data);
				break;
		}
	}
	return 0;
}

#endif


DEFSMETHOD(Panelgroup_MoveMode_Start)
{
	//GETDATA;
	
	return( 0 );
}


/************************************************************************/

BEGINMTABLE

DECNEW
DECGET
DECSET
DECDISPOSE
DECMMETHOD(ContextMenuBuild)
DECMMETHOD(ContextMenuAdd)
DECMMETHOD(ContextMenuChoice)
DECMMETHOD(AskMinMax)
DECSMETHOD(Icon_AddBitMap)
DECMMETHOD(DragQuery)
DECSMETHOD(Panelbutton_AddIcon)
DECMMETHOD(Draw)
DECSMETHOD(Panel_SaveConfig)
DECTMETHOD(Panelbutton_Launch)
DECMMETHOD(DragFinish)
DECMMETHOD(DragBegin)
DECTMETHOD(Panel_Settings_Group)


DECSMETHOD(Panelgroup_MoveMode_Start)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Area, panelbasebuttonclass)

#endif