/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
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
 * $Id: mimeadjustgroupclass.h,v 1.1 2006/09/18 23:17:29 fab Exp $
 */

#ifndef AMBIENT_MIMEADJUSTGROUPCLASS_H
#define AMBIENT_MIMEADJUSTGROUPCLASS_H

struct ActionEntry
{
	ULONG id;
	APTR  o;
	ULONG inherited;
	APTR action_node;
	TEXT name[256];
	TEXT type[256];
};

#endif
