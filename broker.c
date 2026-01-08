/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2006-2012 Ambient Open Source Team
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
 * $Id: broker.c,v 1.5 2012/04/29 15:18:23 itix Exp $
 */

#include "ambient.h"

/* public */
#include <libraries/mui.h>
#include <proto/alib.h>
#include <proto/commodities.h>

/* private */
#include "broker.h"
#include "classes.h"
#include "prefs_advanced.h"

#define EXTRAWKEY_POWER       0x5e
#define EXTRAWKEY_MYCOMPUTER  0x6b

#ifndef IECLASS_EXTRAWKEY
#define IECLASS_EXTRAWKEY     0x18
#endif

STATIC VOID BrokerFunc(void)
{
	APTR app = (APTR)REG_A2;
	CxMsg *msg = (APTR)REG_A1;

	if (CxMsgType(msg) == CXM_IEVENT)
	{
		switch (CxMsgID(msg))
		{
#if !USE_LEGACY
			case EVT_EVENTSENDER:
				if (_aprefs(mmkeyevents))
				{
					struct InputEvent *ie = CxMsgData(msg);

					if (ie->ie_Class == IECLASS_EXTRAWKEY)
					{
						if (ie->ie_ExtKey1 == EXTKEY_MODE1)
						{
							switch (ie->ie_Code)
							{
								case EXTRAWKEY_POWER:
									DoMethod(app, MUIM_Application_PushMethod, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
									return;

								case EXTRAWKEY_MYCOMPUTER:
									DoMethod(app, MM_Application_OpenDevicesWindow, 0);
									return;
							}
						}
					}
				}
				break;
#endif

			case EVT_EXCHANGE_HOTKEY:
				DoMethod(app, MUIM_Application_PushMethod, app, 1, MM_Application_Open_CxWindow);
				break;
		}
	}
}

STATIC const struct EmulLibEntry BrokerHookTrap = { TRAP_LIBNR, 0, (APTR)&BrokerFunc };

const struct Hook BrokerHook = { { NULL, NULL }, (HOOKFUNC)&BrokerHookTrap, NULL, NULL };
