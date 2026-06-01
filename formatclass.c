/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2006 Ambient Open Source Team
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 * $Id: formatclass.c,v 1.15 2026/05/05 02:40:37 cyfm Exp $
 */

#include "ambient.h"

/* public */
#include <dos/dostags.h>
#include <proto/dos.h>

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "prefs.h"
#include "threads.h"
#include "device_func.h"
#include "capacity.h"
#include "format.h"
#include "methodstack.h"
#include "dostype.h"
#include "doslistcache.h"
#include "file_func.h"
#include "smartreq.h"


struct Data {
	APTR grp_flags;
	APTR spc;
	APTR grp_specific;
	APTR chk_icon;
	APTR bt_format;
	APTR bt_quick;
	APTR bt_verify;
	APTR str_name;
	APTR grp_general;
	APTR grp_status;
	APTR txt_fs;
	APTR txt_status;
	APTR txt_volstat;
	APTR gauge_progress;
	struct device_info *di;
	ULONG standalone;
	/* FFS */
	APTR chk_international;
	/* SFS */
	APTR chk_case;
	APTR chk_recycled;
	APTR chk_show_recycled;
	
	/* computation */
	ULONG total;

	/* options */
	ULONG formattable;
};


DEFNEW
{
	struct Data *data;
	struct TagItem *ti;
	APTR str_name;
	APTR grp_flags, grp_status;
	APTR chk_icon;
	APTR grp_general;
	APTR grp_spc;
	APTR spc;
	APTR txt_status, txt_fs, txt_volstat;

	ti = FindTagItem(MA_Window_Path, INITTAGS);

	obj = DoSuperNew(cl, obj,
		
		Child, ColGroup(2),
			Child, NSLabel2( MSG_FORMAT_LABEL ),
			Child, str_name = StringObject,
				StringFrame,
				MUIA_ShortHelp, GSI(MSG_FORMAT_LABEL_HELP),
				MUIA_String_Reject, "/:",
				MUIA_String_MaxLen, 30,
				MUIA_CycleChain, 1,
				MUIA_ControlChar, MUIGetUnderScore( MSG_FORMAT_LABEL ),
			End,

			Child, NSLabel1( MSG_FORMAT_FILESYSTEM ),
			Child, txt_fs = TextObject,
				TextFrame,
				MUIA_Background, MUII_TextBack,
				MUIA_ShortHelp, GSI( MSG_FORMAT_FILESYSTEM_HELP ),
			End,

			Child, NSLabel1(MSG_FORMAT_STATUS),
			Child, txt_volstat = MUICreateTextNoFrame( MSG_FORMAT_STATUS, NULL),
			//text2(NULL),

		End,

		Child, grp_spc = VGroup,

			Child, grp_flags = HGroup,

				Child, HSpace(0),

				Child, grp_general = ColGroup(2), GroupFrameT(GSI(MSG_FORMAT_GENERAL_GROUP)),

					Child, ColGroup(2),
						Child, chk_icon = MUICreateCheckbox( MSG_FORMAT_CREATE_ICON, FALSE, "FORMCRIC"),
						Child, MUICreateLabel( MSG_FORMAT_CREATE_ICON, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
					End,

				End,

				Child, spc = HSpace(0),

			End,

		End,

		Child, grp_status = VGroup,

			Child, txt_status = TextObject,
				TextFrame,
				MUIA_Background, MUII_TextBack,
				MUIA_Text_PreParse, "\33c",
			End,
		End,

	End;

	if (!obj)
	{
		return (0);
	}

	data = INST_DATA(cl, obj);
	data->grp_flags = grp_flags;
	data->spc = spc;
	data->chk_icon = chk_icon;
	data->str_name = str_name;
	data->grp_general = grp_general;
	data->grp_status = grp_status;
	data->txt_status = txt_status;
	data->txt_fs = txt_fs;
	data->txt_volstat = txt_volstat;

	/* XXX: add a method to update that.. */
	if (ti && ti->ti_Data)
	{
		ULONG isdevice = FALSE;
		TEXT t[VOLUME_SIZE];
		struct device_info *di;

		data->standalone = TRUE;

		if(isdevicename((STRPTR) ti->ti_Data))
		{
			stccpy(t, (STRPTR)ti->ti_Data, strlen((STRPTR)ti->ti_Data)); /* remove ':' */
			set(data->str_name, MUIA_String_Contents, t); /* XXX: redundant ? */
			strcat(t, ":"); /* add it back */

			if ( (di = deviceinfo_build(t, TRUE)) )
			{
				isdevice = TRUE;
				DoMethod(obj, MM_Format_ChangeOptions, di); /* XXX: free data->di within that ChangeOptions */
			}
		}

		if(!isdevice)
		{
			smartreq_info(GSI(MSG_FORMAT_ERROR), MV_Notification_Error, GSI(MSG_FORMAT_ERROR_CANNOT), NULL);
			CoerceMethod(cl, obj, OM_DISPOSE);
			return (0);
		}
	}
	else
	{
		APTR sp1, sp2;

		/*
		 * Adjust spacing.
		 */
		sp1 = VSpace(0);
		sp2 = VSpace(0);

		if (sp1 && sp2)
		{
			DoMethod(grp_spc, OM_ADDMEMBER, sp1);
			DoMethod(grp_spc, OM_ADDMEMBER, sp2);
			DoMethod(grp_spc, MUIM_Group_Sort, sp1, grp_flags, sp2, NULL);
		}
		else
		{
			if (sp1)
				MUI_DisposeObject(sp1);

			if (sp2)
				MUI_DisposeObject(sp2);
		}
	}
	return ((ULONG)obj);
}


DEFDISPOSE
{
	GETDATA;

	if (data->standalone)
	{
		deviceinfo_delete(data->di);
	}
	return (DOSUPER);
}


DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_Format_NameStatus:
			{
				{
					STRPTR s = (STRPTR)getv(data->str_name, MUIA_String_Contents);

					if (!(*s))
					{
						*msg->opg_Storage = MV_Format_NameStatus_Empty;
					}
					else
					{
						#if USE_LEGACY
						*msg->opg_Storage = MV_Format_NameStatus_Valid;
						#else
						CONST_STRPTR *badname;
						ULONG rc = FALSE;


						for (badname = DOSBase->dl_BannedDeviceNames; *badname; badname++)
						{
							if (!stricmp(*badname, s))
							{
								rc = TRUE;
								break;
							}
						}

						if (!rc)
						{
							for (badname = DOSBase->dl_BannedVolumeNames; *badname; badname++)
							{
								if (!stricmp(*badname, s))
								{
									rc = TRUE;
									break;
								}
							}
						}

						if (rc)
						{
							*msg->opg_Storage = MV_Format_NameStatus_Reserved;
						}
						else
						{
							*msg->opg_Storage = MV_Format_NameStatus_Valid;
						}
						#endif
					}
				}
				return (TRUE);
			}

		case MA_Format_DeviceInfo:
			*msg->opg_Storage = (ULONG)data->di;
			return (TRUE);

		case MA_Format_Formattable:
			*msg->opg_Storage = data->formattable;
			return (TRUE);
	}
	return (DOSUPER);
}


DEFSMETHOD(Format_ChangeOptions)
{
	GETDATA;
	UQUAD sz;
	TEXT t[22];
	struct dlcnode *dlcn;
	CONST_STRPTR s;

	if (data->standalone)
	{
		deviceinfo_delete(data->di);
	}

	data->di = msg->di;

	if (!data->standalone)
	{
		if ( (dlcn = doslistcache_find_dlcvolume_by_devicename(msg->di->name)) )
		{
			set(data->str_name, MUIA_String_Contents, dlcn->name);
			set(data->txt_volstat, MUIA_Text_Contents, GSI(MSG_FORMAT_FORMATTED));
		}
		else
		{
			set(data->str_name, MUIA_String_Contents, "");
			set(data->txt_volstat, MUIA_Text_Contents, GSI(MSG_FORMAT_UNFORMATTED));
		}
	}
	else
	{
		set(data->txt_volstat, MUIA_Text_Contents, GSI(MSG_FORMAT_FORMATTED));
	}

	dlcn = doslistcache_find_dlcdevice_by_devicename(msg->di->name);
	data->formattable = (!dlcn || dlcn->medium == DLC_MEDIUM_CDROM) ? 0 : 1;

	sz = (UQUAD)data->di->surfaces * data->di->blocksize * data->di->blockspertrack * (data->di->highcyl - data->di->lowcyl + 1);
	capacity_format_size(t, sizeof(t), sz);

	DoMethod(data->txt_status, MUIM_SetAsString, MUIA_Text_Contents, "%s, unit %ld, %s", data->di->devname, data->di->unit, t);

	if ( (s = dostype_get(msg->di->dostype)) )
	{
		set(data->txt_fs, MUIA_Text_Contents, s);
	}
	else
	{
		TEXT dostype[8];
			
		dostype_to_str(msg->di->dostype, dostype);
		DoMethod(data->txt_fs, MUIM_SetAsString, MUIA_Text_Contents, "Custom FS 0x%08lx <%s>", msg->di->dostype, dostype);
	}

	DoMethod(data->grp_flags, MUIM_Group_InitChange);

	if (data->grp_specific)
	{
		DoMethod(data->grp_flags, OM_REMMEMBER, data->grp_specific);
		MUI_DisposeObject(data->grp_specific);
		data->grp_specific = NULL;
		data->chk_international = NULL;
		data->chk_case = NULL;
		data->chk_recycled = NULL;
		data->chk_show_recycled = NULL;
	}

	DoMethod(data->grp_flags, OM_REMMEMBER, data->spc);

	switch (msg->di->dostype)
	{
		case FS_AMIGA_SFS:
		case FS_AMIGA_SFS2:
			if ( (data->grp_specific = ColGroup(2), GroupFrameT( GSI( MSG_FORMAT_SPECIFIC_GROUP )),
					Child, data->chk_case = MUICreateCheckbox( MSG_FORMAT_CASE_SENSITIVE, getprefslong(DSI_FORMAT_SFS_CASE), "FORMCASE"),
					Child, MUICreateLabel( MSG_FORMAT_CASE_SENSITIVE, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
					Child, data->chk_recycled = MUICreateCheckbox( MSG_FORMAT_RECYCLED, getprefslong(DSI_FORMAT_SFS_RECYCLED), "FORMRECY"),
					Child, MUICreateLabel( MSG_FORMAT_RECYCLED, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
					Child, data->chk_show_recycled = MUICreateCheckbox( MSG_FORMAT_SHOWRECYCLED, getprefslong(DSI_FORMAT_SFS_SHOWRECYCLED), "FORMSHRE"),
					Child, MUICreateLabel( MSG_FORMAT_SHOWRECYCLED, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
					TAG_DONE)) )
			{
				DoMethod(data->grp_flags, OM_ADDMEMBER, data->grp_specific);
			}
			break;

		default:
			break;
	}

	DoMethod(data->grp_flags, OM_ADDMEMBER, data->spc);

	DoMethod(data->grp_flags, MUIM_Group_ExitChange);
	
	return (0);
}


DEFSMETHOD(Format_Format)
{
	GETDATA;
	ULONG flags = 0;

	if (msg->mode != MV_Format_Format_Quick)
	{
		data->total = data->di->highcyl - data->di->lowcyl + 1;

		if ( (data->gauge_progress = GaugeObject,
			GaugeFrame,
			MUIA_Gauge_Horiz, TRUE,
			MUIA_Gauge_InfoText, GSI(MSG_FORMAT_PREPARING),
			MUIA_Gauge_Max, data->total,
			End) )
		{
			DoMethod(data->grp_status, MUIM_Group_InitChange);
			DoMethod(data->grp_status, OM_REMMEMBER, data->txt_status);
			DoMethod(data->grp_status, OM_ADDMEMBER, data->gauge_progress);
			DoMethod(data->grp_status, OM_ADDMEMBER, data->txt_status);
			DoMethod(data->grp_status, MUIM_Group_ExitChange);
		}
		else
		{
			return (0); /* XXX */
		}
	}

	if (getv(data->chk_icon, MUIA_Selected))
	{
		flags |= FF_ICON;
	}
	if (data->chk_case && getv(data->chk_case, MUIA_Selected))
	{
		flags |= FF_SFS_CASE;
	}
	if (data->chk_recycled && getv(data->chk_recycled, MUIA_Selected))
	{
		flags |= FF_SFS_RECYCLED;
	}
	if (data->chk_show_recycled && getv(data->chk_show_recycled, MUIA_Selected))
	{
		flags |= FF_SFS_SHOWRECYCLED;
	}

	do_action(obj, TA_Disk_Format,
		TT_Object, _win(obj),
		TT_Disk_Format_Device, getv(data->str_name, MUIA_String_Contents),
		TT_Disk_Format_DeviceInfo, data->di,
		TT_Disk_Format_Mode, msg->mode,
		TT_Disk_Format_FileSystem, data->di->dostype,
		TT_Disk_Format_Flags, flags,
	TAG_DONE);
	
	return (0);
}


DEFSMETHOD(Formatwin_SetText)
{
	GETDATA;

	ASSERT(msg->txt);

	set(data->txt_status, MUIA_Text_Contents, msg->txt);

	return (0);
}


DEFSMETHOD(Formatwin_SetGauge)
{
	GETDATA;

	if (msg->val)
	{
		TEXT t[32];

		sprintf(t, "%llu.%llu%%%%", (UQUAD)msg->val * 100 / data->total, (UQUAD)msg->val * 100 % data->total * 10 / data->total);

		SetAttrs(data->gauge_progress, MUIA_Gauge_Current, msg->val, MUIA_Gauge_InfoText, t, TAG_DONE);
	}
	return (0);
}


DEFTMETHOD(Format_RemoveGauge)
{
	GETDATA;

	if (data->gauge_progress)
	{
		methodstack_kill_methods(data->gauge_progress); /* XXX: that sucks.. */
		DoMethod(data->grp_status, MUIM_Group_InitChange);
		DoMethod(data->grp_status, OM_REMMEMBER, data->gauge_progress);
		MUI_DisposeObject(data->gauge_progress);
		data->gauge_progress = NULL;
		DoMethod(data->grp_status, MUIM_Group_ExitChange);
	}
	return (0);
}


DEFSMETHOD(Format_Busy)
{
	GETDATA;

	set(data->grp_general, MUIA_Disabled, msg->sleep);
	if (data->grp_specific)
	{
		set(data->grp_specific, MUIA_Disabled, msg->sleep);
	}
	set(data->str_name, MUIA_Disabled, msg->sleep);

	return (0);
}


BEGINMTABLE
DECNEW
DECDISPOSE
DECGET
DECSMETHOD(Format_ChangeOptions)
DECSMETHOD(Format_Format)
DECSMETHOD(Formatwin_SetText)
DECSMETHOD(Formatwin_SetGauge)
DECTMETHOD(Format_RemoveGauge)
DECSMETHOD(Format_Busy);
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, formatclass)

