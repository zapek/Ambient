#include "ambient.h"

/* public */
#include <cybergraphx/cybergraphics.h>
#include <datatypes/pictureclass.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <clib/datatypes_protos.h>
#include <proto/timer.h>
#include <graphics/rpattr.h>


/* private */
#include "rexx.h"
#include "command.h"
#include "contextmenu.h"
#include "iconio.h"
#include "name.h"       
#include "prefs.h"
#include "wbstart.h"
#include "threads.h"
#include "file_func.h"
#include "dragdrop.h"
#include "legacy.h"
#include "paneltags.h"

#include "deficonpool.h"
#include "methodstack.h"
#include "viewwatcher_arrow_logo.h"

#define USE_INLINE_STDARG
 
/************************************************************************/

struct Data {
	APTR  object;
	APTR  pctx;
	APTR  pi;
	BOOL  saving;
};

/************************************************************************/

static void doset( APTR obj UNUSED, struct Data *data, struct TagItem *tags )
{
	FORTAG( tags )
	{
		case MA_Panelsupport_Object:
			data->object = (APTR) tag->ti_Data;
			break;
		case MA_Panelsupport_PPool:
			data->pctx = (APTR) tag->ti_Data;
			break;
		case MA_Panelsupport_PItem:
			data->pi = (APTR) tag->ti_Data;
			break;
	}
	NEXTTAG
}

/************************************************************************/

DEFNEW
{
	if( ( obj = (Object*) DoSuperNew( cl, obj, TAG_MORE, msg->ops_AttrList ) ) )
	{
		struct Data *data = (struct Data*) INST_DATA( cl, obj );

		doset( obj, data, INITTAGS );

		data->saving = FALSE;
	}
	return( (ULONG)obj );
}
	
/************************************************************************/

DEFGET
{
#if 0
	switch( msg->opg_AttrID )
	{
		case MA_Panel_:
			{
				GETDATA;
				*msg->opg_Storage = data->something;
			}
			return( TRUE );
      
	}
#endif
	return( DOSUPER );
}

/************************************************************************/

DEFSET
{
	GETDATA;

	doset( obj, data, INITTAGS );

	return( DOSUPER );
}

/************************************************************************/

DEFSMETHOD(Panelsupport_WritePrefsStr)
{
	GETDATA;

	if( data->saving ) {
		setprefsstr_lp( data->pctx, data->pi, DSI_PANEL_CLASSES + msg->ID, msg->value );
	}
	return( 0 );
}

/************************************************************************/

DEFSMETHOD(Panelsupport_WritePrefsLong)
{
	GETDATA;

	if( data->saving ) {
		setprefslong_lp( data->pctx, data->pi, DSI_PANEL_CLASSES + msg->ID, msg->value );
	}
	return( 0 );
}

/************************************************************************/

DEFSMETHOD(Panelsupport_ReadPrefsStr)
{
	GETDATA;

	return(	(ULONG) prefspool_item_get( data->pctx, data->pi, DSI_PANEL_CLASSES + msg->ID, (APTR) msg->storage, NULL ) );
}

/************************************************************************/

DEFSMETHOD(Panelsupport_ReadPrefsLong)
{
	GETDATA;
	ULONG *storage;
	ULONG res;

	res = (ULONG) prefspool_item_get( data->pctx, data->pi, DSI_PANEL_CLASSES + msg->ID, (APTR) &storage, NULL );
	*msg->storage = *storage;

	return( res );
}

/************************************************************************/

DEFSMETHOD(Panelsupport_Saveconfig)
{
	GETDATA;

	data->pctx   = msg->pctx;
	data->pi     = msg->pi;
	data->saving = TRUE;

	DoMethod( data->object, MM_AmbientPanel_SaveConfig,0,0);

	data->saving = FALSE;

	return( 0 );
}

/************************************************************************/

BEGINMTABLE

DECNEW
DECGET
DECSET
DECSMETHOD(Panelsupport_WritePrefsStr)
DECSMETHOD(Panelsupport_WritePrefsLong)
DECSMETHOD(Panelsupport_ReadPrefsStr)
DECSMETHOD(Panelsupport_ReadPrefsLong)
DECSMETHOD(Panelsupport_Saveconfig)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Notify, panelexternalsupportclass)
