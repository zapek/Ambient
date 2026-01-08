/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * © 2001-2004 by David Gerber <zapek@morphos.net>
 * All Rights Reserved
 *
 * use 'make rebuild_defaults' to rebuild default.c
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
 * $Id: default_template.c,v 1.3 2006/02/22 14:48:24 fab Exp $
 */

#include "globals.h"

/* public */
#include <intuition/intuition.h>
#include <workbench/workbench.h>

/* private */
#include "icon_internal.h"
#include "default.h"

extern const struct Image default_image;
extern const struct Image default_disk_image;
extern const struct Image default_drawer_image;
extern const struct Image default_kick_image;
extern const struct Image default_project_image;
extern const struct Image default_tool_image;

/*
 * Default structures. We use
 * the same as the original icon.library
 * for consistency (most of the time).
 */

/*
 * Default DrawerData
 */
static struct DrawerData defdrawerdata = {
	{ /* dd_NewWindow */
	    50,           /* LeftEdge */
		50,           /* TopEdge */
		400,          /* Width */
		100,          /* Height */
		0xff,         /* DetailPen */
		0xff,         /* BlockPen */
		NULL,         /* ICDMPFlags */
		WFLG_SIZEGADGET |
		WFLG_DRAGBAR |
		WFLG_DEPTHGADGET |
		WFLG_CLOSEGADGET |
		WFLG_SIZEBRIGHT |
		WFLG_SIZEBBOTTOM |
		WFLG_SIMPLE_REFRESH |
		WFLG_REPORTMOUSE |
		0x400000 | /* hm.. no clue what that is.. */
	    WFLG_WBENCHWINDOW,
		NULL,         /* FirstGadget */
		NULL,         /* CheckMark */
		NULL,         /* Title */
		NULL,         /* Screen */
		NULL,         /* BitMap */
		90,           /* MinWidth */
		40,           /* MinHeight */
		65535,        /* MaxWidth */
		65535,        /* MaxHeight */
		WBENCHSCREEN, /* Type */
	},
	0,            /* CurrentX */
	0,            /* CurrentY */
	NULL,         /* Flags */
	0             /* ViewModes */
};


/*
 * Default Disk
 */
struct KnownDiskObject defdisk = {
	{
	    WB_DISKMAGIC,
		WB_DISKVERSION,
		{ /* do_Gadget */
			NULL, /* NextGadget */
			0,    /* LeftEdge */
			0,    /* TopEdge */
			35,   /* Width */
			18,   /* Height */
			GFLG_GADGIMAGE | GFLG_GADGBACKFILL, /* Flags */
			GACT_RELVERIFY | GACT_IMMEDIATE,    /* Activation */
			GTYP_BOOLGADGET,                   /* GadgetType */
			(APTR)&default_disk_image,          /* GadgetRender */
			NULL, /* SelectRender */
			NULL, /* GadgetText */
			0,    /* MutualExclude */
			NULL, /* SpecialInfo */
			0,    /* GadgetID */
			NULL, /* UserData */
		},
		WBDISK,           /* do_Type */
		NULL,             /* do_DefaultTool */
		NULL,             /* do_ToolTypes */
		NO_ICON_POSITION, /* do_CurrentX */
		NO_ICON_POSITION, /* do_CurrentY */
		&defdrawerdata,   /* do_DrawerData */
		NULL,             /* do_ToolWindow */
		0                 /* do_StackSize */
	},
	NULL
};


/*
 * Default Drawer
 */
struct KnownDiskObject defdrawer = {
	{
	    WB_DISKMAGIC,
		WB_DISKVERSION,
		{ /* do_Gadget */
			NULL, /* NextGadget */
			0,    /* LeftEdge */
			0,    /* TopEdge */
			57,   /* Width */
			14,   /* Height */
			GFLG_GADGIMAGE | GFLG_GADGBACKFILL, /* Flags */
			GACT_RELVERIFY | GACT_IMMEDIATE,    /* Activation */
			GTYP_BOOLGADGET,                   /* GadgetType */
			(APTR)&default_drawer_image,        /* GadgetRender */
			NULL, /* SelectRender */
			NULL, /* GadgetText */
			0,    /* MutualExclude */
			NULL, /* SpecialInfo */
			0,    /* GadgetID */
			NULL, /* UserData */
		},
		WBDRAWER,         /* do_Type */
		NULL,             /* do_DefaultTool */
		NULL,             /* do_ToolTypes */
		NO_ICON_POSITION, /* do_CurrentX */
		NO_ICON_POSITION, /* do_CurrentY */
		&defdrawerdata,   /* do_DrawerData */
		NULL,             /* do_ToolWindow */
		0                 /* do_StackSize */
	},
	NULL
};


/*
 * Default Tool
 */
struct KnownDiskObject deftool = {
	{
	    WB_DISKMAGIC,
		WB_DISKVERSION,
		{ /* do_Gadget */
			NULL, /* NextGadget */
			0,    /* LeftEdge */
			0,    /* TopEdge */
			54,   /* Width */
			23,   /* Height */
			GFLG_GADGIMAGE | GFLG_GADGBACKFILL, /* Flags */
			GACT_RELVERIFY,                     /* Activation */
			GTYP_BOOLGADGET,                    /* GadgetType */
			(APTR)&default_tool_image,          /* GadgetRender */
			NULL, /* SelectRender */
			NULL, /* GadgetText */
			0,    /* MutualExclude */
			NULL, /* SpecialInfo */
			0,    /* GadgetID */
			NULL, /* UserData */
		},
		WBTOOL,           /* do_Type */
		NULL,             /* do_DefaultTool */
		NULL,             /* do_ToolTypes */
		NO_ICON_POSITION, /* do_CurrentX */
		NO_ICON_POSITION, /* do_CurrentY */
		NULL,             /* do_DrawerData */
		NULL,             /* do_ToolWindow */
		4096              /* do_StackSize */
	},
	NULL
};


/*
 * Default Project
 */
struct KnownDiskObject defproject = {
	{
	    WB_DISKMAGIC,
		WB_DISKVERSION,
		{ /* do_Gadget */
			NULL, /* NextGadget */
			0,    /* LeftEdge */
			0,    /* TopEdge */
			54,   /* Width */
			23,   /* Height */
			GFLG_GADGIMAGE | GFLG_GADGBACKFILL, /* Flags */
			GACT_RELVERIFY,                     /* Activation */
			GTYP_BOOLGADGET,                    /* GadgetType */
			(APTR)&default_project_image,       /* GadgetRender */
			NULL, /* SelectRender */
			NULL, /* GadgetText */
			0,    /* MutualExclude */
			NULL, /* SpecialInfo */
			0,    /* GadgetID */
			NULL, /* UserData */
		},
		WBPROJECT,         /* do_Type */
		NULL,             /* do_DefaultTool */
		NULL,             /* do_ToolTypes */
		NO_ICON_POSITION, /* do_CurrentX */
		NO_ICON_POSITION, /* do_CurrentY */
		NULL,             /* do_DrawerData */
		NULL,             /* do_ToolWindow */
		4096              /* do_StackSize */
	},
	NULL
};


/*
 * Default Trashcan
 */
struct KnownDiskObject deftrashcan = {
	{
	    WB_DISKMAGIC,
		WB_DISKVERSION,
		{ /* do_Gadget */
			NULL, /* NextGadget */
			0,    /* LeftEdge */
			0,    /* TopEdge */
			51,   /* Width */
			31,   /* Height */
			GFLG_GADGIMAGE | GFLG_GADGBACKFILL, /* Flags */
			GACT_RELVERIFY | GACT_IMMEDIATE,    /* Activation */
			GTYP_BOOLGADGET,                    /* GadgetType */
			(APTR)&default_image,               /* GadgetRender */
			NULL, /* SelectRender */
			NULL, /* GadgetText */
			0,    /* MutualExclude */
			NULL, /* SpecialInfo */
			0,    /* GadgetID */
			NULL, /* UserData */
		},
		WBGARBAGE,        /* do_Type */
		NULL,             /* do_DefaultTool */
		NULL,             /* do_ToolTypes */
		NO_ICON_POSITION, /* do_CurrentX */
		NO_ICON_POSITION, /* do_CurrentY */
		&defdrawerdata, /* do_DrawerData */
		NULL,               /* do_ToolWindow */
		0                   /* do_StackSize */
	},
	NULL
};


/*
 * Default Kick
 */
struct KnownDiskObject defkick = {
	{
	    WB_DISKMAGIC,
		WB_DISKVERSION,
		{ /* do_Gadget */
			NULL, /* NextGadget */
			0,    /* LeftEdge */
			0,    /* TopEdge */
			35,   /* Width */
			18,   /* Height */
			GFLG_GADGIMAGE | GFLG_GADGBACKFILL, /* Flags */
			GACT_RELVERIFY | GACT_IMMEDIATE,    /* Activation */
			GTYP_BOOLGADGET,                    /* GadgetType */
			(APTR)&default_kick_image,          /* GadgetRender */
			NULL, /* SelectRender */
			NULL, /* GadgetText */
			0,    /* MutualExclude */
			NULL, /* SpecialInfo */
			0,    /* GadgetID */
			NULL, /* UserData */
		},
		WBKICK,           /* do_Type */
		NULL,             /* do_DefaultTool */
		NULL,             /* do_ToolTypes */
		NO_ICON_POSITION, /* do_CurrentX */
		NO_ICON_POSITION, /* do_CurrentY */
		&defdrawerdata,   /* do_DrawerData */
		NULL,             /* do_ToolWindow */
		0                 /* do_StackSize */
	},
	NULL
};


/*
 * Default Device
 */
struct KnownDiskObject defdevice = {
	{
	    WB_DISKMAGIC,
		WB_DISKVERSION,
		{ /* do_Gadget */
			NULL, /* NextGadget */
			0,    /* LeftEdge */
			0,    /* TopEdge */
			35,   /* Width */
			18,   /* Height */
			GFLG_GADGIMAGE | GFLG_GADGBACKFILL, /* Flags */
			GACT_RELVERIFY | GACT_IMMEDIATE,    /* Activation */
			GTYP_BOOLGADGET,                    /* GadgetType */
			(APTR)&default_disk_image,          /* GadgetRender */
			NULL, /* SelectRender */
			NULL, /* GadgetText */
			0,    /* MutualExclude */
			NULL, /* SpecialInfo */
			0,    /* GadgetID */
			NULL, /* UserData */
		},
		WBDEVICE,         /* do_Type */
		NULL,             /* do_DefaultTool */
		NULL,             /* do_ToolTypes */
		NO_ICON_POSITION, /* do_CurrentX */
		NO_ICON_POSITION, /* do_CurrentY */
		&defdrawerdata,   /* do_DrawerData */
		NULL,             /* do_ToolWindow */
		0                 /* do_StackSize */
	},
	NULL
};

