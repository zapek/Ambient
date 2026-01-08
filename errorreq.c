/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2007 Ambient Open Source Team
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
 * $Id: errorreq.c,v 1.6 2007/02/11 22:35:28 fab Exp $
 */

#include "ambient.h"

/* public */
#include <intuition/intuition.h>
#include <proto/intuition.h>

/* private */
#include "errorreq.h"


/*
 * Shows a requester and waits for the user to press
 * a button.
 *
 * - title: title string or NULL for default
 * - body: string for the requester
 * - gadgets: string describing gadgets or NULL for default
 */
LONG v_errorreq(CONST_STRPTR title, CONST_STRPTR body, CONST_STRPTR gadgets, APTR args)
{
	LONG res;
	struct EasyStruct es;
	TEXT titlebuf[256];

	ASSERT(body);

	if (title)
	{
		snprintf(titlebuf, sizeof(titlebuf), "Ambient - %s",title);
	}

	es.es_StructSize    = sizeof(struct EasyStruct);
	es.es_Flags         = NULL;
	es.es_Title         = (STRPTR)(title ? titlebuf : (CONST_STRPTR)"Ambient");
	es.es_TextFormat    = (STRPTR)body;
	es.es_GadgetFormat  = (STRPTR)(gadgets ? gadgets : (CONST_STRPTR)"Ok");

	res = EasyRequestArgs(NULL, &es, NULL, args);

	return res;
}

