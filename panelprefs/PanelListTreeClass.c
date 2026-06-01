#include "muifuncs.h"

//#include "debug.h" 


#define USE_INLINE_STDARG


#include <exec/libraries.h>
#include <libraries/commodities.h>
#include <dos/dos.h>
#include <clib/exec_protos.h>
#include <proto/asound.h>
#include <clib/alib_protos.h>
#include <clib/commodities_protos.h>
#include <mui/Listtree_mcc.h>
#include <proto/panel.h>
//#include "prefspool.h"
#include "paneltags.h"
#include "prefs.h"
#include "panelitem.h"
//#include "panellib.h"
//#include "debug.h"
#include "MUIClasses_Prefs.h"

#include <proto/ambient.h>
#include <libraries/ambient.h>

#include "locale.h"
#include "panelprefs_cat.h"


#define CATCOMP_NUMBERS
extern const char * const __stringtable[];
#ifdef __SASC
#define GSI(x) __stringtable[x]
#else
#define GSI(x) ( STRPTR )__stringtable[x]
#endif

#warning MSG_PANELWIN_MODES_ATTACHED
#warning MSG_PANELWIN_PLACEMENTH_BEGINNING
#warning MSG_PANELWIN_PLACEMENTH_END
#warning MSG_PANELWIN_DEPTHS_BACK

#define DROP_VOID 	0
#define DROP_BELOW	101
#define DROP_ONTO	102
#define DROP_ABOVE	103

#warning
extern struct Library *PanelBase;
struct Data
{
	ULONG droppos;
	APTR classlist;
};




static void doset( APTR obj, struct Data *data,struct TagItem *tags,BOOL init )
{
	struct TagItem *tstate = tags, *tag;
	ULONG rfr = 0;
	while ((tag = (struct TagItem *) NextTagItem(&tstate)))
	{
		if (init)
		{
			switch (tag->ti_Tag)
			{
			}
		}
		
		switch (tag->ti_Tag)
		{		
			
			case MA_PanelPrefs_ClassListObj:
				data->classlist = (APTR)tag->ti_Data;
				//kprintf("set classlist %x\n",data->classlist);
				break;

		}
	}
}
		




static ULONG mNew(struct IClass *cl,Object *obj,struct opSet *msg)
{
	struct Data *data;
	
	if((obj = DoSuperNew( cl, obj,
			
			InputListFrame,
			MUIA_List_ShowDropMarks, TRUE,
			MUIA_Listview_DragType , MUIV_Listview_DragType_None,//MUIV_Listview_DragType_Immediate,
	
			MUIA_Listtree_DragDropSort,TRUE,
	TAG_MORE,	msg->ops_AttrList,
			TAG_DONE ) ) )
	{
		data = (struct Data*)INST_DATA(cl,obj);
		doset( obj, data,msg->ops_AttrList,TRUE); 
	//	PDB(("\n"));
		
	}
	
	return(ULONG)obj;
}

static ULONG mDispose(struct IClass *cl,Object *obj,Msg msg)
{
	struct Data *data;
	ULONG retval;
	data = (struct Data*)INST_DATA(cl,obj);
	retval = DoSuperMethodA(cl,obj,msg);
	return retval;
}



static ULONG mSet(struct IClass *cl,Object *obj,struct opSet *msg)
{
	struct TagItem *tstate;
    struct Data *data;
	data = (struct Data*)INST_DATA(cl,obj);
	tstate = msg->ops_AttrList;
	doset( obj, data,msg->ops_AttrList,FALSE); 
	return DoSuperMethodA(cl,obj,msg);
}

static ULONG mGet(struct IClass *cl,Object *obj,struct opGet *msg)
{
	//struct Data *data = (struct Data*)INST_DATA(cl,obj);
	/*switch (msg->opg_AttrID)
	{
		case MA_Application_ActiveDisplay:
			*msg->opg_Storage  = (ULONG)data->activedisplay;
			return TRUE;		
	}*/
	return DoSuperMethodA(cl, obj, (Msg)msg);
}

struct SubPanel
{
	ULONG *ID;
	ULONG node;
};

struct SubPanelMarker
{
	APTR parent;
	ULONG node;
};




static ULONG mAddPanel(struct IClass *cl,Object *obj,struct MP_PanelListtree_AddPanel *msg)
{
	ULONG node,i = 0;
	ULONG root_node;
	APTR ppool;
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	//kprintf("addpanel !%s!\n",msg->panelname);
	if((ppool = LockPPool(msg->panelname,PPOOL_TYPE_PANEL)))
	{
		APTR pl, pi;
		ULONG *type;
		ULONG sub_level = 0;
		struct SubPanel sub_panel[10];
	root_node = node = DoMethod(obj,MUIM_Listtree_Insert,msg->panelname, 0, MUIV_Listtree_Insert_ListNode_Root,    MUIV_Listtree_Insert_PrevNode_Head, TNF_OPEN |TNF_LIST | MUIV_Listtree_Insert_Flags_Active);
		
	if((pl = (APTR)GetPPoolItem(ppool,0,  DSI_LISTPOOL_PANEL ,NULL,NULL)))
		{
			while ((pi = (APTR)GetPPoolItem(ppool, pl, i | DSF_LISTPOOL, NULL, NULL)) && GetPPoolItem(ppool, pi, DSI_LISTPOOL_PANEL_TYPE, (APTR)&type, NULL))
			{
				STRPTR object_str = 0;	
				switch (*type)
				{
					case MV_Panel_Type_Spacer:
						object_str = GSI(MSG_PANELITEM_SPACER);
						break;
					case MV_Panel_Type_Separator:
						object_str = GSI(MSG_PANELITEM_SEPARATOR);
						break;
					case MV_Panel_Type_ViewWatcher:
						object_str = "Viewwatcher";
						break;	
					case MV_Panel_Type_Bookmarks:
						object_str = "Bookmarks";
						break;	
					//case MV_Panel_Type_DirPanel:
					case MV_Panel_Type_SubPanel:
						//object_str = GSI(MSG_PANELITEM_SUBPANEL);
						GetPPoolItem(ppool, pi, DSI_LISTPOOL_PANEL_SUBPANEL_ROOT , (APTR) &sub_panel[sub_level].ID, NULL );
						sub_panel[sub_level].node = DoMethod(obj,MUIM_Listtree_Insert,GSI(MSG_PANELITEM_SUBPANEL),i,node,MUIV_Listtree_Insert_PrevNode_Tail,TNF_LIST);
						node = sub_panel[sub_level].node;
						//PDB(("%d %d ",i,*sub_panel[sub_level].ID));
						sub_level++;
						break;
					case MV_Panel_Type_External:
					{
						#warning hardcoded class_name
						STRPTR class_name;
						struct PanelItem *pitem;
						GetPPoolItem(ppool, pi, DSI_LISTPOOL_PANEL_EXT_NAME, (APTR) &class_name , NULL );
						//kprintf("listtree addpanel ecternal 1\n");
						if(DoMethod(data->classlist,MM_ClassListGetExternalObject,class_name,&pitem))
						{
							//kprintf("pi %x\n",pitem);
							object_str = pitem->pi_DisplayName;
						}				
					//	object_str = class_name;						
					//	kprintf("class %x %s\n",data->classlist,class_name);
					//	get( msg->User, MA_Panel_Extern_DisplayName, (ULONG*) &object_str );
						//object_str = "tick";
					}
						break;
					case MV_Panel_Type_Button:
						GetPPoolItem(ppool, pi, DSI_LISTPOOL_PANEL_URI,       (APTR) &object_str , NULL );  
 						break;
					default:
						kprintf("listtree addpanel nop \n");
						break;
				}
				if(object_str) 
				{
					ULONG *sub;
					if((GetPPoolItem(ppool, pi, DSI_LISTPOOL_PANEL_SUBPANEL, (APTR)&sub, NULL ) ) && ( *sub != 0 ) )
					{
					//	PDB(("go to sub %d\n",*sub));
					}
					else
					{
						sub_level = 0;
						node = root_node;
					}
				//	kprintf("addpanel objstr !%s!\n",object_str);
					DoMethod(obj,MUIM_Listtree_Insert,object_str,i,node,MUIV_Listtree_Insert_PrevNode_Tail,0);
				}
				i++;
			}
		//	DoMethod(obj,MM_PanelListTree_Renumber,root_node);
			if((pi = (APTR)GetPPoolItem(ppool, pl, i+1 | DSF_LISTPOOL, NULL, NULL)) && GetPPoolItem(ppool, pi, DSI_LISTPOOL_PANEL_TYPE, (APTR)&type, NULL))
			{
			//	kprintf("not the end %d %d\n",*type,MV_Panel_Type_Button);
			}
		}
		UnLockPPool(ppool);
	}		
	return 0;
}


static ULONG mRebuild(struct IClass *cl,Object *obj,struct MP_PanelListtree_Rebuild *msg)
{
	if(msg->panel_name)
	{
		APTR panel_node;
		TEXT panel_name[300];
		if((panel_node	= (APTR)DoMethod(obj,MUIM_Listtree_FindName, MUIV_Listtree_FindName_ListNode_Root ,msg->panel_name,0)))
		{
			strncpy(panel_name,msg->panel_name,299);
			//kprintf("rebuild !%s!\n",msg->panel_name);
			SetAttrs(obj,MUIA_Listtree_Quiet,TRUE,TAG_DONE);
			DoMethod(obj,MUIM_Listtree_Remove, MUIV_Listtree_Remove_ListNode_Root ,panel_node,0);
			DoMethod(obj,MM_PanelListtree_AddPanel,panel_name);
			SetAttrs(obj,MUIA_Listtree_Quiet,FALSE,TAG_DONE);
		}
		else
		{
		
		#warning wonky rename
			if(0)//(panel_node	= (APTR)DoMethod(obj,MUIM_Listtree_GetEntry,MUIV_Listtree_GetEntry_ListNode_Root, MUIV_Listtree_GetEntry_Position_Active, 0)))
			{
				SetAttrs(obj,MUIA_Listtree_Quiet,TRUE,TAG_DONE);
				DoMethod(obj,MUIM_Listtree_Remove, MUIV_Listtree_Remove_ListNode_Root ,panel_node,0);
				DoMethod(obj,MM_PanelListtree_AddPanel,msg->panel_name);
				SetAttrs(obj,MUIA_Listtree_Quiet,FALSE,TAG_DONE);
			}				
		}
	}
	return 0;
}

#if 0
static ULONG mRenumber(struct IClass *cl,Object *obj,struct MP_PanelListTree_Renumber *msg)
{
	struct Data *data;
	struct MUIS_Listtree_TreeNode *start_node = (struct MUIS_Listtree_TreeNode *)msg->start_node;
	struct MUIS_Listtree_TreeNode *tn = start_node;
	data = (struct Data*)INST_DATA(cl,obj);
	//kprintf("start renumber %x %s\n",start_node->tn_User,start_node->tn_Name);
	while(tn)
	{
		if(tn->tn_Flags & TNF_LIST)
		{
			tn = DoMethod(obj,MUIM_Listtree_GetEntry,tn,  MUIV_Listtree_GetEntry_Position_Head, 0);
		}
		else
		{
			tn = DoMethod(obj,MUIM_Listtree_GetEntry,tn,  MUIV_Listtree_GetEntry_Position_Next, 0);
		}
	//	if(tn)kprintf("renumber %x %s\n",tn->tn_User,tn->tn_Name);
	//	else kprintf("ende\n");
	}
	

	return 0;
}

#endif

static ULONG mDragQuery(struct IClass *cl,Object *obj,struct MUIP_DragQuery *msg)
{
	ULONG x,y;
	APTR *active_node;
	struct MUIS_Listtree_TestPos_Result pos = { NULL, 0, 0, 0 };
	x = _window(obj)->MouseX;
	y = _window(obj)->MouseY;
	active_node =  (APTR) DoMethod( obj, MUIM_Listtree_GetEntry, obj        , MUIV_Listtree_GetEntry_Position_Active );
	if(msg->obj == obj)
	{
		if(( DoMethod( obj, MUIM_Listtree_TestPos, x, y, &pos ) && (pos.tpr_TreeNode != active_node)) ) return( MUIV_DragQuery_Accept );
		else return( MUIV_DragQuery_Refuse );
	}
	return( MUIV_DragQuery_Accept);
}

static ULONG mDragReport(struct IClass *cl,Object *obj,struct MUIP_DragReport *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	struct MUIS_Listtree_TestPos_Result r  = { NULL, 0, 0, 0 };
	struct MUIS_Listtree_TestPos_Result r2 = { NULL, 0, 0, 0 };
	struct MUIS_Listtree_TreeNode *tn;


//PDB(("%d %d \n",msg->x,msg->y));
//	  if(msg->obj == obj)
	{

		/* Autoscroll list, when mouse is in upper or lower area */
		if( _isinobject( msg->x, msg->y ) )
		{
			if( ( msg->y + 10 ) > _top( obj ) + _height( obj ) )
			{
				if( !msg->update )
				{
					return( MUIV_DragReport_Refresh );
				}

				SetAttrs( obj, MUIA_List_TopPixel, getv(obj, MUIA_List_TopPixel ) + 10, TAG_DONE );
				return ( MUIV_DragReport_Continue );
			}
			else if( ( _top( obj ) + 20 ) > msg->y )
			{
				if( getv( obj, MUIA_List_TopPixel ) >= 10 )
				{
					if( !msg->update )
					{
						return( MUIV_DragReport_Refresh );
					}

					SetAttrs( obj, MUIA_List_TopPixel, getv( obj, MUIA_List_TopPixel ) - 10, TAG_DONE );
					return(MUIV_DragReport_Continue );
				}
			}
		}
		else
		{
			return( MUIV_DragReport_Continue );
		}

		// Sort mode
		if( DoMethod( obj, MUIM_Listtree_TestPos, msg->x, msg->y, &r ) && ( r.tpr_TreeNode ) )
		{
			LONG lineheight;
			lineheight = getv( obj, MUIA_List_LineHeight );
			tn = (struct MUIS_Listtree_TreeNode *) r.tpr_TreeNode;

			if( tn->tn_Flags & TNF_LIST )
			{
				struct MUIS_Listtree_TreeNode *parent_node;
				parent_node = (APTR)DoMethod( obj, MUIM_Listtree_GetEntry,tn, MUIV_Listtree_GetEntry_Position_Parent );
		//		PDB(("%x %x\n",tn,parent_node));
				data->droppos = DROP_VOID;
				if(parent_node == 0) 
				{
					data->droppos = DROP_ONTO;
					DoMethod( obj, MUIM_Listtree_SetDropMark, r.tpr_ListEntry, MUIV_Listtree_SetDropMark_Values_Onto );
				} else {
					if(msg->y > lineheight/3)
					{
						DoMethod( obj, MUIM_Listtree_TestPos, msg->x, msg->y - lineheight/4, &r2 );
						if( r.tpr_TreeNode != r2.tpr_TreeNode )
						{
							DoMethod( obj, MUIM_Listtree_SetDropMark, r.tpr_ListEntry, MUIV_Listtree_SetDropMark_Values_Above );
							data->droppos = DROP_ABOVE;
						}
					}
					DoMethod( obj, MUIM_Listtree_TestPos, msg->x, msg->y + lineheight/4, &r2 );
					if( r.tpr_TreeNode != r2.tpr_TreeNode )
					{
						DoMethod( obj, MUIM_Listtree_SetDropMark, r.tpr_ListEntry, MUIV_Listtree_SetDropMark_Values_Below );
						data->droppos = DROP_BELOW;
					}
					if(data->droppos == DROP_VOID)
					{
						DoMethod( obj, MUIM_Listtree_SetDropMark, r.tpr_ListEntry, MUIV_Listtree_SetDropMark_Values_Onto );
						data->droppos = DROP_ONTO;
					}
				}
				
			} else {
				
			//	  PDB(("lien %d\n",lineheight));
				DoMethod( obj, MUIM_Listtree_TestPos, msg->x, msg->y + lineheight/2, &r2 );
			//	  PDB((" %x %x\n", r.tpr_TreeNode,r2.tpr_TreeNode));
				if( r.tpr_TreeNode == r2.tpr_TreeNode )
				{
					DoMethod( obj, MUIM_Listtree_SetDropMark, r.tpr_ListEntry, MUIV_Listtree_SetDropMark_Values_Above );
					data->droppos = DROP_ABOVE;
				} else {
					DoMethod( obj, MUIM_Listtree_SetDropMark, r.tpr_ListEntry, MUIV_Listtree_SetDropMark_Values_Below );
					data->droppos = DROP_BELOW;
				}

			}
#if 0
			if ( ( r.tpr_TreeNode != data->drop_node ) || ( r.tpr_Flags != data->drop_flags ) )
			{
				// Entry under mouse change need refresh
				if( !msg->update ) {
					return( MUIV_DragReport_Refresh );
				}

				data->drop_node = (struct MUIS_Listtree_TreeNode *) r.tpr_TreeNode;
				data->drop_flags = r.tpr_Flags;

				tn = (struct MUIS_Listtree_TreeNode *) r.tpr_TreeNode;
				if( tn )
				{
				//	  td = (struct treedata *) tn->tn_User;
					if( td )
					{
						if( ( data->drop_flags == MUIV_Listtree_TestPos_Result_Flags_Onto ) && !( data->drop_node->tn_Flags & TNF_LIST ) )
						{
							// Don't allow
							DoMethod( obj, MUIM_Listtree_SetDropMark, r.tpr_ListEntry, MUIV_Listtree_SetDropMark_Values_None );
							data->drop = NULL;
							//PDB(("###### Lock\n" ));
							return( MUIV_DragReport_Lock );
						} else {
							//PDB(("###### Draw\n"));
						//	  data->drop = tn;
							DoMethod( obj, MUIM_Listtree_SetDropMark, r.tpr_ListEntry, r.tpr_Flags );
						}
					}
					else
					{//PDB(( "###### DragReport: no td\n" ) );
					}
				}
				else
				{//PDB(( "###### DragReport: not en entry\n" ) );
				}
			}
#endif
		} else {
			//PDB(("###### DragReport: not en entry\n" ) );
			//put under
		}
	}
	return( MUIV_DragReport_Continue );
}

static ULONG mDragDrop(struct IClass *cl,Object *obj,struct MUIP_DragDrop *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	ULONG rx, ry, x, y, dummy;
	APTR ins_object = NULL;
	struct MUIS_Listtree_TestPos_Result pos = { NULL, 0, 0, 0 };
	struct MUIS_Listtree_TreeNode *target_node, *parent_node, *tn2,*root_node;
	LONG target_id = 0;
	LONG target_pos = 0;
	GetAttr( MUIA_Window_LeftEdge, _win( obj ), &x );
	GetAttr( MUIA_Window_TopEdge , _win( obj ), &y );
	rx = msg->x - x;
	ry = msg->y - y;

	if( ( ( DoMethod( obj, MUIM_Listtree_TestPos, rx, ry, &pos ) ) && ( pos.tpr_TreeNode ) ) )
	{
		target_node = pos.tpr_TreeNode;
		parent_node = (APTR)DoMethod( obj, MUIM_Listtree_GetEntry, target_node, MUIV_Listtree_GetEntry_Position_Parent );
		if(!parent_node) 
		{
			target_pos = MV_Application_InsertObject_PosRootHead;
			root_node = target_node;
			target_node = 0;
			target_id = -1;
		}
		else
		{
			tn2 = target_node;
			do
			{
				root_node = tn2;
				tn2 = (struct MUIS_Listtree_TreeNode *) DoMethod(obj,MUIM_Listtree_GetEntry,tn2, MUIV_Listtree_GetEntry_Position_Parent,0);
			}while(tn2);
			if( ( ( (struct MUIS_Listtree_TreeNode *) target_node )->tn_Flags & TNF_LIST ) && ( data->droppos == DROP_ONTO ) )
			{
				parent_node = target_node;
				target_pos = MV_Application_InsertObject_PosHead;
			//	kprintf("drop in closed\n");
			}
			if( data->droppos == DROP_BELOW )
			{
				APTR next_node;
				if((target_node->tn_Flags & TNF_LIST)&&(target_node->tn_Flags & TNF_OPEN))
				{
					target_pos = MV_Application_InsertObject_PosHead;
					parent_node = target_node;
				//	kprintf("drop in\n");
				}
				else
				{
					next_node = (APTR) DoMethod( obj, MUIM_Listtree_GetEntry, target_node, MUIV_Listtree_GetEntry_Position_Next, 0 );
					//	kprintf("drop below\n");
					if( next_node )
					{
					//	target_node = next_node;
					} 
					else 
					{
						target_pos = MV_Application_InsertObject_PosTail;
					}
				}
			}
			else if( data->droppos == DROP_ABOVE )
			{
				APTR prev_node;
				//	kprintf("drop above\n");
				prev_node = (APTR) DoMethod( obj, MUIM_Listtree_GetEntry, target_node, MUIV_Listtree_GetEntry_Position_Previous, 0 );
				if( !prev_node ) target_pos = MUIV_Listtree_Insert_PrevNode_Head;
			}
			if( msg->obj == obj ) /* user moves existing object around */
			{
				struct MUIS_Listtree_TreeNode *src_node, *parent_src_node,*root_src_node;
				APTR panel_win,panel_src_win;
				src_node = (APTR)DoMethod( obj, MUIM_Listtree_GetEntry,	 MUIV_Listtree_GetEntry_ListNode_Active ,   MUIV_Listtree_GetEntry_Position_Active,0);
				if(!src_node) return 0;
				parent_src_node = (APTR)DoMethod( obj, MUIM_Listtree_GetEntry, src_node, MUIV_Listtree_GetEntry_Position_Parent);
				if(!parent_src_node) return 0;
				tn2 = parent_src_node;
				do
				{
					root_src_node = tn2;
					tn2 = (struct MUIS_Listtree_TreeNode *)DoMethod(obj,MUIM_Listtree_GetEntry,tn2, MUIV_Listtree_GetEntry_Position_Parent,0);
				}while(tn2);
			#if 0
				if((panel_win = LockPanel(root_node->tn_Name)))
				{
					if(target_node){
						target_id = (ULONG)target_node->tn_User;
					}
					if(root_node == root_src_node)
					{
						MovePanelObject(panel_win,(ULONG)src_node->tn_User,target_id,target_pos);
					}
					/*else if((panel_src_lock = LockPanel(root_src_node->tn_Name)))
					{
						MovePanelObject(panel_lock,target_id,target_pos,panel_src_lock,(ULONG)src_node->tn_User);
						UnLockPanel(panel_src_lock);	
					}*/
					UnLockPanel(panel_win);	
				}
				#endif
				return 0;
			}
		}
	//	kprintf("xx %x %x\n",msg->obj,MA_Panelitem_list_IsList);
		if( GetAttr( MA_Panelitem_list_IsList, msg->obj,  &dummy ) )    /* user tries to add a new object */
		{
			struct PanelItem *panelitem;
			DoMethod( msg->obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &panelitem );
		//	kprintf("add new item %x %s %d\n",panelitem, panelitem->pi_DisplayName,panelitem->pi_Type);
		//	kprintf("%x %x\n",data->droppos,DROP_BELOW); 
			if( panelitem )
			{
				APTR ppool;
				if((ppool = (APTR)LockPPool(root_node->tn_Name,PPOOL_TYPE_PANEL)))
				{
					ULONG ins_flags = 0;
					APTR pl,pi;
					ULONG size;
					APTR d;
					ULONG s_ID = 0;
					ULONG *sub_ID = &s_ID;
					#warning double var
					ULONG target_ID = 0;
				//	kprintf("%x %x %x\n",root_node->tn_Name,parent_node->tn_Name,target_node->tn_Name);
				//	kprintf("%x %x %x\n",root_node,parent_node,target_node);
					if(target_node)
					{ 
						target_ID = (ULONG)target_node->tn_User;
						if( data->droppos == DROP_BELOW )
						{
							target_ID++;
						}
					}
					if((parent_node)&&(root_node != parent_node))
					{
						ULONG parent_ID = parent_node->tn_User;
						if((pl = (APTR)GetPPoolItem(ppool,0,  DSI_LISTPOOL_PANEL ,NULL,&size)))
						{
							if((pi = (APTR)GetPPoolItem(ppool, pl, parent_ID | DSF_LISTPOOL , &d, &size)))
							{
								GetPPoolItem(ppool, pi, DSI_LISTPOOL_PANEL_SUBPANEL_ROOT , (APTR) &sub_ID, NULL );
						//		kprintf("sub_id %d\n",*sub_ID);
							}
						}
					}
				
					//	kprintf("drop below %x %d %d %d\n",target_node,*sub_ID,target_ID,target_pos);
						if(*sub_ID)
						{
							if(*sub_ID == target_ID) target_ID++;
							if(target_pos == MV_Application_InsertObject_PosHead) target_ID = *sub_ID + 1;
						}
					
						PanelItem_Insert(ppool,target_ID,panelitem,*sub_ID);
					
					UnLockPPool(ppool);
					DoMethod(obj,MM_PanelListtree_Rebuild,root_node->tn_Name);
				}
#if 0			
				if( pi->pi_Type == PT_EXTERNAL )
				{
					APTR sobj;
					if( ( sobj = NewObject( getpanelexternalsupportclass(), NULL, TAG_DONE ) ) )
					{
						if( !( ins_object = MUI_NewObject( pi->pi_ClassName, MA_Panelextern_SupportObject, sobj, TAG_DONE ) ) )
						{
							DisposeObject( sobj );
						}
					}
				} else
#endif				
				{
					
				#if 0
					if(PanelBase)
					{
						APTR panel_lock;
						if((ins_object = (Object*)CreatePanelItem(pi->pi_Type, pi->pi_ClassName)))
						{
						//	kprintf("ins_object %x\n",ins_object);
							if((panel_lock = LockPanel(root_node->tn_Name)))
							{
								if(target_node)
								{
									target_id = (ULONG)target_node->tn_User;
								}
								InsertPanelObject(panel_lock,ins_object,target_pos);
								UnLockPanel(panel_lock);
							}
						}							
					}
					#endif
				}
			}
		}
	}
	return 0;
}

DISPATCHER(PanelListtreeClass)
{
   	switch (msg->MethodID)
	{
		case OM_NEW               				: return(mNew    					(cl,obj,(struct opSet *)msg));
		case OM_DISPOSE           				: return(mDispose					(cl,obj,(Msg)msg));
		case OM_SET                 			: return(mSet           			(cl,obj,(struct opSet *)msg));
		case OM_GET			    				: return(mGet						(cl,obj,(struct opGet *)msg));
		case MM_PanelListtree_AddPanel			: return(mAddPanel					(cl,obj,(struct MP_PanelListtree_AddPanel *)msg));
		case MM_PanelListtree_Rebuild			: return(mRebuild  					(cl,obj,(struct MP_PanelListtree_Rebuild *)msg));
	//	case MM_PanelListTree_Renumber			: return(mRenumber 					(cl,obj,(struct MP_PanelListTree_Renumber *)msg));
		case MUIM_DragQuery						: return(mDragQuery					(cl,obj,(struct MUIP_DragQuery *)msg));
		case MUIM_DragReport					: return(mDragReport				(cl,obj,(struct MUIP_DragReport *)msg));
		case MUIM_DragDrop						: return(mDragDrop					(cl,obj,(struct MUIP_DragDrop*)msg));
	}
  return(DoSuperMethodA(cl,obj,msg));
 }
DISPATCHER_END


struct MUI_CustomClass *PanelListtree_Class(void)
{
	return MUI_CreateCustomClass(NULL,MUIC_Listtree,NULL,sizeof(struct Data),DISPATCHER_REF(PanelListtreeClass));
}