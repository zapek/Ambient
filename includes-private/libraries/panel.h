#ifndef PANEL_PANEL_H
#define PANEL_PANEL_H

#define PPOOL_TYPE_MAIN		1
#define PPOOL_TYPE_PANEL	2



#define PanelHasChanged_Insert	1
#define PanelHasChanged_Remove	2 
#define	PanelHasChanged_Move	3 
#define	PanelHasChanged_Rename	4 
#define PanelPrefsClose			1000
#define PANEL_NAME_SIZE		256

struct PanelMessage {
	char pm_Name[PANEL_NAME_SIZE+1];
	ULONG pm_Mode;
	ULONG pm_Data1,pm_Data2;
};
 
enum {
	PanelObject_Position = TAG_USER + 1, 
	PanelObject_ScaleMode,
	PanelObject_ScaleMin,
	PanelObject_ScaleMax,
	PanelObject_RelSize,		/* percentage of object's size */
	PanelObject_DrawMode,
	PanelObject_Status,
	PanelObject_App,
	PanelObject_CallbackObject,
	PanelObject_CallbackMode,	
	PanelObject_MsgPort,
}; 

enum {
	PanelObjectType = TAG_USER + 100,
	PanelObjectURI,
	PanelObjectID,
	PanelObjectPanel,
	PanelObjectStartObject,
	PanelObjectWindow,
	PanelObjectWindowName,
	PanelObjectTarget,
};

#define Target_Object		0
#define Target_Window		1
#define Target_Group		2
#define Target_SubWindow	3
#define Target_SubGroup		4


#define PanelObject_DrawMode_None		0
#define PanelObject_DrawMode_Replace	1
#define PanelObject_DrawMode_Over		2


#define PanelObjectType_Command			1 /*delete*/



#define PanelObject_ScaleMode_Pixel		1
#define PanelObject_ScaleMode_Fit		2
#define PanelObject_ScaleMode_Aspect	3


#define PanelObject_Position_Top		0x0000001
#define PanelObject_Position_Y_Mid		0x0000002
#define PanelObject_Position_Bottom		0x0000004
#define PanelObject_Position_Left		0x0000008
#define PanelObject_Position_X_Mid		0x0000010
#define PanelObject_Position_Right		0x0000020

#define PanelObject_Position_Mid		PanelObject_Position_X_Mid | PanelObject_Position_Y_Mid

#define PanelObject_Object		1
#define PanelObject_Group		2
#define PanelObject_Window		3

#define PanelObject_CallbackMode_None		0
#define PanelObject_CallbackMode_MUIApp		1
#define PanelObject_CallbackMode_MUIObj		2
#define PanelObject_CallbackMode_MsgPort	3

#define PANEL_MODE_NORMAL		0x00000000
#define PANEL_MODE_PREFS		0x00000001

#endif /* PANEL_H */
