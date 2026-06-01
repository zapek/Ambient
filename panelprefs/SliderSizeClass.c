
#define CATCOMP_NUMBERS
extern const char * const __stringtable[];
#ifdef __SASC
#define GSI(x) __stringtable[x]
#else
#define GSI(x) ( STRPTR )__stringtable[x]
#endif

#include "muifuncs.h"

#include "locale.h"
#include "panelprefs_cat.h"
//#include "debug.h"

struct Data {
	int dummy;
};


static ULONG mNew(struct IClass *cl,Object *obj,struct opSet *msg)
{
	obj = DoSuperNew(cl, obj,
		MUIA_Numeric_Min, 0,
		MUIA_Numeric_Max, 4,
		TAG_MORE		,	msg->ops_AttrList,
	TAG_DONE);

	
	return ((ULONG)obj);
}

static const char * const _sizes[] =
{
"16","24","32","48","64",NULL};

static ULONG mStringify(struct IClass *cl,Object *obj,struct MUIP_Numeric_Stringify *msg)
{
	if(  msg->value >= 0 && msg->value < 5 ) {
		return(  (ULONG) _sizes[msg->value]);//GSI( MSG_PANELSLIDERSIZECLASS_16PIXEL + msg->value ) );
	} else {
		//PDB(("wrong value\n"));
		return( 0 );
	}
	return (0);
}




DISPATCHER(SliderSizeClass)
{
 	switch (msg->MethodID)
	{
		case OM_NEW               				: return(mNew    					(cl,obj,(struct opSet *)msg));
		case MUIM_Numeric_Stringify				: return(mStringify					(cl,obj,(struct MUIP_Numeric_Stringify *)msg));	
	}
	return(DoSuperMethodA(cl,obj,msg));
}
DISPATCHER_END


struct MUI_CustomClass *SliderSize_Class(void)
{
	return MUI_CreateCustomClass(NULL,MUIC_Slider,NULL,sizeof(struct Data),DISPATCHER_REF(SliderSizeClass));
}
/*
BEGINMTABLE
DECNEW
DECMMETHOD(Numeric_Stringify)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Slider, panelslidersizeclass)
*/