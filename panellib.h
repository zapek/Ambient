#ifndef AMBIENT_PANELLIB_H
#define AMBIENT_PANELLIB_H
/*
 * $Id: panellib.h,v 1.6 2018/12/08 14:19:09 kronos Exp $
 */
 

struct PanelMessage {
	struct Message msg;
	ULONG tag;
	ULONG data;
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
	PanelObjectPanel,
	PanelObjectPred,
	PanelObjectWin,
};

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

ULONG panellib_init(void);
void panellib_cleanup(void);

ULONG preclose_panellib(void);


#endif /* AMBIENT_PANELLIB_H */
