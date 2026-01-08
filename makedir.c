/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2016 Ambient Open Source Team
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
 * $Id: makedir.c,v 1.16 2018/02/28 19:06:33 bitrocky Exp $
 */

#define USE_INLINE_STDARG
#include "ambient.h"

/* public */
#include <exec/execbase.h>
#include <proto/dos.h>
#include <proto/wb.h>

/* private */
#include "ambient_cat.h"
#include "deficon_getpath.h"
#include "makedir.h"
#include "fastcopy.h"
#include "name.h"
#include "mui_func.h"
#include "file_func.h" // for exists()
#include "notify.h"
#include "smartreq.h"
#include "methodstack.h"


static ULONG create_cb(struct Hook *h UNUSED, STRPTR file, APTR msg UNUSED) // bitRocky: added UNUSED, ok?
{
	notify_action(file, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Create);
	return 0;
}

static ULONG error_cb(struct Hook *h UNUSED, STRPTR path, APTR msg UNUSED) // bitRocky: added UNUSED, ok?
{
	return (smartreq_request_sync(NULL, GSI(MSG_MAKEDIR_TITLE), GSI(MSG_MAKEDIR_RETRYCANCEL), MV_Notification_Error, GSI(MSG_MAKEDIR_ERRORMSG), path) == 1);
}

ULONG tr_makedir(CONST_STRPTR basedir, CONST_STRPTR path, ULONG createicon)
{
	int len = strlen(basedir) + strlen(path) + 4;
	STRPTR p = AllocMem(len, MEMF_ANY);
	ULONG rc = FALSE;

	THREAD;

	if (p)
	{
		stccpy(p, basedir, len);
		AddPart(p, path, len);
		
		if (exists(p))
		{
			SetIoErr(ERROR_OBJECT_EXISTS); // else error code is 0
			rc = smartreq_request_sync(NULL, GSI(MSG_MAKEDIR_TITLE), GSI(MSG_MAKEDIR_CANCEL), MV_Notification_Warning, GSI(MSG_MAKEDIR_ERRORMSG), p);
		}
		else
		{
			struct Hook crthook, errhook;

			crthook.h_Entry = (HOOKFUNC)HookEntry;
			crthook.h_SubEntry = (HOOKFUNC)create_cb;
			errhook.h_Entry = (HOOKFUNC)HookEntry;
			errhook.h_SubEntry = (HOOKFUNC)error_cb;

			methodstack_push_sync( app, 3, MM_Application_EnableDOSNotify, basedir, FALSE );

			rc = CreateDrawerTags(p,
				WBCREATEDRAWER_CreateHook, (IPTR)&crthook,
				WBCREATEDRAWER_ErrorHook, (IPTR)&errhook,
				WBCREATEDRAWER_CreateIcon, createicon,
				TAG_DONE);

			methodstack_push_sync( app, 3, MM_Application_EnableDOSNotify, basedir, TRUE );
		}
		FreeMem(p, len);
	}

	return rc;
}
