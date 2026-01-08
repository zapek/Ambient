/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2016 Ambient Open Source Team
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
 * $Id: progresswinclass.c,v 1.24 2020/08/16 03:15:18 jacadcaps Exp $
 */

#include "ambient.h"

/* public */
#include <dos/dos.h>
#include <proto/exec.h>

/* private */
#include "methodstack.h"
#include "mui_func.h"
#include "ambient_cat.h"
#include "screen.h"
#include "legacy.h"
#include "prefs_advanced.h"
#include "time_func.h"
#include "capacity.h"
#include "str.h"
#include "threads.h"

struct Data {
	APTR  grp_content;
	APTR  totalgauge;
	APTR  gauge;
	APTR  bt_stop;
	APTR  txt_files;
	APTR  txt_delete;
	APTR  thread;
	ULONG gauge_shift;
	UQUAD gauge_max;
	UQUAD gauge_cur;
	UQUAD totaldone;    /* in bytes */
	UQUAD totalsize;
	UQUAD current_togo; /* how many bytes are left in the current copy */
	ULONG totalshift;
	ULONG totalfiles;
	TEXT  infotext[128];
	TEXT  totalsizetext[64];

	ULONG timestamp_start;
	ULONG timestamp_prev;
	
	ULONG disabled; // bitRocky: TRUE if bt_stop is disabled
	//ULONG buffering; // bitRocky: true while "buffering"
	//ULONG aborted; // bitRocky: set to true if user hits "Stop" button, to abort while "buffering"
};


DEFNEW
{
	struct Data *data;
	CONST_STRPTR title = title;
	APTR grp_content;
	APTR o, grp;
	ULONG mode;
	APTR refwin;

	mode = GetTagData(MA_Progresswin_Look, 0, INITTAGS);
	refwin = (APTR)GetTagData(MA_Progresswin_Refwin, 0, INITTAGS);

	obj = DoSuperNew(cl, obj,
		MUIA_Window_Screen,      get_screen(),
		MUIA_Window_CloseGadget, FALSE,
		MUIA_Window_ScreenTitle, screentitle,
		refwin ? MUIA_Window_RefWindow : TAG_IGNORE, refwin,
		MUIA_Window_LeftEdge,    refwin ? MUIV_Window_LeftEdge_Centered : MUIV_Window_LeftEdge_Moused,
		MUIA_Window_TopEdge,     refwin ? MUIV_Window_TopEdge_Centered : MUIV_Window_TopEdge_Moused,
		MUIA_Window_Width,       MUIV_Window_Width_MinMax(10),
		MUIA_Window_ID,          MAKE_ID('C','O','P','Y'),
		MUIA_Window_ShowIconify, FALSE,
		MUIA_Window_ShowPrefs,   FALSE,
		MUIA_Window_ShowJump,    FALSE,
		MUIA_Window_ShowAbout,   FALSE,
		MUIA_Window_ShowPopup,   FALSE,
		MUIA_Window_ShowSnapshot, TRUE,
		MUIA_Window_NoMenus,      TRUE,
		MUIA_Window_Activate,    _aprefs(progresswindowfocus),
		WindowContents, grp_content = VGroup,
		End,
		//TAG_MORE, INITTAGS,
	End;

	if (!obj)
	{
		return ((ULONG)NULL);
	}

	switch (mode)
	{
		case MV_Progresswin_Look_Move:
			title = GSI(MSG_PROGRESS_MOVINGFILE_TITLE);
			break;

		case MV_Progresswin_Look_MoveMany:
			title = GSI(MSG_PROGRESS_MOVINGFILES_TITLE);
			break;

		case MV_Progresswin_Look_Copy:
			title = GSI(MSG_PROGRESS_COPYINGFILE_TITLE);
			break;

		case MV_Progresswin_Look_CopyMany:
			title = GSI(MSG_PROGRESS_COPYINGFILES_TITLE);
			break;

		case MV_Progresswin_Look_Delete:
			title = GSI(MSG_PROGRESS_DELETINGFILE_TITLE);
			break;

		case MV_Progresswin_Look_DeleteMany:
			title = GSI(MSG_PROGRESS_DELETINGFILES_TITLE);
			break;
	}

	set(obj, MUIA_Window_Title, title);

	data = INST_DATA(cl, obj);
	//data->aborted = data->buffering = FALSE;
	data->grp_content = grp_content;
	data->thread = (APTR)GetTagData(MA_Progresswin_Thread, 0, INITTAGS);

	if (mode <= 3)
	{
		o = VGroup,
			Child,  data->totalgauge = GaugeObject,
				GaugeFrame,
				MUIA_Gauge_Horiz, TRUE,
				MUIA_FixHeightTxt, "/",
				MUIA_Gauge_InfoText, "",
			End,
			Child, data->gauge = GaugeObject,
				GaugeFrame,
				MUIA_Gauge_Horiz, TRUE,
				MUIA_Gauge_InfoText, GSI(MSG_PROGRESS_BUFFERING),
			End,
		End;
		//if (o) data->buffering = TRUE;
	}
	else
	{
		o = data->txt_delete = TextObject,
			//GaugeFrame,
			//MUIA_Background, MUII_BACKGROUND,
		End;
	}

	if (!o)
	{
		CoerceMethod(cl, obj, OM_RELEASE);
		return ((ULONG)NULL);
	}

	DoMethod(data->grp_content, OM_ADDMEMBER, o);

	grp	= HGroup,
		End;

	if (!grp)
	{
		CoerceMethod(cl, obj, OM_DISPOSE);
		return ((ULONG)NULL);
	}
	DoMethod(data->grp_content, OM_ADDMEMBER, grp);

	if (mode == MV_Progresswin_Look_MoveMany || MV_Progresswin_Look_CopyMany || MV_Progresswin_Look_DeleteMany)
	{
		o = HGroup,
				Child, NSLabel( MSG_PROGRESS_FILESDONE),
				Child, data->txt_files = text2("0/0"),
			End;
			
		if (!o)
		{
			CoerceMethod(cl, obj, OM_DISPOSE);
			return ((ULONG)NULL);
		}
		DoMethod(grp, OM_ADDMEMBER, o);
	}

	data->bt_stop = MUICreateButton( MSG_PROGRESS_STOP, "PROGRESS_STOP");

	if (!data->bt_stop)
	{
		CoerceMethod(cl, obj, OM_DISPOSE);
		return ((ULONG)NULL);
	}

	//set( data->bt_stop, MUIA_Text_SetMax, TRUE );
	SetAttrs( data->bt_stop, MUIA_Text_SetMax, TRUE, MUIA_Disabled, TRUE, TAG_DONE );
	DoMethod(grp, OM_ADDMEMBER, data->bt_stop);
	data->disabled = TRUE;

	DoMethod(data->bt_stop, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 1, MM_Progresswin_Stop
	);

	return ((ULONG)obj);
}


DEFGET
{
	switch (msg->opg_AttrID)
	{
		case MA_Window_ID:
			*msg->opg_Storage = 0;
			return (TRUE);

		case MA_Window_Path:
			*msg->opg_Storage = MV_Window_Path_None; /* XXX: fix fix ! */
			return (TRUE);

		case MA_Window_Type:
			*msg->opg_Storage = MV_Window_Type_Progress;
			return (TRUE);
	}
	return (DOSUPER);
}


DEFSMETHOD(Progresswin_InitProgress)
{
	GETDATA;
	UQUAD size = *msg->size;
	ULONG shift = 0;

	//PDB(("data->aborted = %ld, buffering = %ld\n", data->aborted, data->buffering));

	while (size > MAXINT)
	{
		shift++;
		size >>= 1;
	}

	data->totalshift = shift;
	data->totalfiles = msg->files;
	data->totalsize  = *msg->size;

	capacity_format_size(data->totalsizetext, sizeof(data->totalsizetext), data->totalsize);

	set(data->totalgauge, MUIA_Gauge_Max, (ULONG)size);

	data->timestamp_start = data->timestamp_prev = timedm();

	return (0);
}


DEFSMETHOD(Progresswin_Update)
{
	GETDATA;

	UQUAD size = msg->size ? *msg->size : 0;  /* NULL ptr means 0 */

	data->totaldone = msg->totaldone ? *(msg->totaldone) : 0;

	//PDB(("data->aborted = %ld, buffering = %ld, msg->name '%s', size = %ld\n", data->aborted, data->buffering, msg->name ? msg->name : "NULL", size));
	if (msg->name)
	{
		if (size)
		{
			TEXT totaldone[128];
			TEXT escapedfilename[PATH_SIZE];
			/* new file */
			ULONG shift = 0;

			data->gauge_max = size;
			data->gauge_cur = 0;

			data->current_togo = size;

			/*
			 * MUIA_Gauge_Max is LONG, so scale the value till it's
			 * within range. - piru
			 */
			while (size > MAXINT)
			{
				shift++;
				size >>= 1;
			}

			data->gauge_shift = shift;

			set(data->totalgauge, MUIA_Gauge_Current, (ULONG)(data->totaldone >> data->totalshift));

			strescape(msg->name, escapedfilename);

			SetAttrs(data->gauge,
				MUIA_Gauge_Current, 0,
				MUIA_Gauge_Max, (ULONG) size, /* shifted above */
				MUIA_Gauge_InfoText, escapedfilename,
			TAG_DONE);
			
			//if (data->buffering) data->buffering = FALSE;

			capacity_format_size(totaldone, sizeof(totaldone), data->totaldone);
			DoMethod(data->txt_files, MUIM_SetAsString, MUIA_Text_Contents, "%lu/%lu (%s/%s)", msg->count, data->totalfiles, totaldone, data->totalsizetext);
		}
		else
		{
			/* delete */
			set(data->txt_delete, MUIA_Text_Contents, msg->name);

			DoMethod(data->txt_files, MUIM_SetAsString, MUIA_Text_Contents, "%lu", msg->count);
		}
	}
	else
	{
		/* progress update */
		if (size)
		{
			ULONG shift;
			ULONG gauge_old;
			TEXT totaldone[128];
			ULONG timestamp = timedm();

			/* Refresh progress information gauge every 1s or so */
			if(timestamp > data->timestamp_prev + 1000 )
			{
				TEXT speedtext[128], remainingtext[128];
				UQUAD average_speed = (data->totaldone*1000)/(timestamp - data->timestamp_start);
				ULONG remaining = average_speed ? (data->totalsize - data->totaldone)/average_speed : 0;

				if(average_speed)
				{
					ULONG d,h,m,s;
					TEXT fmt[32];
					s = remaining; d = s / (3600*24); s = s % (3600*24); h = s / 3600; m = s / 60 % 60; s = s % 60;
					remainingtext[0] = 0;
					if (d > 0) // who knows;-)
					{
						snprintf(fmt, sizeof(fmt), GSI(((d == 1) ? MSG_PROGRESS_DAY : MSG_PROGRESS_DAYS)), d);
						strcat(remainingtext, fmt);
					}
					if (h > 0)
					{
						if (remainingtext[0]) strcat(remainingtext, ", ");
						snprintf(fmt, sizeof(fmt), GSI((h == 1 ? MSG_PROGRESS_HOUR : MSG_PROGRESS_HOURS)), h);
						strcat(remainingtext, fmt);
					}
					if (m > 0)
					{
						if (remainingtext[0]) strcat(remainingtext, ", ");
						snprintf(fmt, sizeof(fmt), GSI((m == 1 ? MSG_PROGRESS_MINUTE : MSG_PROGRESS_MINUTES)), m);
						strcat(remainingtext, fmt);
					}
					if (s > 0)
					{
						if (remainingtext[0]) strcat(remainingtext, ", ");
						snprintf(fmt, sizeof(fmt), GSI((s == 1 ? MSG_PROGRESS_SECOND : MSG_PROGRESS_SECONDS)), s);
						strcat(remainingtext, fmt);
					}
				}
				else
				{
					stccpy(remainingtext, "-", sizeof(remainingtext));
				}

				capacity_format_size(speedtext, sizeof(speedtext), average_speed);

				snprintf(data->infotext, sizeof(data->infotext), GSI(MSG_PROGRESS_TIMEREMAINING), remainingtext, speedtext);

				set(data->totalgauge, MUIA_Gauge_InfoText, &data->infotext);

				data->timestamp_prev = timestamp;
			}

			/* Update current file progress */
			shift = data->gauge_shift;
			gauge_old = data->gauge_cur >> shift;
			data->gauge_cur += size;
			if (data->gauge_cur <= data->gauge_max)
			{
				ULONG gauge_new = data->gauge_cur >> shift;

				data->current_togo -= size;

				if (gauge_new != gauge_old)
				{
					set(data->gauge, MUIA_Gauge_Current, gauge_new);
					set(data->totalgauge, MUIA_Gauge_Current, (ULONG)(data->totaldone >> data->totalshift));
				}
			}

			/* Update global progress information */
			capacity_format_size(totaldone, sizeof(totaldone), data->totaldone);
			DoMethod(data->txt_files, MUIM_SetAsString, MUIA_Text_Contents, "%lu/%lu (%s/%s)", msg->count, data->totalfiles, totaldone, data->totalsizetext);
		}
		else
		{
			DoMethod(obj, MM_Progresswin_Close);
		}
	}
	if (data->disabled) set( data->bt_stop, MUIA_Disabled, data->disabled = FALSE );

	return (0);
}


DEFTMETHOD(Progresswin_Stop)
{
	GETDATA;

	thread_signal(data->thread, TRUE);
	set(obj, MUIA_Window_Open, FALSE);
	//if (data->buffering) data->aborted = TRUE;
	/* then it's the process which sends an Update to close it */
	return (0);
}

DEFTMETHOD(Progresswin_Close)
{
	//GETDATA;
	//PDB(("data->aborted = %ld, buffering = %ld\n", data->aborted, data->buffering));
	//data->aborted = data->buffering = FALSE;
	methodstack_push(_app(obj), 2, MM_Application_DisposeWindow, obj);
	return (0);
}


BEGINMTABLE
DECNEW
DECGET
DECSMETHOD(Progresswin_Update)
DECTMETHOD(Progresswin_Stop)
DECTMETHOD(Progresswin_Close)
DECSMETHOD(Progresswin_InitProgress)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, progresswinclass)
