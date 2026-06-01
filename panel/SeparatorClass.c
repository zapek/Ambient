
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


#include "muifuncs.h"
#include "MUIClasses.h"
#include "debug.h"

#include "../mui_func.h"
#include "paneltags.h"



#include "ambient_cat.h"

struct MUI_CustomClass *SeparatorClass_Class(void);

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

struct Data {
	APTR   cmenu;
	ULONG  brightness_threshold;
	ULONG  alpha_threshold;
};

/************************************************************************/

/************************************************************************/

static ULONG mAskMinMax(struct IClass *cl,Object *obj,struct MUIP_AskMinMax*msg)
{
	ULONG size   = getv( _parent(obj), MA_Panelgroup_Size );
	ULONG horiz  = getv( _parent(obj), MA_Panelgroup_Horiz );
	
	ULONG defsize = 1;

	DoSuperMethodA(cl,obj,msg);
	if( horiz )
	{
		if( msg->MinMaxInfo->MinWidth < defsize )
		{
			msg->MinMaxInfo->MinWidth = defsize;
			msg->MinMaxInfo->MaxWidth = defsize;
		}
		msg->MinMaxInfo->MinHeight = size;
		msg->MinMaxInfo->MaxHeight = size;
	} else {
		if( msg->MinMaxInfo->MinHeight < defsize )
		{
			msg->MinMaxInfo->MinHeight = defsize;
			msg->MinMaxInfo->MaxHeight = defsize;
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
			*msg->opg_Storage = MV_Panel_Type_Separator;
			break;
	/*	case MA_Panel_Properties:
            *msg->opg_Storage = TRUE;
			break;*/

		case MA_Panel_Extern_DisplayName:
			*msg->opg_Storage = (ULONG) "Separator";//GSI(MSG_PANELITEM_SEPARATOR);
			break;
		case MA_Panel_Extern_Version:
			*msg->opg_Storage = 1;
			break;
		case MA_Panel_Extern_Revision:
			*msg->opg_Storage = 1;
			break;
		case MA_Panel_Extern_Author:
			*msg->opg_Storage = (ULONG) "Christian Rosentreter,\nAmbient Open Source Team";
			break;
		case MA_Panel_Extern_Description:
			*msg->opg_Storage = (ULONG) "Separator";//GSI(MSG_PANELITEM_SEPARATORDESC);
			break;
		default:
			result = DoSuperMethodA(cl,obj,msg);
			break;
	}
	return( result );
}

/************************************************************************/

static ULONG mDraw(struct IClass *cl,Object *obj,struct MUIP_Draw *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	DoSuperMethodA(cl,obj,msg);


	if( msg->flags & ( MADF_DRAWOBJECT | MADF_DRAWUPDATE ) )
	{
		struct RastPort *rp = _rp(obj);

		ULONG mleft         = _mleft(obj);
		ULONG mtop          = _mtop(obj);
		ULONG mright        = _mright(obj);
		ULONG mbottom       = _mbottom(obj);
		ULONG xmid          = _mleft(obj) + _mwidth (obj) / 2;
		ULONG ymid          = _mtop(obj)  + _mheight(obj) / 2;
		ULONG x, y;
		ULONG colour = 0x00EFEFEF;

		ULONG horiz  = getv( _parent(obj), MA_Panelgroup_Horiz );
		ULONG hasalpha;
		
		GetAttr( SA_OpacitySupport, _screen( obj ), &hasalpha );
		DoMethod( _parent(obj), MM_Panelgroup_RefreshRect, mleft, mtop, _mwidth(obj), _mheight(obj), _rp( obj ) );
		if(1)// ( hasalpha == SAOS_OpacitySupport_None )  || ( hasalpha == SAOS_OpacitySupport_OnOff ) )
		{
			data->brightness_threshold  = 255;
		}
		else
		{
#warning
		//	data->brightness_threshold = gfx_analyze_average_brightness( rp, mleft, mtop, _mwidth(obj), _mheight(obj) );
		}
//		data->alpha_threshold = gfx_analyze_average_alpha( rp, mleft, mtop, _mwidth(obj), _mheight(obj) );

		if( data->brightness_threshold > 128 )
		{
			colour = 0x0000000;
		}
		if( data->alpha_threshold > 128 )
		{
			colour += 0xa0000000;
		} else {
			colour += 0xFF000000;
		}
		if( horiz )
		{   
			for( y = mtop + 1 ; y < mbottom ; y++ )
			{
				WriteRGBPixel( rp, xmid,  y, colour );
			}
		} else {
			for( x = mleft + 1 ; x < mright ; x++ )
			{
				WriteRGBPixel( rp, x, ymid, colour );
			}
		
		}
	}
	return( 0 );
}

/***********************************************************************/




DISPATCHER(SeparatorClass)
{
	switch (msg->MethodID)
	{
	//	case OM_NEW             			: return(mNew			(cl,obj,(struct opSet*)msg));
	//	case OM_SET        	    			: return(mSet			(cl,obj,(struct opSet*)msg));
		case OM_GET			    			: return(mGet			(cl,obj,(struct opGet *)msg));
		case MUIM_AskMinMax		   			: return(mAskMinMax		(cl,obj,(struct MUIP_AskMinMax *)msg));
		case MUIM_Draw     					: return(mDraw     		(cl,obj,(struct MUIP_Draw *)msg));
	}
	return(DoSuperMethodA(cl,obj,msg));
}
DISPATCHER_END



struct MUI_CustomClass *SeparatorClass_Class(void)
{
	return MUI_CreateCustomClass(NULL,MUIC_Area,NULL,sizeof(struct Data),DISPATCHER_REF(SeparatorClass));
}

