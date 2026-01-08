/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * © 2001-2004 by David Gerber <zapek@morphos.net>
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
 * $Id: wbobject.c,v 1.6 2006/08/08 13:31:39 fab Exp $
 */

#include "globals.h"

/* public */
#include <string.h>
#include <exec/memory.h>
#include <workbench/workbench.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <proto/dos.h>

/* private */
#include "freelist.h"
#include <macros/vapor.h>
#include "icon_internal.h"
#include "clib/icon_protos.h"


#define DB_WBO 0


#if USE_ICONLIB_JUMPTABLE
#ifndef __PPCINLINE_MACROS_H
#include <ppcinline/macros.h>
#endif
extern struct Library *IconBaseInt;

#define GetIcon(__p0, __p1, __p2) \
	LP3(42, BOOL , GetIcon, \
		STRPTR , __p0, a0, \
		struct DiskObject *, __p1, a1, \
		struct FreeList *, __p2, a2, \
		, IconBaseInt, 0, 0, 0, 0, 0, 0)

#define PutIcon(__p0, __p1) \
	LP2(48, BOOL , PutIcon, \
		STRPTR , __p0, a0, \
		struct DiskObject *, __p1, a1, \
		, IconBaseInt, 0, 0, 0, 0, 0, 0)
#endif



/*****i icon.library/AllocWBObject *******************************************
*
*   NAME
*       AllocWBObject - allocate a Workbench object.
*
*   SYNOPSIS
*       object = AllocWBObject().
*         D0
*
*       struct OldWBObject *;
*
*   FUNCTION
*       This routine allocates a Workbench object, and initializes
*       its free list.  A subsequent call to FreeWBObject will
*       free all of its memory.
*
*       If memory cannot be obtained, a NULL is returned.
*
*       This routine is intended only for internal users that can
*       track changes to the Workbench.
*
*   INPUTS
*       None
*
*   RESULTS
*       object - a pointer to the WBObject (if memory is available) else NULL
*
*   SEE ALSO
*       AllocEntry(), FreeEntry(), FreeWBObject()
*
*   BUGS
*       None
*
******************************************************************************
*/
struct WBObject * AllocWBObject(void)
{
	struct WBObject *wbo;

	D(WBO,bug("called\n"));

	if ((wbo = AllocVec(sizeof(*wbo), MEMF_ANY | MEMF_CLEAR)))
	{
		D(WBO,bug("wbo 0x%lx\n", (ULONG)wbo));
		if (create_freelist(&wbo->wo_FreeList))
		{
			return (wbo);
		}
		else
		{
			D(WBO,bug("create_freelist failed\n"));
		}
		FreeVec(wbo);
	}
	return (NULL);
}


/*****i icon.library/FreeWBObject ********************************************
*
*   NAME
*       FreeWBObject - free all memory in a Workbench object.
*
*   SYNOPSIS
*       FreeWBObject( obj )
*                     A0
*
*       void = struct OldWBObject *;
*
*   FUNCTION
*       This routine frees all memory in a Workbench object, and the
*       object itself.  It is implemented via FreeFreeList().
*
*       AllocWBObject() takes care of all the initialization required
*       to set up the objects free list.
*
*       This routine is intended only for internal users that can
*       track changes to the Workbench.
*
*   INPUTS
*       free -- a pointer to a FreeList structure
*
*   RESULTS
*       None
*
*   SEE ALSO
*       AllocEntry(), FreeEntry(), AllocWBObject(), FreeFreeList()
*
*   BUGS
*       None
*
******************************************************************************
*/

void FreeWBObject(struct WBObject *wbo)
{
	D(WBO,bug("called\n"));
	FreeFreeList(&wbo->wo_FreeList);
	FreeVec(wbo);
}


/*****i icon.library/GetWBObject *********************************************
*
*   NAME
*       GetWBObject - read in a Workbench object from disk.
*
*   SYNOPSIS
*       wbobject = GetWBObject(name)
*         D0                   A0
*
*       struct OldWBObject * = char *;
*
*   FUNCTION
*       This routine reads in a Workbench object in from disk.  The
*       name parameter will have a ".info" postpended to it, and the
*       info file of that name will be read.  If the call fails,
*       it will return zero.  The reason for the failure may be obtained
*       via IoErr().
*
*       This routine is intended only for internal users that can
*       track changes to the Workbench.
*
*   INPUTS
*       name -- name of the object (pointer to a character string).
*
*   RESULTS
*       wbobject -- the Workbench object in question
*
*   SEE ALSO
*
*   BUGS
*       None
*
******************************************************************************
*/
APTR GetWBObject(CONST_STRPTR name)
{
	struct DiskObject	diskobj;
	struct WBObject		*wbo;

	D(WBO,bug("called with %s\n", name));

	if (name)
	{
		/* we expect a clear mem alloc
		 */
		if ((wbo=AllocWBObject()))
		{
			D(WBO,bug("wbo 0x%lx\n", (ULONG)wbo));
			/*
			 * Fill-in defaults.
			 */
			diskobj.do_Magic             = WB_DISKMAGIC;
			diskobj.do_Version           = 1;
			diskobj.do_Type              = WBDISK;
			diskobj.do_Gadget.Activation = GACT_RELVERIFY;
			diskobj.do_Gadget.GadgetType = GTYP_BOOLGADGET;
			diskobj.do_CurrentX          = NO_ICON_POSITION;
			diskobj.do_CurrentY          = NO_ICON_POSITION;
			diskobj.do_StackSize         = 4096;

			if ((GetIcon((STRPTR) name, &diskobj, &wbo->wo_FreeList)))
			{
				struct RealOldDrawerData	*drawerdata;

				D(WBO,bug("geticon worked\n"));
				wbo->wo_Type		=	diskobj.do_Type;
				wbo->wo_DefaultTool	=	diskobj.do_DefaultTool;
				if ((drawerdata=(struct RealOldDrawerData*) diskobj.do_DrawerData))
				{
					wbo->wo_DrawerData	=(struct DrawerData*) drawerdata;
					drawerdata->dd_Object	=	wbo;
				}
				wbo->wo_CurrentX	=	diskobj.do_CurrentX;
				wbo->wo_CurrentY	=	diskobj.do_CurrentY;
				wbo->wo_ToolTypes	=	(char **)diskobj.do_ToolTypes; /* XXX: is that right ? */
				memcpy(&wbo->wo_Gadget,
									   &diskobj.do_Gadget,
									   sizeof(wbo->wo_Gadget));
				wbo->wo_ToolWindow	=	diskobj.do_ToolWindow;
				wbo->wo_StackSize	=	diskobj.do_StackSize;

				return(wbo);
			}
			else
			{
				D(WBO,bug("geticon failed\n"));
				FreeWBObject(wbo);
			}
		}
	}
	return (NULL);
}


/*****i icon.library/PutWBObject *********************************************
*
*   NAME
*       PutWBObject - write out a Workbench object to disk.
*
*   SYNOPSIS
*       status = PutWBObject( name, object )
*         D0                   A0     A1
*
*       BOOL = char *, struct OldWBObject *;
*
*   FUNCTION
*       This routine writes a Workbench object out to disk.  The
*       name parameter will have a ".info" postpended to it, and
*       that file name will have the disk-resident information
*       written into it.  If the call fails, it will return a zero.
*       The reason for the failure may be obtained via IoErr().
*
*       This routine is intended only for internal users that can
*       track changes to the Workbench.
*
*   INPUTS
*       name -- name of the object (pointer to a character string)
*       object -- the Workbench object to be written out
*
*   RESULTS
*       status -- TRUE if call succeeded, else FALSE.
*
*   SEE ALSO
*
*   BUGS
*       None
*
******************************************************************************
*/
BOOL PutWBObject(CONST_STRPTR name, APTR object)
{
	/* XXX */
	D(WBO,bug("called with %s for 0x%lx\n", name, (ULONG)object));

	SetIoErr(ERROR_ACTION_NOT_KNOWN);
	return (FALSE);
}

