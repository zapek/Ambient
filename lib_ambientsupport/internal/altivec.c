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
 * $Id: altivec.c,v 1.2 2017/07/25 20:36:32 piru Exp $
 */

#include "../config.h"
#include "../macros.h"
#include "../library.h"

#include <exec/execbase.h>
#include <exec/system.h>
#include <proto/exec.h>

#include "ambient_altivec.h"


/* Warning: Don't actually use the altivec in altivec_init and
 * altivec_cleanup. These routines must work even if there is
 * no altivec in the CPU.
 */

ULONG altivec_init( struct AmbientSupportBase *AmbientSupportBase )
{
	NewGetSystemAttrs(&AmbientSupportBase->HasAltiVec, sizeof(AmbientSupportBase->HasAltiVec), SYSTEMINFOTYPE_PPC_ALTIVEC, TAG_DONE );
#if USE_ALTIVEC
	if( AmbientSupportBase->HasAltiVec ) // check env here && !_var(noaltivec))
	{
		AmbientSupportBase->UseAltiVec = TRUE;
	}
#endif

	return (TRUE);
}

#if 0
void altivec_cleanup( struct AmbientSupportBase *AmbientSupportBase )
{
	/* nothing to cleanup */
}
#endif

#if USE_ALTIVEC

/*
 * Since we don't link with libgcc.a, we have
 * to save ourself.
 */
#define FUNC_START(x) " .globl " #x "\n" #x ":\n"
#define FUNC_END(x)
#define ENTRY_POINT(x) " .globl " #x "\n" #x ":\n"
asm(
	FUNC_START(_savev20)
"	addi	12,0,-192\n"
"	stvx	20,12,0	# save v20\n"
	ENTRY_POINT(_savev21)
"	addi	12,0,-176\n"
"	stvx	21,12,0	# save v21\n"
	ENTRY_POINT(_savev22)
"	addi	12,0,-160\n"
"	stvx	22,12,0	# save v22\n"
	ENTRY_POINT(_savev23)
"	addi	12,0,-144\n"
"	stvx	23,12,0	# save v23\n"
	ENTRY_POINT(_savev24)
"	addi	12,0,-128\n"
"	stvx	24,12,0	# save v24\n"
	ENTRY_POINT(_savev25)
"	addi	12,0,-112\n"
"	stvx	25,12,0	# save v25\n"
	ENTRY_POINT(_savev26)
"	addi	12,0,-96\n"
"	stvx	26,12,0	# save v26\n"
	ENTRY_POINT(_savev27)
"	addi	12,0,-80\n"
"	stvx	27,12,0	# save v27\n"
	ENTRY_POINT(_savev28)
"	addi	12,0,-64\n"
"	stvx	28,12,0	# save v28\n"
	ENTRY_POINT(_savev29)
"	addi	12,0,-48\n"
"	stvx	29,12,0	# save v29\n"
	ENTRY_POINT(_savev30)
"	addi	12,0,-32\n"
"	stvx	30,12,0	# save v30\n"
	ENTRY_POINT(_savev31)
"	addi	12,0,-16\n"
"	stvx	31,12,0	# save v31\n"
"	blr			# return to prologue\n"
	FUNC_END(_savev20)

	FUNC_START(_restv20)
"	addi	12,0,-192\n"
"	lvx	20,12,0	# restore v20\n"
	ENTRY_POINT(_restv21)
"	addi	12,0,-176\n"
"	lvx	21,12,0	# restore v21\n"
	ENTRY_POINT(_restv22)
"	addi	12,0,-160\n"
"	lvx	22,12,0	# restore v22\n"
	ENTRY_POINT(_restv23)
"	addi	12,0,-144\n"
"	lvx	23,12,0	# restore v23\n"
	ENTRY_POINT(_restv24)
"	addi	12,0,-128\n"
"	lvx	24,12,0	# restore v24\n"
	ENTRY_POINT(_restv25)
"	addi	12,0,-112\n"
"	lvx	25,12,0	# restore v25\n"
	ENTRY_POINT(_restv26)
"	addi	12,0,-96\n"
"	lvx	26,12,0	# restore v26\n"
	ENTRY_POINT(_restv27)
"	addi	12,0,-80\n"
"	lvx	27,12,0	# restore v27\n"
	ENTRY_POINT(_restv28)
"	addi	12,0,-64\n"
"	lvx	28,12,0	# restore v28\n"
	ENTRY_POINT(_restv29)
"	addi	12,0,-48\n"
"	lvx	29,12,0	# restore v29\n"
	ENTRY_POINT(_restv30)
"	addi	12,0,-32\n"
"	lvx	30,12,0	# restore v30\n"
	ENTRY_POINT(_restv31)
"	addi	12,0,-16\n"
"	lvx	31,12,0	# restore v31\n"
"	blr		# return to prologue\n"
	FUNC_END(_restv20)
);

#endif
