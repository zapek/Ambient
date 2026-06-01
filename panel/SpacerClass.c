#include <exec/types.h>

#include <proto/graphics.h>

#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>



#include "muifuncs.h"
#include "MUIClasses.h"
#include "debug.h"

#include "../mui_func.h"
#include "../name.h"
#include "paneltags.h"


#include "ambient_cat.h"


#warning locale pfusch
/* catmaker */
#define CATCOMP_NUMBERS
extern const char * const __stringtable[];
#ifdef __SASC
#define GSI(x) __stringtable[x]
#else
#define GSI(x) ( STRPTR )__stringtable[x]
#endif
/************************************************************************/

struct MUI_CustomClass *SpacerClass_Class(void);

struct Data {
	ULONG  size;
	APTR   cmenu;
	STRPTR help;

	ULONG  brightness_threshold;  
	BOOL   brightness_checked;    
};

/************************************************************************/

static void doset( APTR obj, struct Data *data, struct TagItem *tags)
{
	struct TagItem *tstate = tags, *tag;
	while ((tag = (struct TagItem *) NextTagItem(&tstate)))
	{
		switch (tag->ti_Tag)
		{		
			case MA_Panelgroup_Locked:
				MUI_Redraw( obj, MADF_DRAWOBJECT );
				break;

			case MA_Panelgroup_ImageSpec:  
				data->brightness_checked = FALSE; /* invalidate cache, see MUIM_Draw */
				MUI_Redraw( obj, MADF_DRAWOBJECT );
				break;
		}
	} 
}

/************************************************************************/

static ULONG mNew(struct IClass *cl,Object *obj,struct opSet *msg)
{
	if( ( obj = DoSuperNew( cl, obj,
									MUIA_Frame      , MUIV_Frame_None,
									MUIA_InnerLeft  , 1,
									MUIA_InnerRight , 1,
									MUIA_InnerTop   , 1,
									MUIA_InnerBottom, 1,
									TAG_DONE ) ) )
	{
		struct Data *data = INST_DATA( cl, obj );

		data->help  = NULL;
#warning
		data->size  = 20;//_aprefs( panelspacersize );
		
		data->brightness_checked   = FALSE;
		data->brightness_threshold = 255; /* default to bright bgs */

		doset( obj, data, INITTAGS );

	}
	return( (ULONG) obj );
}

/************************************************************************/

static ULONG mDispose(struct IClass *cl,Object *obj,Msg msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	if( data->help )
	{
		name_delete( data->help );
	}

	return( DoSuperMethodA(cl,obj,msg));
}

/************************************************************************/

static ULONG mAskMinMax(struct IClass *cl,Object *obj,struct MUIP_AskMinMax*msg)
{
    struct Data *data = (struct Data*)INST_DATA(cl,obj);
	ULONG size;
	ULONG horiz;

	DoSuperMethodA(cl,obj,msg);

	size  = getv( _parent(obj), MA_Panelgroup_Size );
	horiz = getv( _parent(obj), MA_Panelgroup_Horiz );

	if( horiz )
	{
		if( msg->MinMaxInfo->MinWidth < data->size )
		{
			msg->MinMaxInfo->MinWidth = data->size;
			msg->MinMaxInfo->MaxWidth = data->size;
		}
		msg->MinMaxInfo->MinHeight = size;
		msg->MinMaxInfo->MaxHeight = size;
	} else {
		if( msg->MinMaxInfo->MinHeight < data->size )
		{
			msg->MinMaxInfo->MinHeight = data->size;
			msg->MinMaxInfo->MaxHeight = data->size;
		}
		msg->MinMaxInfo->MinWidth = size;
		msg->MinMaxInfo->MaxWidth = size;
	}
	return( 0 );
}

/************************************************************************/

static ULONG mGet(struct IClass *cl,Object *obj,struct opGet *msg)
{
	ULONG result = TRUE;

	switch( msg->opg_AttrID )
	{
		case MA_Panel_Type:
			*msg->opg_Storage = MV_Panel_Type_Spacer;
			break;
		/*case MA_Panel_Properties:
            *msg->opg_Storage = TRUE;
			break;*/
		case MA_Panel_Extern_DisplayName:
			*msg->opg_Storage = (ULONG) "Spacer";//GSI(MSG_PANELITEM_SPACER);
			break;
		case MA_Panel_Extern_Version:
			*msg->opg_Storage = 1;
			break;
		case MA_Panel_Extern_Revision:
			*msg->opg_Storage = 2;
			break;
		case MA_Panel_Extern_Author:
			*msg->opg_Storage = (ULONG) "Vladimir Alaev,\nAmbient Open Source Team";
			break;
		case MA_Panel_Extern_Description:
			*msg->opg_Storage = (ULONG) "Spacer";//GSI(MSG_PANELITEM_SPACERDESC);
			break;
		default:
			result = DoSuperMethodA(cl,obj,msg);
			break;
	}
	return( result );
}

static ULONG mSet(struct IClass *cl,Object *obj,struct opSet *msg)
{
	struct TagItem *tstate, *tag;
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	tstate = msg->ops_AttrList;
	doset( obj, data,msg->ops_AttrList); 
	return DoSuperMethodA(cl,obj,msg);
}

static ULONG mDraw(struct IClass *cl,Object *obj,struct MUIP_Draw *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	DoSuperMethodA(cl,obj,msg);
	if (msg->flags & MADF_DRAWOBJECT)// &&
//		  (!getv(_parent(obj), MA_Panelgroup_Locked)))
	{
		struct RastPort *rp = _rp(obj);

        ULONG mleft         = _mleft(obj);
		ULONG mtop          = _mtop(obj);
		ULONG mright        = _mright(obj);
		ULONG mbottom       = _mbottom(obj);

		ULONG x, y;
		ULONG colour = 0x00FFFFFF;
		BOOL ff;
		DoMethod( _parent(obj),MM_Panelgroup_RefreshRect, mleft, mtop, _mwidth( obj ), _mheight(obj), _rp( obj ) );
		/*   check if we have to draw white or black lines; cache the value 
		 */
		if( data->brightness_checked == FALSE )
		{
			data->brightness_checked   = TRUE;
			#warning
			data->brightness_threshold = 100;//gfx_analyze_average_brightness( rp, mleft, mtop, _mwidth(obj), _mheight(obj) );
		}

		if( data->brightness_threshold > 128 )
		{
			colour = 0xFF000000;
		}

		for( x = mleft + 1, ff = TRUE ; x < mright ; x++ )
		{
			if (ff) 
			{ 
			//	WriteRGBPixel( rp, x, mtop,    colour );
			//	WriteRGBPixel( rp, x, mbottom, colour );
			}

			ff = !ff;
		}

		for( y = mtop + 1, ff = TRUE ; y < mbottom ; y++ )
		{
			if (ff) 
			{ 
			//	WriteRGBPixel( rp, mleft,  y, colour );
			//	WriteRGBPixel( rp, mright, y, colour );
			}

			ff = !ff;
		}
	}
	return( 0 );
}

/************************************************************************/


DISPATCHER(SpacerClass)
{
	switch (msg->MethodID)
	{
		case OM_NEW             			: return(mNew			(cl,obj,(struct opSet*)msg));
		case OM_SET        	    			: return(mSet			(cl,obj,(struct opSet*)msg));
		case OM_GET			    			: return(mGet			(cl,obj,(struct opGet *)msg));
		case OM_DISPOSE						: return(mDispose		(cl,obj,(Msg)msg));
		case MUIM_AskMinMax		   			: return(mAskMinMax		(cl,obj,(struct MUIP_AskMinMax *)msg));
		case MUIM_Draw     					: return(mDraw     		(cl,obj,(struct MUIP_Draw *)msg));
	}
	return(DoSuperMethodA(cl,obj,msg));
}
DISPATCHER_END



struct MUI_CustomClass *SpacerClass_Class(void)
{
	return MUI_CreateCustomClass(NULL,MUIC_Area,NULL,sizeof(struct Data),DISPATCHER_REF(SpacerClass));
}
