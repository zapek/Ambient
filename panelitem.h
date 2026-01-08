#ifndef AMBIENT_PANELITEM_H
#define AMBIENT_PANELITEM_H

/*
 * $Id: panelitem.h,v 1.15 2019/04/14 13:17:03 kronos Exp $
 */

#include <graphics/gfx.h>
#include <intuition/intuition.h>
#include <libraries/mui.h>

/************************************************************************/

extern TEXT PANEL_DISKPATHSYS[];
extern TEXT PANEL_DISKPATHMOSSYS[];

//#define PANEL_DISKPATHSYS &PANEL_DISKPATHMOSSYS[3]
//#define PANEL_DISKPATHSYS &PANEL_DISKPATHSYS[3]

/************************************************************************/

struct PanelItem {
	ULONG          pi_Type;            /* panel type */
	STRPTR         pi_DisplayName;
	STRPTR         pi_ClassName;
	STRPTR         pi_Author;
	STRPTR         pi_Description;
	ULONG          pi_Version;
	ULONG          pi_Revision;
	APTR           pi_Image;
	struct BitMap *pi_BitMap;
	struct IClass *pi_Class;
	Object        *pi_Object;
};

#define PANELIMAGE_WIDTH  26
#define PANELIMAGE_HEIGHT 20

/*
 * Paneltypes
 */

enum {
	PT_BUTTON = 0,     /* must be first */
	PT_SPACER,
	PT_SEPARATOR,
	PT_VIEWWATCHER,
	PT_BOOKMARKS,
	PT_SUBPANEL,
	PT_DIRPANEL,
	PT_END,        /* end mark! */
	PT_EXTERNAL,   /* always make it last one! */
};

/************************************************************************/

struct PanelItem *PanelItem_Create( ULONG type, STRPTR panelname );
void              PanelItem_Delete( struct PanelItem *ie );
LONG              PanelItem_NameToType( CONST_STRPTR name );

/************************************************************************/

#endif /* AMBIENT_PANELITEM_H */
