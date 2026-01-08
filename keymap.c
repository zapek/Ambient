/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2010 Ambient Open Source Team
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
 * $Id: keymap.c,v 1.6 2010/07/10 20:29:42 itix Exp $
 */

#include "ambient.h"

/* public */
#include <intuition/intuition.h>
#include <proto/keymap.h>

/* private */
#include "keymap.h"


TEXT keymap_vanilla(struct IntuiMessage *imsg)
{
	ASSERT(KeymapBase);
	ASSERT(imsg->Class == IDCMP_RAWKEY);

	if (!(imsg->Code & IECODE_UP_PREFIX))
	{
		struct InputEvent ie;
		TEXT c;

		ie.ie_Class        = IECLASS_RAWKEY;
		ie.ie_SubClass     = 0;
		ie.ie_Code         = imsg->Code;
		ie.ie_Qualifier    = imsg->Qualifier;
		ie.ie_EventAddress = imsg->IAddress ? (APTR *)*((ULONG *)imsg->IAddress) : NULL;

		if (MapRawKey(&ie, &c, 1, NULL) == 1)
		{
			return (c);
		}
	}
	return '\0';
}
