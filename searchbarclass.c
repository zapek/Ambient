/*
 * $Id: searchbarclass.c,v 1.6 2017/07/30 21:06:21 cyfm Exp $
 */

#include "ambient.h"

/* private */
#include "mui_func.h"
#include "ambient_cat.h"
  
struct Data {
	APTR str_search;
	APTR label;
	APTR bt_next;
	APTR bt_prev;
	APTR bt_close;
	APTR chk_sensitive;
	APTR obj_target;

	ULONG flags;
	ULONG notifysetup;
	ULONG autohide;
};


DEFNEW
{
	APTR bt_close, str_search, lblNotFound, chk_sensitive = NULL;
	APTR sp_beforecase;
	//TEXT buffer[128];

	obj = DoSuperNew(cl, obj,
		GroupFrame, MUIA_Background, MUII_GroupBack,
		MUIA_Group_Horiz, TRUE,

		Child, bt_close = TinyButton("\33I[4:PROGDIR:images/findclose.mbr]"),
		Child, RectangleObject, MUIA_Rectangle_VBar, TRUE, MUIA_Weight, 0, End,
		Child, NSLabel( MSG_SEARCHBARCLASS_SEARCH ),
		Child, str_search = NewObject(getsearchstringclass(), NULL, MUIA_ControlChar, MUIGetUnderScore( MSG_SEARCHBARCLASS_SEARCH ),
																	MUIA_ShortHelp  , GSI( MSG_SEARCHBARCLASS_SEARCH_HELP ),
																	TAG_DONE),
		Child, sp_beforecase = HSpace(6),
		Child, HSpace(6),
		Child, lblNotFound = TextObject,
			MUIA_Text_Contents, "",
			MUIA_Text_PreParse, "\338",
			MUIA_FixWidthTxt, GSI( MSG_SEARCHBARCLASS_TEXTNOTFOUND ),
		End,


		TAG_MORE, INITTAGS
	);

	if (obj)
	{
		struct Data *data;

		data = INST_DATA(cl, obj);

		data->str_search    = str_search;
		data->label         = lblNotFound;
		data->chk_sensitive = chk_sensitive;
		data->bt_next       = NULL;
		data->bt_prev       = NULL;
		data->bt_close      = bt_close;
		data->obj_target    = (APTR)GetTagData( MA_Searchbar_Target, (ULONG)NULL, INITTAGS );
		data->flags         =       GetTagData( MA_Searchbar_Flags, 0xff, INITTAGS );
		data->autohide      =       GetTagData( MA_Searchbar_AutoHide, TRUE, INITTAGS );

		/* add buttons depending on flags */

		if ( data->flags & MV_Searchbar_Flags_ShowPrev )
		{
			//sprintf(buffer, "\33I[4:PROGDIR:images/findprev.mbr] %s", GSI( MSG_SEARCHBARCLASS_PREV) );
			data->bt_prev = TinyButton( "\33I[4:PROGDIR:images/findprev.mbr]" ); // bitRocky: the image should be enough not text required!
			if ( data->bt_prev )
			{
				DoMethod( obj, MUIM_Family_Insert, data->bt_prev, data->str_search );
				set(data->bt_prev, MUIA_Disabled, TRUE);
				set(data->bt_prev, MUIA_ShortHelp, "Hotkey: F2");
			}
		}
		if ( data->flags & MV_Searchbar_Flags_ShowNext )
		{
			//sprintf(buffer, "\33I[4:PROGDIR:images/findnext.mbr] %s", GSI( MSG_SEARCHBARCLASS_NEXT) );
			data->bt_next = TinyButton( "\33I[4:PROGDIR:images/findnext.mbr]" ); // bitRocky: the image should be enough not text required!
			if ( data->bt_next )
			{
				DoMethod( obj, MUIM_Family_Insert, data->bt_next, data->bt_prev ? data->bt_prev : data->str_search );
				set(data->bt_next, MUIA_Disabled, TRUE);
				set(data->bt_next, MUIA_ShortHelp, "Hotkey: F3");
			}
		}
		if ( data->flags & MV_Searchbar_Flags_ShowCase )
		{
			data->chk_sensitive = MUICreateCheckbox( MSG_SEARCHBARCLASS_CASESENSITIVE, FALSE, "" );
			if ( data->chk_sensitive )
			{
				DoMethod( obj, MUIM_Family_Insert, data->chk_sensitive, sp_beforecase );
				DoMethod( obj, MUIM_Family_Insert, NSLabel1(MSG_SEARCHBARCLASS_CASESENSITIVE), sp_beforecase);
			}
		}
	}

	return ((ULONG)obj);
}

DEFMMETHOD(Setup)
{
	ULONG rc;
	rc = DOSUPER;
	if (rc)
	{
		GETDATA;

		if ( !data->notifysetup )
		{
			DoMethod(obj, MUIM_Notify, MUIA_ShowMe, TRUE, _win(obj), 3, MUIM_Set, MUIA_Window_ActiveObject, data->str_search);
			DoMethod(obj, MUIM_Notify, MUIA_ShowMe, TRUE, data->str_search, 3, MUIM_Set, MUIA_String_Contents, "");

			DoMethod(data->str_search, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 2, MM_Searchbar_Search, MV_Search_SearchCurrent);

			if ( data->autohide )
				DoMethod(data->str_search, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, obj, 3, MUIM_Set, MUIA_ShowMe, FALSE);

			if ( data->bt_next != NULL )
				DoMethod(data->bt_next, MUIM_Notify, MUIA_Pressed, FALSE, obj, 2, MM_Searchbar_Search, MV_Search_SearchNext);

			if ( data->bt_prev != NULL )
				DoMethod(data->bt_prev, MUIM_Notify, MUIA_Pressed, FALSE, obj, 2, MM_Searchbar_Search, MV_Search_SearchPrev);

			DoMethod(data->bt_close, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3, MUIM_Set, MUIA_ShowMe, FALSE);
			data->notifysetup = TRUE;
		}

	}
	return (rc);
}


DEFSMETHOD(Searchbar_Search)
{
	ULONG casesensitive;
	STRPTR str;
	GETDATA;

	set(data->label, MUIA_Text_Contents, "");

	casesensitive = data->chk_sensitive != NULL ? getv(data->chk_sensitive, MUIA_Selected) : FALSE;
	str = (STRPTR)getv(data->str_search, MUIA_String_Contents);

	if ( data->bt_next != NULL )
		set(data->bt_next, MUIA_Disabled, !(str && *str));
	if ( data->bt_prev != NULL )
		set(data->bt_prev, MUIA_Disabled, !(str && *str));

	if ( msg->direction == MV_Search_SearchNext && data->bt_next == NULL )
		return 0;
	if ( msg->direction == MV_Search_SearchPrev && data->bt_prev == NULL )
		return 0;

	if ( str != NULL && *str && data->obj_target!= NULL )
	{
		ULONG res = DoMethod( data->obj_target, MM_Search, str, msg->direction, casesensitive);
		set(data->label, MUIA_Text_Contents, res ? (STRPTR) "" : GSI( MSG_SEARCHBARCLASS_TEXTNOTFOUND ) );

	}
	return (0);
}

DEFSET
{
	GETDATA;

	FORTAG(INITTAGS)
	{
		case MA_Searchbar_Target:
			data->obj_target = (APTR)tag->ti_Data;
			break;
	}
	NEXTTAG;

	return (DOSUPER);
}


BEGINMTABLE
DECNEW
DECSET
DECMMETHOD(Setup)
DECSMETHOD(Searchbar_Search)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, searchbarclass)
