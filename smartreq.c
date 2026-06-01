/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2017 Ambient Open Source Team
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
 * $Id: smartreq.c,v 1.22 2025/09/12 16:06:01 jacadcaps Exp $
 */

#include "ambient.h"

/* public */
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>

/* private */
#include "ambient_cat.h"
#include "capacity.h"
#include "examine64.h"
#include "findver.h"
#include "md5sum.h"
#include "mui_func.h"
#include "prefs.h"
#include "smartreq.h"
#include "screen.h"
#include "methodstack.h"
#include "template.h"
#include "threads.h"
#include "dos_internal.h"


static APTR reqobj;
static struct reqmsg rqmsg;

static STRPTR titlebuf[2];
static ULONG titlebufnum;

static struct SignalSemaphore reqsem;

/*
 * Settable constants
 */
#define REQ_TITLEBUFSIZE 256
#define REQ_BUFFERSIZE 4096 /* same as MUI_Request() but without buffer overflows */

#if (REQ_BUFFERSIZE * 2) > (STACKSIZE - 2048)
#error main stacksize too low
#elseif (REQ_BUFFERSIZE * 2) > (STACKSIZE_THREAD - 2048)
#error thread stacksize too low
#endif


struct reqnode {
	struct MinNode n;
	ULONG methodid;
	LONG userdata;
	APTR obj;
	APTR winobj;
	ULONG flags;
	CONST_STRPTR title;
	STRPTR buffer;
	STRPTR gadgets;
	LONG type;

	/* Used by replace requester only */
	CONST_STRPTR srcpath;
	CONST_STRPTR dstpath;
};

#define SRF_SYNC (1 << 0UL) /* flags */

struct Data {
	APTR txt_req;
	APTR grp_buttons;
	APTR grp_root;
	APTR obj;
	APTR grp_moreinfo;
	struct MinList reqlist;

	/* Replace request */
	APTR currobj;
	APTR txt_srcinfo;
	APTR txt_srcver;
	APTR txt_srcmd5;
	APTR txt_dstinfo;
	APTR txt_dstver;
	APTR txt_dstmd5;

	ULONG counter;

	ULONG thread_running;
	ULONG postpone_packet;
	struct MP_SmartReq_Pressed eventpacket;
};


DEFNEW
{
	struct Data *data;
	APTR txt_req, grp_buttons, grp_root;

	obj = DoSuperNew(cl, obj,
		MUIA_Window_ScreenTitle, screentitle,
		MUIA_Window_LeftEdge, MUIV_Window_LeftEdge_Centered,
		MUIA_Window_TopEdge, MUIV_Window_TopEdge_Centered,
		MUIA_Window_Width, MUIV_Window_Width_MinMax(0),
		MUIA_Window_Height, MUIV_Window_Height_MinMax(0),
		MUIA_Window_CloseGadget, FALSE,
		MUIA_Window_Activate, TRUE,
		MUIA_Window_Remember, FALSE,
		MUIA_Window_NoMenus, TRUE, /* TOFIX: yes or no ? */
		MUIA_Window_ShowIconify, FALSE,
		MUIA_Window_ShowAbout, FALSE,
		MUIA_Window_Frontdrop, TRUE,
		WindowContents, grp_root = VGroup,
			InnerSpacing(4, 4),
			GroupSpacing(8),
			MUIA_Background, MUII_RequesterBack,
			Child, txt_req = NewObject(getnotificationclass(), NULL,
				TextFrame,
				InnerSpacing(8, 8),
				MUIA_Background, MUII_TextBack,
			End,
			Child, grp_buttons = HGroup,
			End,
		End,
		TAG_MORE, INITTAGS
	);

	if (!obj)
	{
		return (0);
	}

	data = INST_DATA(cl, obj);
	data->txt_req = txt_req;
	data->grp_buttons = grp_buttons;
	data->grp_root = grp_root;

	NEWLIST(&data->reqlist);

	return ((ULONG)obj);
}


DEFSMETHOD(Infowin_UpdateVersion)
{
	GETDATA;
	return (set(data->currobj, MUIA_Text_Contents, msg->text));
}


DEFSMETHOD(Infowin_UpdateMD5sum)
{
	GETDATA;
	return (set(data->currobj, MUIA_Text_Contents, msg->text));
}


DEFGET
{
	switch (msg->opg_AttrID)
	{
		case MA_Window_ID:
			*msg->opg_Storage = 0;
			return (TRUE);

		case MA_Window_Path:
			*msg->opg_Storage = MV_Window_Path_None; /* XXX: fix fix !! we should find another scheme. */
			return (TRUE);

		case MA_Window_Type:
			*msg->opg_Storage = MV_Window_Type_SmartReq_Delete; /* XXX: fix fix ! */
			return (TRUE);
	}
	return (DOSUPER);
}


DEFSMETHOD(SmartReq_Enqueue)
{
	GETDATA;

	ADDTAIL(&data->reqlist, msg->rn);

	/*
	 * Fire off the first request, or we wait
	 * for the next action as there's already a
	 * window.
	 */
	if (!getv(obj, MUIA_Window_Open))
	{
		DoMethod(app, MM_Application_DisplayDelayedDialog);
		DoMethod(obj, MM_SmartReq_Change);
		/* msg->rn is gone at that point! */

		if (!getv(obj, MUIA_Window_Open))
		{
			//displaybeep();
			return (0); /* argh, there's little we can do, wait the next request :/ */
		}
	}
	return (0);
}


/*
 * Changes the requester.
 */
DEFTMETHOD(SmartReq_Change)
{
	GETDATA;
	struct reqnode *rn;
	ULONG gadcnt, i;
	int act = FALSE;
	CONST_STRPTR p, ul;
	STRPTR next;
	APTR txtobj;
	TEXT iebuf[10][3];

	set(obj, MUIA_Window_Open, FALSE);

	if (!ISLISTEMPTY(&data->reqlist))
	{
		rn = FIRSTNODE(&data->reqlist);

		data->obj = rn->obj;
		data->counter++;

		// Kill notifications (well, does not help...)
		DoMethod(obj, MUIM_KillNotify, MUIA_Window_InputEvent);

		/*
		 * Set the title and main text
		 */
		titlebufnum = (titlebufnum + 1) % 2;
		strcpy(titlebuf[titlebufnum], rn->title);

		SetAttrs(obj,
			MUIA_Window_ActiveObject, NULL,
			MUIA_Window_DefaultObject, NULL,
			MUIA_Window_RefWindow, rn->winobj,
			MUIA_Window_Title, titlebuf[titlebufnum],
			TAG_DONE);

		SetAttrs(data->txt_req, MA_Notification_Text, rn->buffer, MA_Notification_Type, rn->type, TAG_DONE);

		DoMethod(data->grp_root, MUIM_Group_InitChange);

		/*
		 * Remove the childs, if any.
		 */
		FORCHILD(data->grp_buttons, MUIA_Group_ChildList)
		{
			DoMethod(data->grp_buttons, OM_REMMEMBER, child);
			MUI_DisposeObject(child);
		}
		NEXTCHILD

		if (data->grp_moreinfo)
		{
			DoMethod(data->grp_root, OM_REMMEMBER, data->grp_moreinfo);
			MUI_DisposeObject(data->grp_moreinfo);
			data->grp_moreinfo = NULL;
		}

		if (rn->srcpath && rn->dstpath)
		{
			data->grp_moreinfo = VGroup,
					Child, ColGroup(2),
						GroupFrameT(GSI(MSG_SMARTREQUEST_SOURCE)),
						Child, RectangleObject, MUIA_Weight, 0, End,
						Child, TextObject, MUIA_Text_Contents, rn->srcpath, MUIA_Text_SetMin, FALSE, End,
						Child, RectangleObject, MUIA_Weight, 0, End,
						Child, data->txt_srcinfo = TextObject, End,
						Child, NSLabel( MSG_SMARTREQUEST_VERSION ),
						Child, data->txt_srcver = TextObject, MUIA_Text_Contents, GSI( MSG_SMARTREQUEST_CHECKING), TextFrame, MUIA_Background, MUII_TextBack, End,
						Child, NSLabel( MSG_SMARTREQUEST_MD5 ),
						Child, data->txt_srcmd5 = TextObject, MUIA_Text_Contents, GSI( MSG_SMARTREQUEST_CHECKING), TextFrame, MUIA_Background, MUII_TextBack, End,
					End,
					Child, ColGroup(2),
						GroupFrameT(GSI(MSG_SMARTREQUEST_TARGET)),
						Child, RectangleObject, MUIA_Weight, 0, End,
						Child, TextObject, MUIA_Text_Contents, rn->dstpath,  MUIA_Text_SetMin, FALSE,  End,
						Child, RectangleObject, MUIA_Weight, 0, End,
						Child, data->txt_dstinfo = TextObject, End,
						Child, NSLabel( MSG_SMARTREQUEST_VERSION ),
						Child, data->txt_dstver = TextObject, MUIA_Text_Contents, GSI( MSG_SMARTREQUEST_CHECKING), TextFrame, MUIA_Background, MUII_TextBack, End,
						Child, NSLabel( MSG_SMARTREQUEST_MD5 ),
						Child, data->txt_dstmd5 = TextObject, MUIA_Text_Contents, GSI( MSG_SMARTREQUEST_CHECKING), TextFrame, MUIA_Background, MUII_TextBack, End,
					End,
				End;

			if (data->grp_moreinfo)
			{
				DoMethod(data->grp_root, OM_ADDMEMBER, data->grp_moreinfo);
				DoMethod(data->grp_root, MUIM_Group_MoveMember, data->grp_moreinfo, 1);

				if (do_action(obj, TA_SmartReq_ExamineFiles, TT_SmartReq_Data, data, TT_SmartReq_File1, rn->srcpath, TT_SmartReq_File2, rn->dstpath, TAG_DONE))
					data->thread_running = 1;
			}
		}

		/*
		 * Build the buttons
		 */
		for (gadcnt = 1, p = rn->gadgets; *p; p++)
		{
			if (*p == '|')
			{
				gadcnt++;
			}
		}

		for (p = rn->gadgets, i = 0; i < gadcnt; i++, p = next)
		{
			/* handle special chars */
			if ((next = strchr(p, '|')))
			{
				*next++ = '\0';
			}

			if (*p == '*')
			{
				act = TRUE;
				p++;
			}

			ul = strchr(p, '_');

			txtobj = TextObject,
				ButtonFrame,
				MUIA_CycleChain, 1,
				MUIA_Text_Contents, p,
				MUIA_Text_PreParse, "\33c",
				MUIA_InputMode, MUIV_InputMode_RelVerify,
				MUIA_Font, MUIV_Font_Button,
				MUIA_Background, MUII_ButtonBack,
				ul ? MUIA_Text_HiIndex : TAG_IGNORE, '_',
				ul ? MUIA_ControlChar : TAG_IGNORE, ul ? ToLower(*(ul + 1)) : 0,
			End;

			if (txtobj)
			{
				if (gadcnt == 1)
				{
					DoMethod(data->grp_buttons, OM_ADDMEMBER, HSpace(0));
					DoMethod(data->grp_buttons, OM_ADDMEMBER, HSpace(0));
					DoMethod(data->grp_buttons, OM_ADDMEMBER, txtobj);
					DoMethod(data->grp_buttons, OM_ADDMEMBER, HSpace(0));
					DoMethod(data->grp_buttons, OM_ADDMEMBER, HSpace(0));
					if (act)
					{
						set(obj, MUIA_Window_DefaultObject, txtobj);
					}
				}
				else if (i < gadcnt - 1)
				{
					DoMethod(data->grp_buttons, OM_ADDMEMBER, txtobj);
					DoMethod(data->grp_buttons, OM_ADDMEMBER, HSpace(4));
					DoMethod(data->grp_buttons, OM_ADDMEMBER, HSpace(0));
				}
				else
				{
					DoMethod(data->grp_buttons, OM_ADDMEMBER, txtobj);
				}

				if (i <= 8)
				{
					iebuf[i][0] = 'f';
					iebuf[i][1] = '0' + i + 1;
					iebuf[i][2] = 0;

					DoMethod(obj, MUIM_Notify, MUIA_Window_InputEvent, iebuf[i],
						app, 9,
						MUIM_Application_PushMethod,
						obj, 6,
						MM_SmartReq_Pressed,
						rn->methodid,
						rn->flags,
						(i == gadcnt - 1) ?  0 : (i + 1),
						rn->userdata,
						data->counter
					);
				}

				DoMethod(txtobj, MUIM_Notify, MUIA_Pressed, FALSE,
					app, 9,
					MUIM_Application_PushMethod,
					obj, 6,
					MM_SmartReq_Pressed,
					rn->methodid,
					rn->flags,
					(i == gadcnt - 1) ?  0 : (i + 1),
					rn->userdata,
					data->counter
				);

				if (act)
				{
					set(obj, MUIA_Window_ActiveObject, txtobj);
					act = FALSE;
				}
			}

			if (next)
			{
				*(next - 1) = '|';
			}
		}

		DoMethod(data->grp_root, MUIM_Group_ExitChange);

		REMOVE(rn);
		free(rn->gadgets);
		free(rn->buffer);
		free(rn);

		set(obj, MUIA_Window_Open, TRUE);
	}
	return (0);
}


static void execute_packet(APTR obj, struct Data *data, struct MP_SmartReq_Pressed *msg)
{
	if (msg->flags & SRF_SYNC)
	{
		rqmsg.retval = msg->butnum;
		PutMsg((struct MsgPort *)msg->userdata, (struct Message *)&rqmsg);
		WaitPort(rqmsg.msg.mn_ReplyPort);
		(void)GetMsg(rqmsg.msg.mn_ReplyPort);
	}
	else if (msg->methodid)
	{
		DoMethod(data->obj ? data->obj : app, msg->methodid, msg->butnum, msg->userdata);
	}
	DoMethod(obj, MM_SmartReq_Change);
}


DEFSMETHOD(Thread_Finished)
{
	GETDATA;

	data->thread_running = 0;

	if (data->postpone_packet)
	{
		data->postpone_packet = 0;

		set(obj, MUIA_Window_Sleep, FALSE);
		execute_packet(obj, data, &data->eventpacket);
	}

	return (0);
}


DEFSMETHOD(SmartReq_Pressed)
{
	GETDATA;

	/*
	 * Inform the parent object and rebuild
	 * the next requester.
	 */
	threads_abort(obj, NULL);

	/*
	 * HACK HACK HACK!
	 */
	if (data->thread_running)
	{
		data->postpone_packet = 1;
		bcopy(msg, &data->eventpacket, sizeof(*msg));
		set(obj, MUIA_Window_Sleep, TRUE);
	}
	else
	{
		if (data->counter == msg->counter)
			execute_packet(obj, data, msg);
	}

	return (0);
}


/*
 * Send out methods with a cancel field to remove allocated stuff..
 */
DEFDISPOSE
{
	GETDATA;
	struct reqnode *rn, *nextrn;

	/* We should wait threads to finish... Possible solution could be
	 * polling methodstack since stalling on exit() doesnt matter.
	 */
	threads_abort(obj, NULL);

	/* This used to REMTAIL from &data->reqlist, thus processing the nodes
	 * in reverse order. IMO processing them in order shouldn't change
	 * anything. - Piru
	 */
	ITERATELISTSAFE(rn, nextrn, &data->reqlist)
	{
		if (rn->flags & SRF_SYNC)
		{
			rqmsg.retval = 0; /* cancel */
			PutMsg((struct MsgPort *)rn->userdata, (struct Message *)&rqmsg);
			WaitPort(rqmsg.msg.mn_ReplyPort);
			(void)GetMsg(rqmsg.msg.mn_ReplyPort);
		}
		else if (rn->methodid)
		{
			DoMethod(data->obj ? data->obj : app, rn->methodid, 0, rn->userdata); /* send a cancel, XXX: I think the current displayed one is not removed.. */
		}
	}

	return (DOSUPER);
}


BEGINMTABLE
DECNEW
DECDISP
DECGET
DECSMETHOD(Thread_Finished)
DECSMETHOD(Infowin_UpdateVersion)
DECSMETHOD(Infowin_UpdateMD5sum)
DECSMETHOD(SmartReq_Enqueue)
DECSMETHOD(SmartReq_Pressed)
DECTMETHOD(SmartReq_Change)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, smartreqclass)


ULONG smartreq_init(void)
{
	InitSemaphore(&reqsem);

	if ( (rqmsg.msg.mn_ReplyPort = CreateMsgPort()) )
	{
		if ( (titlebuf[0] = malloc(REQ_TITLEBUFSIZE)) )
		{
			if ( (titlebuf[1] = malloc(REQ_TITLEBUFSIZE)) )
			{
				rqmsg.msg.mn_Node.ln_Type = NT_REPLYMSG;
				rqmsg.msg.mn_Length       = sizeof(rqmsg);

				return (TRUE);
			}
			free(titlebuf[0]);
		}
		DeleteMsgPort(rqmsg.msg.mn_ReplyPort);
	}
	return (FALSE);
}


void smartreq_cleanup(void)
{
	if (titlebuf[1])
	{
		free(titlebuf[1]);
	}

	if (titlebuf[0])
	{
		free(titlebuf[0]);
	}

	if (rqmsg.msg.mn_ReplyPort)
	{
		DeleteMsgPort(rqmsg.msg.mn_ReplyPort);
	}
}



static ULONG smartreq_requesta(APTR obj, APTR winobj, CONST_STRPTR title, ULONG methodid, LONG userdata, CONST_STRPTR gadgets, ULONG flags, LONG type, CONST_STRPTR srcpath, CONST_STRPTR dstpath, CONST_STRPTR format, va_list va)
{
	struct reqnode *rn;
	LONG errno;

	ASSERT(format);

	errno = IoErr();  /* get as soon as possible, before something trashes it */

	switch ((LONG)title)
	{
		case SRT_NONE:
			title = GSI( MSG_SMARTREQUEST_AMBIENTREQUEST );
			break;

		case SRT_DOSERROR:
			title = GSI( MSG_SMARTREQUEST_AMBIENTDOSERROR);
			break;

		case SRT_IOERROR:
			title = GSI( MSG_SMARTREQUEST_AMBIENTIOERROR );
			break;
	}

	ObtainSemaphore(&reqsem);

	if (!reqobj)
	{
		reqobj = (APTR)methodstack_push_sync(app, 1,
			MM_Application_CreateSmartReq
		);

		if (!reqobj)
		{
			ReleaseSemaphore(&reqsem);
			return (FALSE);
		}
	}

	ReleaseSemaphore(&reqsem);

	if ((rn = malloc(sizeof(*rn))))
	{
		TEXT buf[REQ_BUFFERSIZE];
		TEXT bufexp[REQ_BUFFERSIZE];
		TEXT errnum[16];
		CONST_STRPTR errstr;
		ULONG len;
		rn->methodid = methodid;
		rn->userdata = userdata;
		rn->obj = obj;
		rn->winobj = winobj;
		rn->flags = flags;
		rn->type = type;
		rn->srcpath = srcpath;
		rn->dstpath = dstpath;

		if (errno)
		{
			errstr = DosGetString(errno);

			if (!errstr || !*errstr)
			{
				errstr = GSI( MSG_SMARTREQUEST_UNKNOWN );
			}
		}
		else
		{
			errstr = GSI( MSG_SMARTREQUEST_UNKNOWN );
		}

		snprintf(errnum, sizeof(errnum), "%ld", errno);
		template_expand(format, bufexp, sizeof(bufexp),
			'S', errstr,
			'N', errnum,
			NULL
		);

		/*
		 * Process the input to build a buffer.
		 */
		buf[0] = '\0'; /* I don't know if vsnprintf() NULL terminates if the format is not here.. better safe than sorry */
		vsnprintf(buf, sizeof(buf), bufexp, va);
		len = strlen(buf);

		if (len)
		{
			if ((rn->buffer = malloc(len + 1)))
			{
				strcpy(rn->buffer, buf);

				if ((rn->gadgets = malloc(strlen(gadgets) + 1)))
				{
					strcpy(rn->gadgets, gadgets);
					rn->title = title;

					methodstack_push(reqobj, 2,
						MM_SmartReq_Enqueue, rn
					);
					return (TRUE);
				}
				free(rn->buffer);
			}
		}
		free(rn);
	}
	return (FALSE);
}


/*
 * Simple requester that has only OK choice.
 */
VOID smartreq_ok(size_t title, size_t type, size_t format, ...)
{
	va_list va;
	va_start(va, format);

	smartreq_requesta(NULL, NULL, GSI(title), 0, 0, GSI(MSG_REQUESTER_OK_GAD), 0, type, NULL, NULL, GSI(format), va);

	va_end(va);
}

/*
 * We don't use a method so that we have
 * the convenience of varargs. Methods are always sent to the app object.
 * Can be called from another task.
 */
ULONG smartreq_request(APTR obj, APTR winobj, CONST_STRPTR title, ULONG methodid, LONG userdata, CONST_STRPTR gadgets, LONG type, CONST_STRPTR format, ...)
{
	ULONG res;
	va_list va;

	va_start(va, format);

	res = smartreq_requesta(obj, winobj, title, methodid, userdata, gadgets, 0, type, NULL, NULL, format, va);

	va_end(va);

	return (res);
}


static ULONG smartreq_request_synca(APTR winobj, CONST_STRPTR title, CONST_STRPTR gadgets, LONG type, CONST_STRPTR file1, CONST_STRPTR file2, CONST_STRPTR format, va_list va)
{
	ULONG rc = 0;
	struct Process *me;
	struct MsgPort mp;

	THREAD;

	me = (struct Process *)FindTask(NULL);
	//td = me->pr_Task.tc_UserData;

	if ( (BYTE) (mp.mp_SigBit = AllocSignal(-1)) != -1 )
	{
		mp.mp_Node.ln_Type = NT_MSGPORT;
		mp.mp_Flags        = PA_SIGNAL;
		mp.mp_SigTask      = &me->pr_Task;
		NEWLIST(&mp.mp_MsgList);

		//td->reqport = &mp;

		if (smartreq_requesta(NULL, winobj, title, 0, (LONG) &mp, gadgets, SRF_SYNC, type, file1, file2, format, va))
		{
			struct reqmsg *rm;

			WaitPort(&mp);
			rm = (struct reqmsg *)GetMsg(&mp);
			rc = rm->retval;
			ReplyMsg(&rm->msg);
		}

		//td->reqport = NULL;

		FreeSignal(mp.mp_SigBit);
	}
	return (rc); /* XXX */
}


ULONG smartreq_request_sync(APTR winobj, CONST_STRPTR title, CONST_STRPTR gadgets, LONG type, CONST_STRPTR format, ...)
{
	va_list va;
	ULONG rc;
	va_start(va, format);
	rc = smartreq_request_synca(winobj, title, gadgets, type, NULL, NULL, format, va);
	va_end(va);
	return (rc); /* XXX */
}


ULONG smartreq_replacefile_sync(APTR winobj, CONST_STRPTR title, CONST_STRPTR gadgets, LONG type, CONST_STRPTR from, CONST_STRPTR to, CONST_STRPTR format, ...)
{
	va_list va;
	ULONG rc;
	va_start(va, format);
	rc = smartreq_request_synca(winobj, title, gadgets, type, from, to, format, va);
	va_end(va);
	return (rc); /* XXX */
}


static ULONG set_fileinfo(APTR obj, CONST_STRPTR file)
{
	struct fileinfo64 fi;
	ULONG rc = TRUE;

	if (examine64(file, &fi))
	{
		struct DateTime dt;
		TEXT tmpdate[ 50 ];
		TEXT tmptime[ 50 ];
		TEXT t[32];
		TEXT info[256];
		CONST_STRPTR str;

		dt.dat_Stamp.ds_Days    = fi.fi_Date.ds_Days;
		dt.dat_Stamp.ds_Minute  = fi.fi_Date.ds_Minute;
		dt.dat_Stamp.ds_Tick    = fi.fi_Date.ds_Tick;
		dt.dat_Format           = FORMAT_DEF; /* FORMAT_DOS */
		dt.dat_Flags            = 0; /* DTF_SUBST */
		dt.dat_StrDay           = NULL;
		dt.dat_StrDate          = tmpdate;
		dt.dat_StrTime          = tmptime;

		if (!DateToStr(&dt))
		{
			strcpy(tmpdate, "Unknown");
			strcpy(tmptime, "Unknown");
		}

		str = " ";

		if (fi.fi_Size > 0)
		{
			if(_conf(fl_compact_size_display))
			{
				capacity_format_size(t, sizeof(t), fi.fi_Size);
			}
			else
			{
				str = " bytes ";
				capacity_format_size_separated(t, sizeof(t), fi.fi_Size);
			}
		}
		else
		{
			strcpy(t, "<empty>");
		}

		snprintf(info, sizeof(info), "%s%s(%s %s)", t, str, tmpdate, tmptime);
		methodstack_push_sync(obj, 3, MUIM_Set, MUIA_Text_Contents, &info);
	}

	if (threads_check_abort())
		rc = FALSE;

	return rc;
}

ULONG tr_smartreq_examine(APTR obj, APTR idata, CONST_STRPTR file1, CONST_STRPTR file2)
{
	struct Data *data = idata;

	THREAD; /* moron protection */
	ASSERT(data);
	ASSERT(file1);
	ASSERT(file2);

	if (set_fileinfo(data->txt_srcinfo, file1))
	{
		if (set_fileinfo(data->txt_dstinfo, file2))
		{
			/* hacky */
			data->currobj = data->txt_srcver;
			if (tr_findver(obj, file1) != ABORTED)
			{
				data->currobj = data->txt_srcmd5;
				if (tr_md5sum(obj, file1) != ABORTED)
				{
					data->currobj = data->txt_dstver;
					if (tr_findver(obj, file2) != ABORTED)
					{
						data->currobj = data->txt_dstmd5;
						tr_md5sum(obj, file2);
					}
				}
			}
		}
	}

	return (0);
}
