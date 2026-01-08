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
	APTR targetobject;
	APTR app_object;
	struct MsgPort *msgport;
	ULONG callbackmode;
	BOOL callback_enabled;
	struct PanelMessage *messages;
	ULONG message_ct;
};

/************************************************************************/

static void doset( APTR obj, struct Data *data, struct TagItem *tags )
{
	
	FORTAG( tags )
	{
		case MA_Panelmessenger_PanelObject:
			data->targetobject = (APTR)tag->ti_Data;
			if(data->targetobject) SetAttrs(data->targetobject,MA_Panel_Messenger,obj,TAG_DONE);	
			break;
		case PanelObject_App:
			data->app_object = (APTR)tag->ti_Data;
			break;
		case PanelObject_MsgPort:
			data->msgport = (struct MsgPort*)tag->ti_Data;
			data->message_ct = 0;
			if(data->messages == NULL)
			{
				PDB(("%d %d\n",sizeof(struct PanelMessage) * (PANEL_MESSAGE_COUNT+1),sizeof(struct PanelMessage) * PANEL_MESSAGE_COUNT+1));
				data->messages = (struct PanelMessage *) AllocTaskPooled(sizeof(struct PanelMessage) * (PANEL_MESSAGE_COUNT+1));
			}
			if(data->messages) 
			{
				for(ULONG i = 0;i <= PANEL_MESSAGE_COUNT;i++)
				{
					data->messages[i].msg.mn_Node.ln_Type = NT_FREEMSG; 
					data->messages[i].msg.mn_ReplyPort = NULL; 
					data->messages[i].msg.mn_Length = sizeof(struct PanelMessage);
				}
			}
		case PanelObject_CallbackMode:
			data->callbackmode = tag->ti_Data;
			if ((! data->callback_enabled ) && ( data->callbackmode))
			{
				data->callback_enabled = TRUE;
			}
			break;
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
	data->app_object = NULL;
	data->msgport = NULL;
	data->messages = NULL;
	data->callback_enabled = FALSE;
	
	doset( obj, data, INITTAGS );
	return( (ULONG) obj );
}

/************************************************************************/

DEFDISP 
{
	GETDATA;
	if(data->targetobject) SetAttrs(data->targetobject,MA_Panel_Messenger,NULL,TAG_DONE);
	
	return( DOSUPER );
}


DEFGET
{
	GETDATA;
	ULONG result = TRUE;

	switch( msg->opg_AttrID )
	{
		case MA_Panelmessenger_PanelObject:
			PDB(("!!!!!!!!!!!!!!!!!!!!!!!! %x !!!!!!!!!!!!!!!!!!!\n",data->targetobject));
			*msg->opg_Storage = (ULONG) data->targetobject;
			break;
	}
	return( result );
}

/************************************************************************/

DEFSET
{
	GETDATA;

	doset( obj, data, INITTAGS );
	PDB(("%x \n",data->targetobject));
	DoMethod(obj,MM_Panelbutton_Callback,INITTAGS );
	if( data->targetobject )DoMethodA(data->targetobject,msg);
	return( DOSUPER );
}





DEFSMETHOD(Panelbutton_GetAttr)
{
	APTR group;
	GETDATA;
	//PDB(("getattr %d\n",msg->target));
	if(! data->targetobject ) return 0;
	switch ( msg->attrID)
	{
		case MA_AmbientPanel_Group_Size:
		case MA_AmbientPanel_Group_Horiz:
		case MA_Panelgroup_Locked:
		case MA_Panelgroup_Zipping:
		case MA_Panelgroup_Zipped:
			if(GetAttr(MUIA_Parent,data->targetobject,(ULONG*)&group) && (group))
			{
				BOOL ret = GetAttr(msg->attrID,group,msg->storage);
				PDB(("getattr %x %d %d\n",msg->storage,*msg->storage,ret));
				return ret;
			}
			break;
		default:
			return GetAttr(msg->attrID,data->targetobject,msg->storage);
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



static void sendmessage(struct Data *data,struct TagItem *tag)
{
	if (data->messages[data->message_ct].msg.mn_Node.ln_Type == NT_FREEMSG)
	{
		data->messages[data->message_ct].msg.mn_Node.ln_Type = NT_MESSAGE;
		data->messages[data->message_ct].tag = tag->ti_Tag;
		data->messages[data->message_ct].data = tag->ti_Data;
		PutMsg(data->msgport,(struct Message*)&data->messages[data->message_ct]);
		data->message_ct++;
		if(data->message_ct == PANEL_MESSAGE_COUNT) data->message_ct = 0;
	}
	else
	{
		if (data->messages[PANEL_MESSAGE_COUNT].msg.mn_Node.ln_Type == NT_FREEMSG)
		{
			data->messages[PANEL_MESSAGE_COUNT].msg.mn_Node.ln_Type = NT_MESSAGE;
			data->messages[PANEL_MESSAGE_COUNT].tag = MA_Panelbutton_MsgPortJammed;
			data->messages[PANEL_MESSAGE_COUNT].data = 0;
			PutMsg(data->msgport,(struct Message*)&data->messages[PANEL_MESSAGE_COUNT]);
		}
	}
}

DEFSMETHOD(Panelbutton_Callback)
{
	GETDATA;
	struct TagItem *tstate = msg->ops_AttrList, *tag;
	APTR destObj = NULL;
	if(data->callbackmode == PanelObject_CallbackMode_None ) return 0;
	if ( ( data->callbackmode == PanelObject_CallbackMode_MUIApp ) ) destObj = data->app_object;
	while ( (tag = (struct TagItem *) NextTagItem(&tstate) ) )
	{
		switch (tag->ti_Tag)
		{	
			case MA_Panel_Highlighted:
			case MA_AmbientPanel_Group_Size:
			case MA_AmbientPanel_Group_Horiz:
			case MA_Panelgroup_Locked:
			case MA_Panelgroup_Zipping:
			case MA_Panelgroup_Zipped:
				if ( ( data->callbackmode == PanelObject_CallbackMode_MsgPort) && (data->msgport)&&(data->messages))
				{
					sendmessage(data,tag);
				}
				else if ( ( data->app_object ) && ( destObj ) )
				{
					PDB(("%x %x\n",data->app_object,destObj));
					DoMethod(data->app_object,MUIM_Application_PushMethod,destObj,3,MUIM_Set,tag->ti_Tag,tag->ti_Data);
				}
				break;
		}
	}
	return 0;
}

DEFTMETHOD(Panelmessenger_PanelObjectDisposed)
{
	GETDATA;
	data->targetobject = NULL;
	return 0;
}

/************************************************************************/

BEGINMTABLE

DECNEW
DECGET
DECSET
DECDISPOSE
DECSMETHOD(Panelbutton_GetAttr)
DECSMETHOD(Panelbutton_Callback)
DECTMETHOD(Panelmessenger_PanelObjectDisposed)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Notify, panelmessengerclass)

