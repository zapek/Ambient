#include <graphics/rastport.h>
#include <intuition/classes.h>
#include <datatypes/pictureclass.h>
#include <graphics/gfxmacros.h>
#include <exec/types.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/icon.h>
#include <proto/timer.h>
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>
#include <clib/datatypes_protos.h>
#include <clib/alib_protos.h>
#include <graphics/rpattr.h>

#include "muifuncs.h"
#include "MUIClasses.h"
#include "debug.h"



#include <proto/panel.h>
#include <libraries/panel.h>


#include "gfx_bitmap.h"
#include "gfx_scale.h"
#include "gfx_blit.h"
#include "gfx_pen.h"

#include "paneltags.h"
#include "panellib.h"

#include "prefs.h"
#include "def_view_logo.h"

#include <proto/ambient.h>

struct MUI_CustomClass *BaseButtonClass_Class(void);
ULONG icon_read(STRPTR filename, APTR obj);

/************************************************************************/

#define STATUS_SELECTED 	0x00000001
#define STATUS_HIGHLIGHT 	0x00000002
#define STATUS_DROPTARGET 	0x00000004

#define PANEL_MESSAGE_COUNT 10 

/************************************************************************/

STRPTR name_build_sysify(CONST_STRPTR name);
void name_delete(STRPTR name);


struct Data
{
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
	ULONG color,colorfade,brighten,darken;
	APTR tempbm,selbm;
	APTR bm, alt_bm;
	struct BitMap *alt_nbm;
	APTR x_bm;
	APTR sclbm,alt_sclbm;
	ULONG scalemode,altbm_rwidth,altbm_rheight; 
	ULONG altbm_pos,altbm_rsize;
	ULONG win_x,win_y,win_xs,win_ys;
	ULONG viewmode;
	ULONG sortmode;
	LONG icontype;
	struct timeval lastclick;
};



/************************************************************************/

static VOID clear_bitmap(APTR bm)
{
	struct RastPort rp;

	InitRastPort( &rp );
	rp.BitMap = gfx_bitmap_bm( bm );
	FillPixelArray(&rp,0,0,gfx_bitmap_width( bm ),gfx_bitmap_height( bm ),0x00000000);
}

/************************************************************************/

static VOID set_def_image(APTR obj, struct Data *data)
{
	if((data->bm = gfx_bitmap_create(DEF_VIEW_WIDTH ,DEF_VIEW_HEIGHT,DEF_VIEW_DEPTH,BITMAPTAG_Clear, TRUE,  TAG_DONE)))
	{
		gfx_blit(def_view, data->bm, BLITTAG_SrcType, BLITVAL_SrcType_Array,
													 BLITTAG_SrcFormat, BLITVAL_SrcFormat_ARGB,
													 BLITTAG_DstWidth, DEF_VIEW_WIDTH,
													 BLITTAG_DstHeight, DEF_VIEW_HEIGHT,
													 BLITTAG_Alpha, 0xffffffff,
													 TAG_DONE);
	}
}

/**********************************************************************/

static VOID process_image(APTR obj, struct IClass *cl, APTR dstbm)
{
	struct RastPort rp;
	
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	InitRastPort( &rp );
	rp.BitMap = gfx_bitmap_bm( dstbm );

	switch ( data->desired_effect )
	{
		case PANEL_EFFECT_TINT:
			ProcessPixelArray( &rp, 0, 0, gfx_bitmap_width( dstbm ), gfx_bitmap_height( dstbm ), POP_TINT, data->color, NULL );
		break;
	
		case PANEL_EFFECT_BLUR:
			ProcessPixelArray( &rp, 0, 0, gfx_bitmap_width( dstbm ), gfx_bitmap_height( dstbm ), POP_BLUR, 0, NULL );
		break;

		case PANEL_EFFECT_BRIGHTEN:
			ProcessPixelArray( &rp, 0, 0, gfx_bitmap_width( dstbm ), gfx_bitmap_height( dstbm ), POP_BRIGHTEN, data->brighten, NULL );
		break;

		case PANEL_EFFECT_DARKEN:
			ProcessPixelArray( &rp, 0, 0, gfx_bitmap_width( dstbm ), gfx_bitmap_height( dstbm ), POP_DARKEN, data->darken, NULL );
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
			ProcessPixelArray( &rp, 0, 0, gfx_bitmap_width( dstbm ), gfx_bitmap_height( dstbm ), POP_TINTFADE, data->colorfade, NULL );
		break;
	}
}

/*   */
static void doset( APTR obj, struct Data *data, struct TagItem *tags,BOOL _init )
{
	struct TagItem *tstate = tags, *tag;
	ULONG old_status = data->status;
	while ((tag = (struct TagItem *) NextTagItem(&tstate)))
	{
		if (_init)
		{
			switch (tag->ti_Tag)
			{
				
				}
		}
		switch (tag->ti_Tag)
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
;
			break;

		case MA_Panel_Highlighted:
			if( tag->ti_Data )
			{
				data->status = data->status | STATUS_HIGHLIGHT;
			} else {
				data->status = data->status & ~STATUS_HIGHLIGHT;
			}
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
			//	name_delete( data->imagepath );
			}
			if(tag->ti_Data)data->imagepath = name_build_sysify( (STRPTR) tag->ti_Data );
			else data->imagepath = 0;
			if((data->imagepath) && ( strlen( data->imagepath )))
			{
			//	PDB(("%s\n",data->imagepath));
#warning load icon in special cases 				
			/*	struct DiskObject *diskobj;
				ReadIcon(data->imagepath,obj);
				if((diskobj = GetDiskObject(data->imagepath)))
				{
				
					if(diskobj->do_DrawerData)
					{
						PDB(("%d\n",diskobj->do_DrawerData->dd_NewWindow.LeftEdge));
					}
					PDB(("disko\n"));	
				}*/
			
				//	PDB(("impath %s\n",data->imagepath));
				icon_read( data->imagepath, obj );
			} else {
			//	PDB(("%x\n",data->icontype));
			//	test(obj);
				set_def_image(obj, data);
				if( data->icontype )
				{
			//		PDB(("%x %d !%s!\n",_screen(obj),data->icontype,data->uri));
				//	load_icon( obj, data, &data->bm, &data->selbm );
				}
			}
			MUI_Redraw( obj, MADF_DRAWUPDATE );
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
		case MA_Icon_WindowTop:
			data->win_y = tag->ti_Data;
			break;
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
		case MA_PANEL_DROPTARGET:
			if(tag->ti_Data) data->status = data->status | STATUS_DROPTARGET;
			else data->status = data->status & ~STATUS_DROPTARGET;
			break;
		}
		
	}
	if(old_status != data->status) /* check if and what GFX effect will be applied */
	{
		APTR main_ppool;
		data->desired_effect = PANEL_EFFECT_NONE;
		if((main_ppool = LockPPool("MAIN_PPOOL",PPOOL_TYPE_MAIN)))
		{
			ULONG *brighten = NULL;
			ULONG *darken = NULL;
			struct MUI_PenSpec *tint_spec;
			ULONG *desired_effect;

			if( data->status == STATUS_DROPTARGET )
			{
				GetPPoolItem(main_ppool,0, DSI_PANEL_DRAGDROP_EFFECT,(APTR*)&desired_effect,NULL );
				GetPPoolItem(main_ppool,0, DSI_PANEL_DRAGDROP_BRIGHTEN,(APTR*)&brighten,NULL );
				GetPPoolItem(main_ppool,0, DSI_PANEL_DRAGDROP_TINT,(APTR*)&tint_spec,NULL );
				data->color     = gfx_get_penspec_value(muiRenderInfo( obj ),tint_spec);
				GetPPoolItem(main_ppool,0, DSI_PANEL_DRAGDROP_TINTFADE,(APTR*)&tint_spec,NULL );
				data->colorfade     = gfx_get_penspec_value(muiRenderInfo( obj ),tint_spec);
				data->desired_effect = *desired_effect;
			}
			else if( data->status ==  STATUS_HIGHLIGHT)
			{
				GetPPoolItem(main_ppool,0, DSI_PANEL_HIGHLIGHT_EFFECT,(APTR*)&desired_effect,NULL );
				GetPPoolItem(main_ppool,0, DSI_PANEL_HIGHLIGHT_BRIGHTEN,(APTR*)&brighten,NULL );
				GetPPoolItem(main_ppool,0, DSI_PANEL_HIGHLIGHT_DARKEN,(APTR*)&darken,NULL );
				GetPPoolItem(main_ppool,0, DSI_PANEL_HIGHLIGHT_TINT,(APTR*)&tint_spec,NULL );
				data->color     = gfx_get_penspec_value(muiRenderInfo( obj ),tint_spec);
				GetPPoolItem(main_ppool,0, DSI_PANEL_HIGHLIGHT_TINTFADE,(APTR*)&tint_spec,NULL );
				data->colorfade     = gfx_get_penspec_value(muiRenderInfo( obj ),tint_spec);		
			}
			if(brighten) data->brighten = *brighten;
			if(darken) data->darken = *darken;
			if(desired_effect) 
			{
				data->desired_effect = *desired_effect;
				if(data->desired_effect == PANEL_EFFECT_CLONE_ICONVIEW)
				{
					if( data->status & STATUS_DROPTARGET )
					{
						GetPPoolItem(main_ppool,0, DSI_DRAGDROP_TINTVAL,(APTR*)&tint_spec,NULL );
						data->color     = gfx_get_penspec_value(muiRenderInfo( obj ),tint_spec);
						data->desired_effect = PANEL_EFFECT_TINT;
					}
					else if( data->status & STATUS_SELECTED )
					{
						GetPPoolItem(main_ppool,0, DSI_ICON_TINTVAL,(APTR*)&tint_spec,NULL );
						data->color     = gfx_get_penspec_value(muiRenderInfo( obj ),tint_spec);
						data->desired_effect = PANEL_EFFECT_TINT;
					}
					else if( data->status & STATUS_HIGHLIGHT )
					{
						data->brighten = 0x20;
						data->desired_effect = PANEL_EFFECT_BRIGHTEN;
					}
				}
			}
			UnLockPPool(main_ppool);
		}
		MUI_Redraw( obj, MADF_DRAWUPDATE );
	}
}


static ULONG mNew(struct IClass *cl,Object *obj,struct opSet *msg)
{
	if( ( obj = DoSuperNew( cl, obj,
			
					TAG_MORE			,	msg->ops_AttrList,
					MUIA_Dropable, TRUE,
					TAG_DONE ) ) )
	{
			struct Data *data = (struct Data*) INST_DATA( cl, obj );
			data->bm = 0;
		
			SetAttrs( obj, MUIA_InputMode, MUIV_InputMode_RelVerify, MUIA_ShowSelState, FALSE, TAG_DONE );
			DoMethod( obj, MUIM_Notify  , MUIA_Pressed, FALSE, obj, 1, MM_Panelbutton_Launch,0);
			data->drawmode =  PanelObject_DrawMode_None;
			data->scalemode =  PanelObject_ScaleMode_Fit;
			data->altbm_pos = PanelObject_Position_Mid;
			data->imagepath = data->uri = 0L;
			data->altbm_rwidth = 100;
			data->altbm_rheight = 100; 
			data->altbm_rsize = 100;
			doset( obj, data, msg->ops_AttrList, TRUE );
		if(!data->bm) set_def_image(obj, data);
	}
	return( (ULONG) obj );
}


static ULONG mSet(struct IClass *cl,Object *obj,struct opSet *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	doset( obj, data,msg->ops_AttrList,FALSE); 
	return DoSuperMethodA(cl,obj,msg);
}


/************************************************************************/

static ULONG mGet(struct IClass *cl,Object *obj,struct opGet *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
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
			result = DoSuperMethodA(cl,obj,msg);
			break;
	}
	return( result );
}

/************************************************************************/




static ULONG mAskMinMax(struct IClass *cl,Object *obj,struct MUIP_AskMinMax*msg)
{

	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	
	
	ULONG size = 0;
	ULONG w,h;
	APTR parent;
	DoSuperMethodA(cl,obj,msg);
	GetAttr( MUIA_Parent, obj, (ULONG*) &parent );
   
	GetAttr( MA_Panelgroup_Size, parent, &size );
	if( ( data->bm ) && (parent) )
	{
		ULONG horiz,grid;
		GetAttr( MA_Panelgroup_Horiz, parent, &horiz );
		GetAttr( MA_Panelgroup_GridMode, parent, &grid );
		w = gfx_bitmap_width( data->bm );
		h = gfx_bitmap_height( data->bm );
		#warning GRID mode forced
		if(TRUE)// ( w == h ) || ( grid ) ) 
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
	else
	{
		msg->MinMaxInfo->MinWidth  = size;
		msg->MinMaxInfo->MinHeight = size;
		msg->MinMaxInfo->MaxWidth  = size;
		msg->MinMaxInfo->MaxHeight = size;
	}
	return (0);
}

	

static ULONG mDraw(struct IClass *cl,Object *obj,struct MUIP_Draw *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	BOOL sclbm_used = FALSE;
	DoSuperMethodA(cl,obj,msg);
	
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
		DoMethod( _parent(obj),MM_Panelgroup_RefreshRect, mleft, mtop, mwidth, mheight, _rp( obj ) );
		//if(data->drawmode !=  PanelObject_DrawMode_Replace )
		{
			/* default graphics aka icon needs to be drawn */
			/* external bitmap disabled for the moment*/
			tbm = data->bm;
		
			gfx_scale_calc_aspect_constraints(
										gfx_bitmap_width(tbm),// data->bm ),
										gfx_bitmap_height(tbm),// data->bm ),
										&scalewidth,
										&scaleheight
										);
			#warning scaling
			if(  data->sclbm )
			{
			
				gfx_scale( tbm, data->sclbm, scalewidth, scaleheight,
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
	}
	return(0);
}

static ULONG mAddBitMap(struct IClass *cl,Object *obj,struct MP_Icon_AddBitMap *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);

	
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

		#if 1//USE_PNGICONS
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
		
		#if 1//USE_SVGICONS
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
		//	PDB(("out of bound bitmap: val: %ld\n", msg->type));
			break;
		#endif
	}
	return( 0 );

	
}
    

static ULONG mLaunch(struct IClass *cl,Object *obj,struct MP_Panelbutton_Launch *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	ULONG doubleclick;
	struct timeval currenttime;

	if (TimerBase->lib_Version >= 52)
		GetUTCSysTime( &currenttime );
	else
		GetSysTime( &currenttime );
	doubleclick = DoubleClick( data->lastclick.tv_secs, data->lastclick.tv_micro, currenttime.tv_secs, currenttime.tv_micro );
	data->lastclick = currenttime;
//	PDB(("%d\n",doubleclick));
	return( doubleclick );
}



/************************************************************************/

static ULONG mDragQuery(struct IClass *cl,Object *obj,struct MUIP_DragQuery *msg)
{
	#warning check for parent locked status
//	PDB(("\n"));
	//if( getv( _parent( obj ), MA_Panelgroup_Locked ) )
	{
		return( MUIV_DragQuery_Accept );
	}
	return( MUIV_DragQuery_Refuse );
}


/************************************************************************/

static ULONG mDragFinish(struct IClass *cl,Object *obj,struct MUIP_DragFinish *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	ULONG rc = DoSuperMethodA(cl,obj,msg);
	data->status = data->status & ~STATUS_DROPTARGET;
	MUI_Redraw( obj, MADF_DRAWUPDATE );

	return( rc );
}

/************************************************************************/

static ULONG mDragBegin(struct IClass *cl,Object *obj,struct MUIP_DragBegin *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	ULONG rc = DoSuperMethodA(cl,obj,msg);
	data->status = data->status | STATUS_DROPTARGET;
	MUI_Redraw( obj, MADF_DRAWUPDATE );
	return( rc );
}

static ULONG mExtraBitmap(struct IClass *cl,Object *obj,struct MP_Panelbutton_ExtraBitmap *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	APTR abm;
	
	if((msg->bm)&&(abm = gfx_bitmap_create_from_native(msg->bm,msg->width,msg->height)))
	{
		
		if((data->alt_bm = v_gfx_bitmap_create(msg->width, msg->height, 32,0)))//bmTags ))
		{
			gfx_blit(abm, data->alt_bm,TAG_DONE);
		}
		data->drawmode =  PanelObject_DrawMode_Replace;
		gfx_bitmap_delete(abm);
	}
	else
	{
		data->drawmode = PanelObject_DrawMode_None;
		if( !data->alt_bm ) gfx_bitmap_delete(data->alt_bm);
		data->alt_bm = 0;
	}
	MUI_Redraw( obj, MADF_DRAWOBJECT );
	return 0;
}

static ULONG mSaveConfig(struct IClass *cl,Object *obj,struct MP_Panel_SaveConfig *msg)
{
	APTR pl, pi; /* list, item */
	if(!(pl = (APTR)GetPPoolItem(msg->panel_lock, NULL,  DSI_LISTPOOL_PANEL, NULL, NULL)))
	{
		pl = (APTR) AddPPoolItem(msg->panel_lock, NULL, DSI_LISTPOOL_PANEL, NULL, 0 );
	}
	if (pl)
	{
		if(!(pi = (APTR)GetPPoolItem(msg->panel_lock, pl, msg->index | DSF_LISTPOOL, NULL, NULL)))
		{
			pi = (APTR) AddPPoolItem(msg->panel_lock, pl, msg->index | DSF_LISTPOOL, NULL, (ULONG)NULL );
		}
		if( pi )
		{
			STRPTR imagepath = 0;
			STRPTR uri;
			if(( get( obj, MA_Panel_Imagepath, &imagepath ) && (imagepath) && (strlen(imagepath))))
			{
			//	PDB(("ima !%s! ",imagepath));
				AddPPoolItem(msg->panel_lock, pi,DSI_LISTPOOL_PANEL_IMAGEPATH, imagepath , strlen(imagepath)+1);
			}
			else
			{
			//	PDB(("no imagepath\n"));
		//		RemovePanelItem(msg->panel_lock,pi,DSI_LISTPOOL_PANEL_IMAGEPATH);
			}
		
			if( (get( obj, MA_Panel_URI, &uri ) &&(uri)))
			{
			//	PDB(("uri !%s! ",uri));
				AddPPoolItem(msg->panel_lock, pi,DSI_LISTPOOL_PANEL_URI, uri , strlen(uri)+1);
			}
			//PDB((" \n"));
		}			
	}
	return( 0 ); /* XXX: what about some useful result ? */
}

static ULONG mPrefsUpdate(struct IClass *cl, Object *obj, struct MP_Panel_PrefsUpdate *msg )
{
	STRPTR prefs_str;
	APTR pi = 0L; 
	if(msg->prefs_ID != DSI_LISTPOOL_PANEL_IMAGEPATH) return 0;
	
	if(GetPPoolItem(msg->ppool, pi, msg->prefs_ID , (APTR) &prefs_str , NULL ))
	{
		SetAttrs(obj,MA_Panel_Imagepath,prefs_str,TAG_DONE);
	}
	return 0;
}

DISPATCHER(BaseButtonClass)
{
	switch (msg->MethodID)
	{
		case OM_NEW             			: return(mNew			(cl,obj,(struct opSet*)msg));
		case OM_SET        	    			: return(mSet			(cl,obj,(struct opSet*)msg));
		case OM_GET			    			: return(mGet			(cl,obj,(struct opGet *)msg));
		case MUIM_AskMinMax		   			: return(mAskMinMax		(cl,obj,(struct MUIP_AskMinMax *)msg));
		case MUIM_Draw     					: return(mDraw     		(cl,obj,(struct MUIP_Draw *)msg));
		case MM_Panelbutton_Launch			: return(mLaunch		(cl,obj,(struct MP_Panelbutton_Launch *)msg));
		case MM_Icon_AddBitMap				: return(mAddBitMap		(cl,obj,(struct MP_Icon_AddBitMap *)msg));
		case MUIM_DragQuery					: return(mDragQuery		(cl,obj,(struct MUIP_DragQuery *)msg));
		case MUIM_DragFinish				: return(mDragFinish	(cl,obj,(struct MUIP_DragFinish *)msg));
		case MUIM_DragBegin					: return(mDragBegin		(cl,obj,(struct MUIP_DragBegin *)msg));
		case MM_Panelbutton_ExtraBitmap		: return(mExtraBitmap	(cl,obj,(struct MP_Panelbutton_ExtraBitmap*)msg));
		case MM_Panel_SaveConfig			: return(mSaveConfig	(cl,obj,(struct MP_Panel_SaveConfig *)msg));
	//	case MM_Panel_Setup					: return(mPanel_Setup	(cl,obj,(struct MP_Panel_Setup	*)msg));
		case MM_Panel_PrefsUpdate			:return( mPrefsUpdate	(cl,obj,(struct MP_Panel_PrefsUpdate*)msg));
	}
	return(DoSuperMethodA(cl,obj,msg));
}
DISPATCHER_END



struct MUI_CustomClass *BaseButtonClass_Class(void)
{
	return MUI_CreateCustomClass(NULL,MUIC_Area,NULL,sizeof(struct Data),DISPATCHER_REF(BaseButtonClass));
}
