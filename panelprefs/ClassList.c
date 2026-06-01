#include "muifuncs.h"




#define USE_INLINE_STDARG


#include <exec/libraries.h>
#include <libraries/commodities.h>
#include <dos/dos.h>
#include <clib/exec_protos.h>
#include <proto/asound.h>
#include <clib/alib_protos.h>
#include <clib/commodities_protos.h>
#include <mui/Listtree_mcc.h>
#include <proto/panel.h>
#include "../panel/MUIClasses.h"
#include "MUIClasses_Prefs.h"
#include "paneltags.h"
//#include "gfx_bitmap.h"
#include "panelitem.h"
#include "debug.h" 



extern struct Library *PanelBase;

struct Data {
	int dummy;
};

#warning
/*
static APTR create_image( APTR obj, APTR bm )
{
	APTR iobj;

	if ( ( iobj = BitmapObject,
		MUIA_Bitmap_Bitmap, gfx_bitmap_bm( bm ),
		MUIA_Bitmap_Height, PANELIMAGE_HEIGHT, 
		MUIA_Bitmap_Width,  PANELIMAGE_WIDTH,
		MUIA_FixHeight,     PANELIMAGE_HEIGHT, 
		MUIA_FixWidth,      PANELIMAGE_WIDTH,
		MUIA_Bitmap_Alpha,  0xffffffff,
		End ) )
	{
		return( (APTR) DoMethod( obj, MUIM_List_CreateImage, iobj, 0 ) );
	}

	return( NULL );
}*/


static ULONG mNew(struct IClass *cl,Object *obj,struct opSet *msg)
{
	struct Data *data;
	STRPTR class_name = NULL;
	LONG pti;
	struct PanelItem *pi;
	if( ( obj = DoSuperNew( cl, obj,
						InputListFrame,
						MUIA_List_MinLineHeight, PANELIMAGE_HEIGHT,
						TAG_MORE			,	 msg->ops_AttrList,TAG_DONE ) ) )
	{

		data = INST_DATA( cl, obj );
		
		if(PanelBase)
		{
#if 1
			/*  create internal panel items. */
			for( pti = 0 ; pti < PT_END ; pti++ ) 
			{
				if( ( pi = PanelItem_Create( pti, NULL ) ) ) 
				{ /* insert only if creation succeded. */
					DoMethod( obj, MUIM_List_InsertSingle, pi, MUIV_List_Insert_Bottom );
				}
			}

#warning no extern classes			
#if 0	
			while((class_name = NextExternClass(class_name)))
			{
				//kprintf("classlist new %s\n",class_name);
				if( ( pi = PanelItem_Create( PT_EXTERNAL, class_name ) ) ) 
				{ /* insert only if creation succeded. */
					DoMethod( obj, MUIM_List_InsertSingle, pi, MUIV_List_Insert_Bottom );
				}
			}
		#endif
	#endif
		}
	}
	return( (ULONG) obj );
}



static ULONG mGet(struct IClass *cl,Object *obj,struct opGet *msg)
{
	ULONG result = TRUE;
	PDB(("get obj %x islist %x tag %x\n",obj,MA_Panelitem_list_IsList,msg->opg_AttrID));
	switch( msg->opg_AttrID )
	{		
		case MA_Panelitem_list_IsList:
			*msg->opg_Storage = TRUE;
			break;
		default:
			result = DoSuperMethodA(cl,obj,msg);
			break;
	}
	return( result );
}


static ULONG mDisplay(struct IClass *cl, Object *obj, struct MUIP_List_Display *msg)
{
	#define DISPLAYTMP_SIZEOF 40
	static TEXT tmp[ DISPLAYTMP_SIZEOF ];
	struct PanelItem *pi = msg->entry;

//	if( !pi->pi_Image && pi->pi_BitMap ) /* no image, but bitmap -> create image */
	{
	#warning
//		pi->pi_Image = create_image( obj, pi->pi_BitMap ); /* XXX is it a good idea to create stuff in list display method? (geit) */
	}

	/*if( pi->pi_Image )
	{
		snprintf( tmp, sizeof( tmp ), "\033O[%08lx] %s", (ULONG) pi->pi_Image, pi->pi_DisplayName );
	} else*/ {
		snprintf( tmp, sizeof( tmp ), "%s", pi->pi_DisplayName );
	}

	msg->array[0] = tmp;

	
	return 0;
}


static ULONG mDestruct(struct IClass *cl, Object *obj, struct MUIP_List_Destruct *msg)
{
	struct PanelItem *pi = msg->entry;
//	kprintf("classlist destruct %x\n",pi);
	if(pi->pi_Object)
	{
		MUI_DisposeObject(pi->pi_Object);
		pi->pi_Object = 0;
	}
	return DoSuperMethodA(cl,obj,msg);
}

static ULONG mExternalObject(struct IClass *cl, Object *obj, struct MP_ClassListGetExternalObject *msg)
{
	ULONG i = 0;
	static struct PanelItem *pi;
	do
	{
		DoMethod(obj,MUIM_List_GetEntry,i,(APTR*)&pi);
		if((pi)&&(pi->pi_Type == MV_Panel_Type_External))
		{	
			PDB(("eob %s %s\n",pi->pi_DisplayName,pi->pi_ClassName));
			if(!strcmp(msg->class_name,pi->pi_ClassName))
			{
				*msg->panel_item = (APTR)pi;
				return TRUE;
			}
		}			
		i++;
	}while(pi);
	return 0;
}

DISPATCHER(ClassList)
{
 	switch (msg->MethodID)
	{
		case OM_NEW               				: return(mNew    			(cl,obj,(struct opSet *)msg));
		case OM_GET			    				: return(mGet				(cl,obj,(struct opGet *)msg));
		case MUIM_List_Display					: return(mDisplay			(cl,obj,(struct MUIP_List_Display*)msg));
		case MUIM_List_Destruct					: return(mDestruct 			(cl,obj,(struct MUIP_List_Destruct*)msg));
		case MM_ClassListGetExternalObject		: return(mExternalObject	(cl,obj,(struct MP_ClassListGetExternalObject*)msg));
	//	case OM_DISPOSE           				: return(mDispose		(cl,obj,(Msg)msg));
	
	}
	return(DoSuperMethodA(cl,obj,msg));
}
DISPATCHER_END


struct MUI_CustomClass *ClassList_Class(void)
{
	return MUI_CreateCustomClass(NULL,MUIC_List,NULL,sizeof(struct Data),DISPATCHER_REF(ClassList));
}