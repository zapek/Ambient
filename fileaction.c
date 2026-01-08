/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2006 Ambient Open Source Team
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
 * $Id: fileaction.c,v 1.1 2006/09/18 23:17:28 fab Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "ambient_cat.h"
#include "fileaction.h"
#include "file_func.h"
#include "mui_func.h"
#include "smartreq.h"

ULONG fileaction_unprotect(CONST_STRPTR path, ULONG mode, ULONG *flags)
{
	CONST_STRPTR title = title, body = body;
	ULONG req, rc = rc, unprotflag = unprotflag, skipflag = skipflag, dosprot = dosprot;

	switch (mode)
	{
		case FILEACTION_UNPROTECT_FROM_DELETION:
			unprotflag = FILEACTION_UNPROTECT_DELETION_ALL;
			skipflag   = FILEACTION_UNPROTECT_DELETION_SKIP_ALL;
			dosprot    = PROTF_DELETE | PROTF_WRITE;
			title      = GSI( MSG_FILEACTION_DELETETITLE );
			body       = GSI( MSG_FILEACTION_DELETEMSG   );
			break;

		case FILEACTION_UNPROTECT_FROM_OVERWRITE:
			unprotflag = FILEACTION_UNPROTECT_OVERWRITE_ALL;
			skipflag   = FILEACTION_UNPROTECT_OVERWRITE_SKIP_ALL;
			dosprot    = PROTF_DELETE | PROTF_WRITE;
			title      = GSI( MSG_FILEACTION_OVERWRITETITLE );
			body       = GSI( MSG_FILEACTION_OVERWRITEMSG   );
			break;

		default:
			PDB(("Bad mode\n"));
			break;
	}

	if (*flags & unprotflag)
	{
		req = 1;
	}
	else if (*flags & skipflag)
	{
		req = 3;
	}
	else
	{
		req = smartreq_request_sync(NULL, title, GSI( MSG_FILEACTION_PROTECTBUTTONS ), MV_Notification_Error, body, path);
	}

	switch (req)
	{
		case 2:
			*flags |= unprotflag;
		case 1:
			if (!dosprotection_set(path, dosprot))
			{
				smartreq_info(title, MV_Notification_Error, GSI( MSG_FILEACTION_PROTECTIONERROR ), path);
				*flags |= FILEACTION_REQUEST_CANCEL;
				rc = FILEACTION_CANCEL;
				break;
			}
		case 5:
			rc = FILEACTION_PROCEED;
			break;

		case 4:
			*flags |= skipflag;
		case 3:
			rc = FILEACTION_SKIP;
			break;

		case 0:
			*flags |= FILEACTION_REQUEST_CANCEL;
			rc = FILEACTION_CANCEL;
			break;
	}

	return rc;
}


ULONG fileaction_replace(CONST_STRPTR from, CONST_STRPTR to, ULONG mode, ULONG *flags)
{
	CONST_STRPTR title = title;
	ULONG req, rc = rc;

	switch (mode)
	{
		case FILEACTION_REPLACE_COPY:
			title      = GSI( MSG_FILEACTION_COPYTITLE );
			break;

		case FILEACTION_REPLACE_MOVE:
			title      = GSI( MSG_FILEACTION_MOVETITLE );
			break;

		default:
			PDB(("Bad mode\n"));
			break;
	}

	if (*flags & FILEACTION_REPLACE_ALL)
	{
		req = 1;
	}
	else if (*flags & FILEACTION_REPLACE_SKIP_ALL)
	{
		req = 3;
	}
	else
	{
		req = smartreq_replacefile_sync(NULL, title, GSI( MSG_FILEACTION_REPLACEBUTTONS ), MV_Notification_Warning, from, to, GSI( MSG_FILEACTION_REPLACEMSG), to);
	}

	switch (req)
	{
		case 2:
			*flags |= FILEACTION_REPLACE_ALL;
		case 1:
			rc = FILEACTION_PROCEED;
			break;

		case 4:
			*flags |= FILEACTION_REPLACE_SKIP_ALL;
		case 3:
			rc = FILEACTION_SKIP;
			break;

		default:
		case 0:
			*flags |= FILEACTION_REQUEST_CANCEL;
			rc = FILEACTION_CANCEL;
			break;
	}

	return rc;
}
