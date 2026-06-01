#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <proto/exec.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>
#include <exec/lists.h>
//#include "debug.h"

static struct List mcclist;
struct Library *MUIMasterBase;


struct MUI_CustomClass *PanelListtree_Class(void);
struct MUI_CustomClass *DefaultWindowPrefs_Class(void);
struct MUI_CustomClass *BaseButtonPrefs_Class(void);
struct MUI_CustomClass *SliderSize_Class(void);
struct MUI_CustomClass *SliderSpeed_Class(void);
struct MUI_CustomClass *ClassList_Class(void);


struct mcc_node
{
	struct Node n;
	struct MUI_CustomClass *mcc;
};


static void AddCustomClass(STRPTR classname, struct MUI_CustomClass *mcc)
{
	if(mcc)
	{
		struct mcc_node *node = (struct mcc_node*)AllocVecTaskPooled(sizeof (struct mcc_node));
		node->n.ln_Name = classname;
		node->mcc = mcc;
		ADDHEAD(&mcclist,node);
	//	PDB(("addclass %x %s\n",mcc,classname));
	}
}

void InitClasses(void)
{
	NEWLIST(&mcclist);
	AddCustomClass("PanelListtree",PanelListtree_Class());
	AddCustomClass("DefaultWindowPrefs",DefaultWindowPrefs_Class());
	AddCustomClass("BaseButtonPrefs",BaseButtonPrefs_Class());
	AddCustomClass("SliderSize",SliderSize_Class());		
	AddCustomClass("SliderSpeed",SliderSpeed_Class());	
	AddCustomClass("ClassList",ClassList_Class());
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



void init(VOID)
{
	
	InitClasses();
}

