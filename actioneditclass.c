/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2006 Ambient Open Source Team
 * actioneditclass.c, Copyright 2005-2006 by Adam Waldenberg
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
 * $Id: actioneditclass.c,v 1.8 2025/09/09 12:46:45 jacadcaps Exp $
 */

/*
 * Internal actions for icon actions:
 * IconInfo, LoadURI, Makedir, MakeLink, Rename, Select,
 * Move, Copy, LoadBackground
 */

/* public */
#include <dos/rdargs.h>
#include <libraries/asl.h>
#include <proto/dos.h>

/* private */
#include "ambient.h"
#include "action.h"
#include "appclass.h"
#include "command.h"
#include "descsaver.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "imagecache.h"
#include "mimetype.h"
#include "mui_func.h"
#include "rexx.h"
#include "screen.h"

#include "actioneditor_dnd_logo.h"
#include "actioneditor_menu_logo.h"

#define CHECKMARK(a, b, c) \
Child, HGroup, Child, HGroup, Child, a = CheckMark(c), End, \
Child, TextObject, MUIA_Text_Contents, b, End, Child, HSpace(0), End

#define IMGBUTTON(a, b) \
Child, a = ImageObject, MUIA_Background, MUII_ButtonBack, ButtonFrame, \
MUIA_Image_FontMatchHeight, TRUE, MUIA_Image_Spec, b, MUIA_InputMode, \
MUIV_InputMode_RelVerify, MUIA_Image_FreeHoriz, TRUE, End

#define GETTAIL(_l)  \
({ struct List *l = (struct List *)(_l);  \
	l->lh_TailPred->ln_Pred ? l->lh_TailPred : (struct Node *)0;  \
})

#define COMMANDS_SIZE 10

/*
 * Wee keep this one static for now...
 */
#define COMMAND_IMGSIZE 48

typedef struct {
	const struct ambient_command *c;
	CONST_STRPTR name, description;
	ULONG set;
	APTR bitmap, grp;
} Command;

struct Data {
	struct internal_mimetype_node *mime_node;
	ULONG commandset;
	APTR commandwin, actionstring;
	APTR commandgrp, argstr;
	APTR opencmd_bt, selectcmd_bt;

	struct {
		APTR command, action;
	} list;
	
	struct {
		APTR cdd, cds, quote;
	} checkmark;

	struct {
		APTR grp, popasl, popstr;
	} commandwidgets;

	struct {
		APTR up, down, addcmd, remcmd, addaction, remaction;
	} action;

	struct {
		APTR dnd_o, dnd_bmp, dnd_list, menu_o, menu_bmp, menu_list;
	} images;

	Command command[COMMANDS_SIZE];
};

typedef struct {
	APTR action_node;
	struct Data *data;
	TEXT string[256]; /* used by hooks */
} Action_Container;

Command *commandlist;

MUI_HOOK(ac_constructfunc, APTR pool, APTR *acarray)
{
	Action_Container *acont;
	   
	if ((acont = AllocPooled(pool, sizeof(Action_Container))))
	{
		acont->action_node = *acarray++;
		acont->data = *acarray;
	}

	D(MIMEACTIONED, bug("Created list object (%lx, ->action_node: %lx, ->data: %lx)!\n",
		acont, acont->action_node, acont->data = *acarray)
	);
	return (ULONG) acont;
}

MUI_HOOK(ac_destructfunc, APTR pool, Action_Container *acont)
{
	D(MIMEACTIONED, bug("Free'd list object (%lx)!\n", acont));
	FreePooled(pool, acont, sizeof(Action_Container));
	return 0;
}

MUI_HOOK(ac_displayfunc, STRPTR *str, Action_Container *acont)
{
	STRPTR s = (STRPTR) actionnode_getattr(acont->action_node, ACTIONNODETAG_NAME);
	APTR listimg;
	BOOL bold = FALSE;

	switch ((ULONG) actionnode_getattr(acont->action_node, ACTIONNODETAG_EVENT))
	{
		case ACTION_EVENT_DOUBLECLICK:
			bold = TRUE;
		case ACTION_EVENT_MENU:
			listimg = acont->data->images.menu_list;
			break;
		case ACTION_EVENT_DRAGNDROP:
			listimg = acont->data->images.dnd_list;
			break;
		default: /* ACTION_EVENT_NONE */
			listimg = NULL;
			break;
	}

	snprintf(acont->string, sizeof(acont->string), "\33O[%08lx] %s%s",
		(ULONG) listimg, bold ? "\33b" : "", s
	);

	D(MIMEACTIONED, bug("Displaying string: (%s)!\n", acont->string));
	*str = acont->string;
	return 0;
}

MUI_HOOK(ac_comparefunc, Action_Container *acont1, Action_Container *acont2)
{
	STRPTR str1 = (STRPTR) actionnode_getattr(acont1->action_node, ACTIONNODETAG_NAME);
	STRPTR str2 = (STRPTR) actionnode_getattr(acont2->action_node, ACTIONNODETAG_NAME);

	return stricmp(str2, str1);
}

/*
 * Some special handling to put together all commandtypes but AC_INTERNAL
 * into a common group.
 */
inline static CONST_STRPTR identify_command(ULONG type, STRPTR s, APTR cn UNUSED)
{
	int i;

	switch (type)
	{
		case AC_INTERNAL:
			for (i = 0; i < COMMANDS_SIZE; i ++)
			{
				int r = strnicmp(commandlist[i].c->name, s,
					strlen(commandlist[i].c->name)
				);

				if (r == 0)
					return commandlist[i].name;
			}
			break;

		case AC_AMIGADOS:
		case AC_WORKBENCH:
		case AC_SCRIPT:
		case AC_AREXX:
			return commandlist[0].name;
			break;

		#ifdef DEBUG
		default:
			PDB(("unknown cmd type 0x%lx\n", type));
			break;
		#endif
	}

	/*
	 * Should never happen. But you know how it is...
	  */
	return "Unknown";
}

MUI_HOOK(cm_displayfunc, STRPTR *str, APTR cn)
{
	if (!cn)
	{
		*str++ = "Type";
		*str = "Command string";
	}
	else
	{
		ULONG type = (ULONG) commandnode_getattr(cn, COMMANDNODETAG_TYPE);
		STRPTR s = commandnode_getattr(cn, COMMANDNODETAG_COMMAND);
	
		*str++ = (STRPTR)identify_command(type, s, cn);
		*str =  s;
	}

	return 0;
}

static void swap_nodes(struct MinNode *cn)
{
	struct MinNode *pn = cn->mln_Pred;
	struct MinNode *nn = cn->mln_Succ;

	D(MIMEACTIONED, bug("Swapping nodes at: (%lx)!\n", cn));

	if (pn) pn->mln_Succ = nn;
	nn->mln_Pred = pn;
	cn->mln_Succ = nn->mln_Succ;
	nn->mln_Succ = cn;
	cn->mln_Pred = nn;
}

static void set_commands(struct Data *data, APTR obj)
{
	ULONG pattern;
	int i;

	data->command[0].c = &rexxcmds[RXCMD_Run-1];
	data->command[0].name = "Run command";
	data->command[0].description = "Run the specified executable or script.";
	data->command[0].set = MV_Actionedit_Commandset_Normal;	       
	data->command[0].bitmap = imagecache_getbitmap("command_run.png", COMMAND_IMGSIZE);

	data->command[1].c = &rexxcmds[RXCMD_IconInfo-1];
	data->command[1].name = "Open icon information";
	data->command[1].description = "Opens up the icon information window for the specified icon.";
	data->command[1].set = MV_Actionedit_Commandset_Normal;
	data->command[1].bitmap = imagecache_getbitmap("command_iconinfo.png", COMMAND_IMGSIZE);

	data->command[2].c = &rexxcmds[RXCMD_LoadURI-1];
	data->command[2].name = "Open URI (Uniform Resource Identifier)";
	data->command[2].description = "Open specified location or file in ambient.";
	data->command[2].set = MV_Actionedit_Commandset_Normal;
	data->command[2].bitmap = imagecache_getbitmap("command_loaduri.png", COMMAND_IMGSIZE);
	
	data->command[3].c = &rexxcmds[RXCMD_Makedir-1];
	data->command[3].name = "Make directory";
	data->command[3].description = "Creates a directory.";
	data->command[3].set = MV_Actionedit_Commandset_Normal;
	data->command[3].bitmap = imagecache_getbitmap("command_makedir.png", COMMAND_IMGSIZE);
	
	data->command[4].c = &rexxcmds[RXCMD_MakeLink-1];
	data->command[4].name = "Make link";
	data->command[4].description = "Make a link to the specified file or directory.";
	data->command[4].set = MV_Actionedit_Commandset_Normal;
	data->command[4].bitmap = imagecache_getbitmap("command_makelink.png", COMMAND_IMGSIZE);

	data->command[5].c = &rexxcmds[RXCMD_Rename-1];
	data->command[5].name = "Rename";
	data->command[5].description = "Rename the specified file or directory.";
	data->command[5].set = MV_Actionedit_Commandset_Normal;
	data->command[5].bitmap = imagecache_getbitmap("command_rename.png", COMMAND_IMGSIZE);
	
	data->command[6].c = &rexxcmds[RXCMD_Select-1];
	data->command[6].name = "Select";
	data->command[6].description = "Select all icons/files or the specified pattern.";
	data->command[6].set = MV_Actionedit_Commandset_Normal;
	data->command[6].bitmap = imagecache_getbitmap("command_select.png", COMMAND_IMGSIZE);
	
	data->command[7].c = &rexxcmds[RXCMD_Move-1];
	data->command[7].name = "Move";
	data->command[7].description = "Move specified files or directories.";
	data->command[7].set = MV_Actionedit_Commandset_Normal;
	data->command[7].bitmap = imagecache_getbitmap("command_move.png", COMMAND_IMGSIZE);
	
	data->command[8].c = &rexxcmds[RXCMD_Copy-1];
	data->command[8].name = "Copy";
	data->command[8].description = "Copy specified files or directories.";
	data->command[8].set = MV_Actionedit_Commandset_Normal;
	data->command[8].bitmap = imagecache_getbitmap("command_copy.png", COMMAND_IMGSIZE);
	
	data->command[9].c = &rexxcmds[RXCMD_LoadBackground-1];
	data->command[9].name = "Load background";
	data->command[9].description = "Sets the specified image as background for desktop or windows.";
	data->command[9].set = MV_Actionedit_Commandset_Normal;
	data->command[9].bitmap = imagecache_getbitmap("command_loadbackground.png", COMMAND_IMGSIZE);

	switch (data->commandset)
	{
		default: /* MV_Actionedit_Commandset_Normal */
			pattern = MV_Actionedit_Commandset_Normal;
			break;
	}

	/*
	 * Add commandlist to command window.
	 */
	for (i = 0; i < COMMANDS_SIZE; i++)
	{
		if (pattern == data->command[i].set)
		{
			Object *o = HGroup,
				MUIA_UserData, &data->command[i],
				MUIA_InputMode, MUIV_InputMode_Immediate,
				Child, VGroup,
					Child, BitmapObject,
						MUIA_Bitmap_Height, COMMAND_IMGSIZE,
						MUIA_Bitmap_Width, COMMAND_IMGSIZE,
						MUIA_FixHeight, COMMAND_IMGSIZE,
						MUIA_FixWidth,  COMMAND_IMGSIZE,
						MUIA_Bitmap_Bitmap, gfx_bitmap_bm(data->command[i].bitmap),
						MUIA_Bitmap_Alpha, 0xffffffff,
					End,
					Child, VSpace(2),
				End,
				Child, VGroup,
					Child, TextObject,
						MUIA_Text_PreParse, "\033b",
						MUIA_Text_Contents, data->command[i].name,
					End,
					Child, TextObject,
						MUIA_Text_Contents, data->command[i].description,
					End,
					Child, VSpace(0),
				End,
			End;

			if (o)
			{
				DoMethod(data->commandgrp, MUIM_Group_InitChange);
				DoMethod(data->commandgrp, OM_ADDMEMBER, o);
				DoMethod(data->commandgrp, MUIM_Group_ExitChange);
				data->command[i].grp = o;

				DoMethod(o, MUIM_Notify, MUIA_Selected, TRUE,
					obj, 3, MM_Actionedit_ChangeCommand, o, TRUE
				);
			}
		}
	}
}

static BOOL set_img(APTR *muio, APTR *bitmap, APTR img, int w, int h)
{
	*bitmap = gfx_bitmap_create(w, h, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE);

	if (*bitmap)
	{
		gfx_blit(img, *bitmap, BLITTAG_SrcType, BLITVAL_SrcType_Array,
			BLITTAG_SrcFormat, BLITVAL_SrcFormat_ARGB, TAG_DONE
		);

		*muio = BitmapObject,
			MUIA_Bitmap_Width, w, MUIA_Bitmap_Height, h,
			MUIA_FixHeight, h, MUIA_FixWidth, w,
			MUIA_Bitmap_Bitmap, gfx_bitmap_bm(*bitmap),
			MUIA_Bitmap_Alpha, 0xffffffff,
		End;

		return *muio ? TRUE : FALSE;
	}

	return FALSE;
}

static void set_mainmethods(struct Data *data, APTR obj)
{
	DoMethod(data->opencmd_bt, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 1, MM_Actionedit_OpenCommands
	);

	if (DoMethod(obj, MM_Actionedit_GetActions))
	{
		DoMethod(data->list.action, MUIM_Notify, MUIA_List_Active,
			MUIV_EveryTime, obj, 1, MM_Actionedit_ShowAction
		);

		DoMethod(data->list.action, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
			data->list.command, 3, MUIM_Set, MUIA_List_Active, MUIV_List_Active_Top
		);

		DoMethod(data->list.command, MUIM_Notify, MUIA_List_Active,
			MUIV_EveryTime, obj, 1, MM_Actionedit_SelectCommand
		);

		set(data->list.action, MUIA_List_Active, MUIV_List_Active_Top);
	}

	DoMethod(data->action.up, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_Actionedit_MoveCommand, -1L
	);

	DoMethod(data->action.down, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_Actionedit_MoveCommand, 1L
	);

	DoMethod(data->action.addaction, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 1, MM_Actionedit_AddAction
	);

	DoMethod(data->action.remaction, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 1, MM_Actionedit_RemAction
	);

	DoMethod(data->action.addcmd, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 1, MM_Actionedit_AddCommand
	);

	DoMethod(data->action.remcmd, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 1, MM_Actionedit_RemCommand
	);

	DoMethod(data->argstr, MUIM_Notify, MUIA_Argstring_Contents, MUIV_EveryTime,
		obj, 2, MM_Actionedit_SetCommand, MUIV_TriggerValue
	);

	DoMethod(data->commandwidgets.popstr, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
		obj, 2, MM_Actionedit_SetCommand, MUIV_TriggerValue
	);
}

DEFNEW
{
//	ULONG commandset;
	APTR listobj, ac_listobj, c_cdd, c_cds, c_quote, commandwin, commandgrp, commandwidgetsgrp;
	APTR argstr, opencmd_bt, ac_up, ac_down, ac_addcmd, ac_remcmd, ac_addaction, ac_remaction;
	APTR popstr, popasl, actionstring;
	struct internal_mimetype_node *mime_node = NULL;
	struct Data *data;

	static STRPTR cyc_commandtypes[] = {"AmigaDOS", "Workbench", "Shell script", "Arexx", NULL};

	FORTAG(INITTAGS)
	{
		case MA_Actionedit_MimeNode:
			mime_node = (APTR) tag->ti_Data;
			break;

//		case MA_Actionedit_Commandset:
//			commandset = tag->ti_Data;    /* why? */
	}
	NEXTTAG

	if (!mime_node)
		return (ULONG) obj;

	obj = DoSuperNew(cl, obj,
		MUIA_Group_Horiz, TRUE,
		Child, VGroup,
			MUIA_Weight, 70,
			Child, ac_listobj = ListObject,
				InputListFrame,
				MUIA_List_ConstructHook, (ULONG) &ac_constructfunc_hook,
				MUIA_List_DestructHook, (ULONG) &ac_destructfunc_hook,
				MUIA_List_DisplayHook, (ULONG) &ac_displayfunc_hook,
				MUIA_List_CompareHook, (ULONG) &ac_comparefunc_hook,
				MUIA_List_MinLineHeight, 20,
				MUIA_List_AutoVisible, TRUE,
			End,
			Child, actionstring = StringObject, StringFrame, End,
			Child, HGroup,
				Child, ac_addaction = SimpleButton("Add"),
				Child, ac_remaction = SimpleButton("Remove"),
			End,
		End,
		Child, BalanceObject, End,
		Child, VGroup,
			Child, VGroup,
				GroupFrameT("Command"),
				Child, HGroup,
					Child, listobj = ListObject,
						InputListFrame,
						MUIA_List_DisplayHook, (ULONG) &cm_displayfunc_hook,
						MUIA_List_Format, "C=0 MIW=-1 BAR,C=1 MIW=-1",
						MUIA_List_Title, TRUE,
						MUIA_List_MinLineHeight, 20,
					MUIA_List_AutoVisible, TRUE,
					End,
					Child, VGroup,
						MUIA_Weight, 1,
						Child, ac_addcmd = SimpleButton("Add"),
						Child, ac_remcmd = SimpleButton("Remove"),
						Child, HGroup,
							IMGBUTTON(ac_up, MUII_TapeUp),
							IMGBUTTON(ac_down, MUII_TapeDown),
						End,
						Child, VSpace(0),
					End,
				End,
				Child, HGroup,
					Child, opencmd_bt = SimpleButton("Select new command"),
					Child, HSpace(0),
				End,
				Child, RectangleObject,
					MUIA_Rectangle_HBar, TRUE,
					MUIA_Weight, 0L,
				End,
				Child, argstr = MUI_NewObject("Argstring.mui",
					MUIA_Weight, 0L,
					MUIA_ShowMe, FALSE,
				TAG_DONE),
				Child, commandwidgetsgrp = HGroup,
					InnerSpacing(0,0),
					MUIA_ShowMe, FALSE,
					MUIA_Weight, 0L,
					Child, CycleObject,
						MUIA_Weight, 0L,
						MUIA_Cycle_Entries, cyc_commandtypes,
					End,
					Child, popasl = PopaslObject,
						MUIA_Popasl_Type, ASL_FileRequest,
						MUIA_Popstring_Button, PopButton(MUII_PopFile),
						MUIA_Popstring_String, popstr = StringObject,
							StringFrame,
							MUIA_String_MaxLen,512,
						End,
					End,
				End,
			End,
			Child, VGroup,
				CHECKMARK(c_cds, "Change working directory to source", FALSE),
				CHECKMARK(c_cdd, "Change working directory to destination", FALSE),
				CHECKMARK(c_quote, "Quote arguments", TRUE),
			End,
		End,
		TAG_MORE, INITTAGS,
	End;

	commandwin = WindowObject,
		MUIA_Window_PublicScreen, active_screen_name(),
		MUIA_Window_ScreenTitle, screentitle,
		MUIA_Window_Title, "Select a command",
		MUIA_Window_LeftEdge, MUIV_Window_LeftEdge_Moused,
		MUIA_Window_TopEdge, MUIV_Window_TopEdge_Moused,
		MUIA_Window_ShowIconify, FALSE,
		MUIA_Window_ShowPrefs, FALSE,
		MUIA_Window_ShowJump, FALSE,
		MUIA_Window_ShowAbout, FALSE,
		MUIA_Window_NoMenus, TRUE,
		MUIA_Window_Height, 500,
		WindowContents, VGroup,
			Child, ScrollgroupObject,
				MUIA_Scrollgroup_Contents, commandgrp = VGroupV,
					MUIA_Background, MUII_ListBack,
					ReadListFrame,
				End,
			End,
		End,
	End;

	/*
	 * We need a mimetype to continue here...
	 */
	if (obj && commandwin)
	{
		data = INST_DATA(cl, obj);
		data->mime_node = mime_node;
		data->list.command = listobj;
		data->list.action = ac_listobj;
		data->checkmark.cds = c_cds;
		data->checkmark.cdd = c_cdd;
		data->checkmark.quote = c_quote;
		data->action.up = ac_up;
		data->action.down = ac_down;
		data->action.addcmd = ac_addcmd;
		data->action.remcmd = ac_remcmd;
		data->action.addaction = ac_addaction;
		data->action.remaction = ac_remaction;
		data->commandwin = commandwin;
		data->commandgrp = commandgrp;
		data->opencmd_bt = opencmd_bt;
		data->commandwidgets.grp = commandwidgetsgrp;
		data->commandwidgets.popstr = popstr;
		data->commandwidgets.popasl = popasl;
		data->actionstring = actionstring;
		data->argstr = argstr;
		commandlist = &data->command[0];
		
		data->images.dnd_list = data->images.menu_list = NULL;

		set_img(&data->images.dnd_o, &data->images.dnd_bmp, &actioneditor_dnd,
			ACTIONEDITOR_DND_WIDTH, ACTIONEDITOR_DND_HEIGHT
		);
		
		set_img(&data->images.menu_o, &data->images.menu_bmp, &actioneditor_menu,
			ACTIONEDITOR_MENU_WIDTH, ACTIONEDITOR_MENU_HEIGHT
		);

		/*
		 * Woopsie.. Seems we are bailing out---?
		 */
		if (data->images.dnd_o == NULL || data->images.dnd_bmp == NULL ||
		    data->images.menu_o == NULL || data->images.menu_bmp == NULL)
		{
			PDB(("Something went dead wrong when creating listicons...\n"));

			if (data->images.dnd_o)
				MUI_DisposeObject(data->images.dnd_o);

			if (data->images.menu_o)
				MUI_DisposeObject(data->images.menu_o);

			if (data->images.dnd_bmp)
				gfx_bitmap_delete(data->images.dnd_bmp);

			if (data->images.menu_bmp)
				gfx_bitmap_delete(data->images.menu_bmp);

			MUI_DisposeObject(commandwin);
			MUI_DisposeObject(obj);

			return ((ULONG) NULL);
		}
		
		set_commands(data, obj);
		set_mainmethods(data, obj);
	}
	else if (commandwin)
	{
		MUI_DisposeObject(commandwin);
	}

	return (ULONG) obj;
}

DEFDISPOSE
{
	GETDATA;

	if (data->commandwin)
		MUI_DisposeObject(data->commandwin);

	MUI_DisposeObject(data->images.dnd_o);
	MUI_DisposeObject(data->images.menu_o);

	gfx_bitmap_delete(data->images.dnd_bmp);
	gfx_bitmap_delete(data->images.menu_bmp);

	return DOSUPER;
}

DEFMMETHOD(Setup)
{
	ULONG rc = DOSUPER;
	GETDATA;

	if (rc && _win(obj))
	{
		data->images.dnd_list = (APTR) DoMethod(data->list.action, MUIM_List_CreateImage,
			data->images.dnd_o, 0L
		);
		
		data->images.menu_list = (APTR) DoMethod(data->list.action, MUIM_List_CreateImage,
			data->images.menu_o, 0L
		);
	}

	return rc;
}

DEFMMETHOD(Cleanup)
{
	GETDATA;

	DoMethod(data->list.action, MUIM_List_DeleteImage, data->images.dnd_list);
	DoMethod(data->list.action, MUIM_List_DeleteImage, data->images.menu_list);

	return DOSUPER;
}

DEFTMETHOD(Actionedit_OpenCommands)
{
	GETDATA;
	
	DoMethod(data->commandwin, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
		obj, 1, MM_Actionedit_CloseCommands
	);

	DoMethod(app, OM_ADDMEMBER, data->commandwin);
	set(data->commandwin, MUIA_Window_Open, TRUE);

	return 0;
}

DEFTMETHOD(Actionedit_CloseCommands)
{
	GETDATA;

	D(MIMEACTIONED, bug("Closing commands window at: (%lx)!\n", data->commandwin));

	if (getv(data->commandwin, MUIA_Window_Open))
	{
		set(data->commandwin, MUIA_Window_Open, FALSE);
		DoMethod(app, OM_REMMEMBER, data->commandwin);
	}

	return 0;
}

DEFSMETHOD(Actionedit_ChangeCommand)
{
	GETDATA;
	APTR cn;
	Command *c = (Command *) getv(msg->selobj, MUIA_UserData);
	STRPTR s;

	DoMethod(data->list.command, MUIM_List_GetEntry,
		MUIV_List_GetEntry_Active, &cn
	);

	if(cn)
	{
		DoMethod(data->commandgrp, MUIM_Group_InitChange);
		DoMethod(msg->selobj, MUIM_NoNotifySet, MUIA_Selected, TRUE);

		FORCHILD(data->commandgrp, MUIA_Group_ChildList)
		{
			if (getv(child, MUIA_Selected) && child != msg->selobj)
				set(child, MUIA_Selected, FALSE);
		}
		NEXTCHILD

		DoMethod(data->commandgrp, MUIM_Group_ExitChange);

		if (msg->modifylist && (s = commandnode_getattr(cn, COMMANDNODETAG_COMMAND)))
		{
			TEXT txt[strlen(c->c->name) + strlen(s) + 2];
			TEXT cmdstr[32];
			ULONG oldtype = (ULONG) commandnode_getattr(cn, COMMANDNODETAG_TYPE);

			if (c->c->id == RXCMD_Run)
				commandnode_setattrs(cn, COMMANDNODETAG_TYPE, AC_AMIGADOS, TAG_DONE);
			else
				commandnode_setattrs(cn, COMMANDNODETAG_TYPE, AC_INTERNAL, TAG_DONE);

			if (oldtype != AC_INTERNAL)
			{
				sprintf(txt, "%s %s", c->c->name, s);
			}
			else if (c->c->id != RXCMD_Run)
			{
				sscanf(s, "%s", cmdstr);

				if (strlen(cmdstr) < strlen(s))
					sprintf(txt, "%s %s", c->c->name, s + strlen(cmdstr) + 1);
				else
					strcpy(txt, c->c->name);
			}
			else
			{
				sscanf(s, "%s", cmdstr);

				if (strlen(cmdstr) < strlen(s))
					strcpy(txt, s + strlen(cmdstr) + 1);
				else
					strcpy(txt, "");

			}

			D(MIMEACTIONED, bug("Generated commandstring: (%s)!\n", txt));
			commandnode_setattrs(cn, COMMANDNODETAG_COMMAND, txt, TAG_DONE);
			DoMethod(data->list.command, MUIM_List_Redraw, MUIV_List_Redraw_Active);
		}

		/*
		 * Seems we should use our own constructed GUI elements...
		 */
		if (c->c->id == RXCMD_Run)
		{
			set(data->argstr, MUIA_ShowMe, FALSE);
			set(data->commandwidgets.grp, MUIA_ShowMe, TRUE);
		}
		/*
		 * Here the Argtring class is used to dynamically construct widgets
		 * assoicated to the command template!
		 */
		else
		{
			set(data->argstr, MUIA_Argstring_Template, c->c->id == RXCMD_Run ?
				(STRPTR) "" : c->c->args
			);

			set(data->commandwidgets.grp, MUIA_ShowMe, FALSE);
			set(data->argstr, MUIA_ShowMe, TRUE);
		}

		if ((s = commandnode_getattr(cn, COMMANDNODETAG_COMMAND)))
		{
			set(data->commandwidgets.popstr, MUIA_String_Contents, s);
			set(data->argstr, MUIA_Argstring_Contents, s);
		}
	}

	return 0;
}

/*
 * Here we set all the widgets in the actioneditor so they represent
 * the actual pressed command. (Also via Actionedit_ChangeCommand)
 */
DEFTMETHOD(Actionedit_SelectCommand)
{
	GETDATA;
	APTR cn;
	int i = -1;

	DoMethod(data->list.command, MUIM_List_GetEntry,
		MUIV_List_GetEntry_Active, &cn
	);

	if (cn)
	{
		ULONG type = (ULONG) commandnode_getattr(cn, COMMANDNODETAG_TYPE);
		STRPTR s = commandnode_getattr(cn, COMMANDNODETAG_COMMAND);

		for (i = 0; i < COMMANDS_SIZE; i++)
			if (stricmp(identify_command(type, s, cn), commandlist[i].name) == 0)
				break;

		DoMethod(obj, MM_Actionedit_ChangeCommand, commandlist[i].grp, FALSE);
	}
	else return FALSE;

	return TRUE;
}


DEFSMETHOD(Actionedit_SetCommand)
{
	GETDATA;
	APTR cn;
	STRPTR s;

	DoMethod(data->list.command, MUIM_List_GetEntry,
		MUIV_List_GetEntry_Active, &cn
	);

	if (msg->cmd && (s = malloc(strlen(msg->cmd) + 1)))
	{
		strcpy(s, msg->cmd);
		commandnode_setattrs(cn, COMMANDNODETAG_COMMAND, s, TAG_DONE);
		DoMethod(data->list.command, MUIM_List_Redraw, MUIV_List_Redraw_Active);	
		free(s);
	}

	return 0;
}

DEFTMETHOD(Actionedit_GetActions)
{
	GETDATA;
	APTR an;
	int i = 0;

	DoMethod(data->list.action, MUIM_List_Clear);

	if (!ISLISTEMPTY( data->mime_node->action_list ))
	{
		ITERATELIST(an, data->mime_node->action_list )
		{
			static APTR acarray[2];

			acarray[0] = an;
			acarray[1] = data;

			DoMethod(data->list.action, MUIM_List_InsertSingle,
				&acarray, MUIV_List_Insert_Sorted
			);

			i++;
		}
	}

	return i;
}

/*
 * Fetches all commands defined for the currently chosen action
 * (data->list.action).
 */
DEFTMETHOD(Actionedit_ShowAction)
{
	GETDATA;
	Action_Container *acont;
	int i = 0;

	DoMethod(data->list.command, MUIM_List_Clear);

	if (DoMethod(data->list.action, MUIM_List_GetEntry, MUIV_List_GetEntry_Active , &acont))
	{
		APTR commandlist = (APTR) actionnode_getattr(acont->action_node, ACTIONNODETAG_COMMAND_LIST);
		STRPTR name = (STRPTR) actionnode_getattr(acont->action_node, ACTIONNODETAG_NAME);
		APTR cn;

		set(data->actionstring, MUIA_String_Contents, name);

		if (!ISLISTEMPTY(commandlist))
		{
			ITERATELIST(cn, commandlist)
			{
				DoMethod(data->list.command, MUIM_List_InsertSingle,
					cn, MUIV_List_Insert_Bottom
				);

				i++;
			}
		}

	}

	return i;
}

DEFTMETHOD(Actionedit_AddAction)
{
	GETDATA;
	APTR an;

	if ((an = actionnode_create()))
	{
		static APTR acarray[2];

		acarray[0] = an;
		acarray[1] = data;

		D(MIMEACTIONED, bug("Adding action... (actionnode: %lx, action_list: %lx)!\n",
			acarray[0], data->mime_node->action_list)
		);

		actionnode_setattrs(an, ACTIONNODETAG_NAME, "Specify action description here.",
			ACTIONNODETAG_EVENT, ACTION_EVENT_MENU,
		TAG_DONE
		);

		ADDTAIL( data->mime_node->action_list, an);

		DoMethod(data->list.action, MUIM_List_InsertSingle,
			&acarray, MUIV_List_Insert_Sorted
		);
	}
	else return FALSE;

	return TRUE;
}

DEFTMETHOD(Actionedit_RemAction)
{
	GETDATA;
	Action_Container *acont;

	DoMethod(data->list.action, MUIM_List_GetEntry,
		MUIV_List_GetEntry_Active, &acont
	);

	D(MIMEACTIONED, bug("Removing action... (action_container: %lx, actionnode: %lx)!\n",
		acont, acont->action_node)
	);

	DoMethod(data->list.action, MUIM_List_Remove, MUIV_List_Remove_Active);
	
	REMOVE(acont->action_node);
	actionnode_delete(acont->action_node);
	return 0;
}

DEFTMETHOD(Actionedit_AddCommand)
{
	Action_Container *acont;
	GETDATA;

	DoMethod(data->list.action, MUIM_List_GetEntry,
		MUIV_List_GetEntry_Active, &acont
	);

	if (actionnode_addcommand(acont->action_node, AC_AMIGADOS, "Specify command here."))
	{
		APTR commandlist = (APTR) actionnode_getattr(acont->action_node, ACTIONNODETAG_COMMAND_LIST);
		struct MinNode *cn = (struct MinNode *) GETTAIL(commandlist);

		DoMethod(data->list.command, MUIM_List_InsertSingle,
			cn, MUIV_List_Insert_Bottom
		);
	}
	else return FALSE;

	return TRUE;
}

DEFTMETHOD(Actionedit_RemCommand)
{
	struct MinNode *cn;
	GETDATA;

	DoMethod(data->list.command, MUIM_List_GetEntry,
		MUIV_List_GetEntry_Active, &cn
	);

	DoMethod(data->list.command, MUIM_List_Remove, MUIV_List_Remove_Active);
	actionnode_remcommand(cn);
	
	return 0;
}

DEFSMETHOD(Actionedit_MoveCommand)
{
	struct MinNode *cn;
	GETDATA;

	DoMethod(data->list.command, MUIM_List_GetEntry,
		MUIV_List_GetEntry_Active, &cn
	);

	if (msg->offset == -1)
	{	 
		if (cn->mln_Pred != NULL)
		{
			swap_nodes(cn->mln_Pred);
			DoMethod(data->list.command, MUIM_List_Exchange,
				MUIV_List_Exchange_Active, MUIV_List_Exchange_Previous
			);
			set(data->list.command, MUIA_List_Active, MUIV_List_Active_Up);
		}
	}
	else if (cn->mln_Succ != NULL)
	{
		swap_nodes(cn);
		DoMethod(data->list.command, MUIM_List_Exchange,
			MUIV_List_Exchange_Active, MUIV_List_Exchange_Next
		);
		set(data->list.command, MUIA_List_Active, MUIV_List_Active_Down);
	}

	return 0;
}

BEGINMTABLE
DECNEW
DECDISPOSE
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECTMETHOD(Actionedit_OpenCommands)
DECTMETHOD(Actionedit_CloseCommands)
DECSMETHOD(Actionedit_ChangeCommand)
DECTMETHOD(Actionedit_SelectCommand)
DECSMETHOD(Actionedit_SetCommand)
DECTMETHOD(Actionedit_GetActions)
DECTMETHOD(Actionedit_ShowAction)
DECTMETHOD(Actionedit_AddAction)
DECTMETHOD(Actionedit_RemAction)
DECTMETHOD(Actionedit_AddCommand)
DECTMETHOD(Actionedit_RemCommand)
DECSMETHOD(Actionedit_MoveCommand)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, actioneditclass)
