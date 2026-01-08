/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
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
 * $Id: callmain.c,v 1.1 2017/07/25 19:52:47 piru Exp $
 */
#include "ambient.h"

APTR __savesp;

extern LONG zmain(STRPTR arg, ULONG arglen);

asm("	.section \".text\"\n\
\n\
	.globl	callmain\n\
	.type	callmain,@function\n\
callmain:\n\
	stwu	1,-96(1)\n\
	mflr	12\n\
	mr	11,2\n\
	mfcr	10\n\
	stmw	10,8(1)\n\
	stw	12,100(1)\n\
"
"	lis	12,__savesp@ha\n\
	stw	1,__savesp@l(12)\n\
"
"	bl	zmain\n\
	lwz	0,100(1)\n\
	mtlr	0\n\
	addi	1,1,96\n\
	blr\n\
");
