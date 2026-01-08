#include "ambient.h"

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
#include "panellib.h"

#include "deficonpool.h"
#include "methodstack.h"
#include "deficon_getpath.h"
#include "viewwatcher_arrow_logo.h"

/************************************************************************/


#define PANEL_MESSAGE_COUNT 10 

/************************************************************************/

struct Data {
	APTR panelobject;
};

/************************************************************************/

static void doset( APTR obj, struct Data *data, struct TagItem *tags )
{
	
	FORTAG( tags )
	{
	/*	MA_Panelmessenger_PanelObject:
			data->panelobject = (APTR)tag->ti_Data;
			PDB(("%\n",tag->ti_Data));
		break;*/
	}
	NEXTTAG
}

/************************************************************************/
DEFNEW
{
	struct Data *data;
	obj = (Object*) DoSuperNew( cl, obj,
			
			TAG_MORE, INITTAGS
			);
	data = (struct Data*) INST_DATA( cl, obj );

	
	doset( obj, data, INITTAGS );
	return( (ULONG) obj );
}

/************************************************************************/

DEFDISP 
{
	GETDATA;

	
	return( DOSUPER );
}


DEFGET
{
	GETDATA;
	ULONG result = TRUE;

	switch( msg->opg_AttrID )
	{
		
	}
	return( result );
}

/************************************************************************/

DEFSET
{
	GETDATA;

	doset( obj, data, INITTAGS );
	DoMethod(obj,MM_Panelbutton_Callback,INITTAGS );
	return( DOSUPER );
}



DEFSMETHOD(Panelmessengerfamily_NewObject)
{
	GETDATA;
	APTR messenger_obj;
	PDB(("....................%x\n",msg->target));	
	if((messenger_obj = NewObject(getpanelmessengerclass(),NULL,MA_Panelmessenger_PanelObject,msg->target,TAG_DONE)))
	{
		DoMethod(obj,MUIM_Family_AddTail,messenger_obj);
		return messenger_obj;
	}
	return 0;
}

DEFSMETHOD(Panelmessengerfamily_RemObject)
{
	GETDATA;
	DoMethod(obj,MUIM_Family_Remove,msg->object);
	return 0;
}

/************************************************************************/

BEGINMTABLE

DECNEW
DECGET
DECSET
DECDISPOSE
DECSMETHOD(Panelmessengerfamily_NewObject)
DECSMETHOD(Panelmessengerfamily_RemObject)
ENDMTABLE

DECSUBCLASS_NC("Family.mui", panelmessengerfamilyclass)

//DECSUBCLASS_NC(MUIC_Family, panelmessengerfamilyclass)

