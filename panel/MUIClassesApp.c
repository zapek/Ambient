#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <proto/exec.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>
#include <exec/lists.h>
#include "debug.h"

static struct List mcclist;
struct Library *MUIMasterBase;

void init(VOID);
void  fail(APTR app,char *str);

void InitClasses(void);
void RemClasses();
struct MUI_CustomClass *GetMUIClass(STRPTR classname);
struct IClass *GetClass(STRPTR classname);

struct MUI_CustomClass *Application_Class(void);
struct MUI_CustomClass *PObject_Class(void);
struct MUI_CustomClass *DefaultWindowClass_Class(void);
struct MUI_CustomClass *DefaultGroupClass_Class(void);
struct MUI_CustomClass *BaseButtonClass_Class(void);
struct MUI_CustomClass *DragButtonClass_Class(void);
struct MUI_CustomClass *CommandButtonClass_Class(void);
struct MUI_CustomClass *SubPanelButtonClass_Class(void);
struct MUI_CustomClass *SubPanelWindowClass_Class(void);
struct MUI_CustomClass *SpacerClass_Class(void);
struct MUI_CustomClass *SeparatorClass_Class(void);
struct MUI_CustomClass *ViewWatcherClass_Class(void);
struct MUI_CustomClass *BookmarksClass_Class(void);



struct mcc_node
{
	struct Node n;
	struct MUI_CustomClass *mcc;
};


static void AddCustomClass(STRPTR classname, struct MUI_CustomClass *mcc)
{
	if(mcc)
	{
		struct mcc_node *node = (struct mcc_node*)malloc(sizeof (struct mcc_node));
		node->n.ln_Name = classname;
		node->mcc = mcc;
		ADDHEAD(&mcclist,node);
	//	PDB(("addclass %x %s\n",mcc,classname));
	}
}

void InitClasses(void)
{
	NEWLIST(&mcclist);
	AddCustomClass("Application",Application_Class());
	AddCustomClass("DefaultWindow",DefaultWindowClass_Class());
	AddCustomClass("DefaultGroup",DefaultGroupClass_Class());
	AddCustomClass("BaseButton",BaseButtonClass_Class());	
	AddCustomClass("DragButton",DragButtonClass_Class());
	AddCustomClass("CommandButton",CommandButtonClass_Class());
	AddCustomClass("SubPanelButton",SubPanelButtonClass_Class());
	AddCustomClass("SubPanelWindow",SubPanelWindowClass_Class());
	AddCustomClass("Spacer",SpacerClass_Class());
	AddCustomClass("Separator",SeparatorClass_Class());
	AddCustomClass("ViewWatcher",ViewWatcherClass_Class());
	AddCustomClass("Bookmarks",BookmarksClass_Class());
}

void RemClasses()
{
	struct mcc_node *node;
	for(node = (struct mcc_node *)mcclist.lh_Head;node->n.ln_Succ;node = (struct mcc_node*)node->n.ln_Succ)
	{
		MUI_DeleteCustomClass(node->mcc);
	}

}

struct IClass *GetClass(STRPTR classname)
{
	struct mcc_node *node;
	if((node = (struct mcc_node*)FindName(&mcclist,classname)))
	{
		return node->mcc->mcc_Class;
	}
//	PDB(("GetClass failed\n"));
	return NULL;
}
  


struct MUI_CustomClass *GetMUIClass(STRPTR classname)
{
	struct mcc_node *node;
	if((node = (struct mcc_node*)FindName(&mcclist,classname)))
	{
		return node->mcc;
	}
	//PDB(("GetClass failed\n"));
	return NULL;
}


void  fail(APTR app,char *str)
{
		if (app)
				MUI_DisposeObject((Object*)app);

	#ifndef _DCC
   if (MUIMasterBase)
		CloseLibrary(MUIMasterBase);
	#endif

		if (str)
		{
				puts(str);
				exit(20);
		}
		exit(0);
}

void init(VOID)
{
	#ifdef _DCC
	onbreak(brkfunc);
	#endif

	#ifndef _DCC
	if (!(MUIMasterBase = OpenLibrary(MUIMASTER_NAME,MUIMASTER_VMIN)))
		fail(NULL,"Failed to open "MUIMASTER_NAME".");
	#endif
	InitClasses();
}

