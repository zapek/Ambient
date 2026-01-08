#ifndef AMBIENT_CONTEXTMENU_H
#define AMBIENT_CONTEXTMENU_H
/*
 * $Id: contextmenu.h,v 1.14 2012/08/17 11:03:13 geit Exp $
 */

/*
 * Menu types
 */
enum {
	CM_ICON = 1,
	CM_ICONGROUP,
	CM_ICONVIEW,
	CM_PANELGROUP,
	CM_PANELGROUP_DRAG,
	CM_PANELGROUP_SUB,
	CM_PANELGROUP_SUBPANEL,
	CM_PANELGROUP_DIRPANEL,
	CM_PANELBUTTON,
	CM_PANELSPACER,
	CM_PANELSEPARATOR,
	CM_PANELPOPUP,
	CM_VIEW,
	CM_LIST,
	CM_LISTGROUP,
	CM_LISTVIEW,
	CM_APPICON,
};

struct menuitem_state
{
	ULONG checkit;
	ULONG checked;
	ULONG enabled;
	/* XXX: to be completed */
};

#define CM_BUFFERSIZE 96  /* context menu string buffer size */

enum {
	MENU_GLOBALACTION_FILE,
	MENU_GLOBALACTION_DIRECTORY,
	MENU_GLOBALACTION_DEVICE
};


APTR contextmenu_build(ULONG menumode, ULONG allow);
APTR contextmenu_build_simple(ULONG menumode, ULONG allow);

ULONG contextmenu_init(void);
void contextmenu_cleanup(void);
ULONG contextmenu_setuplabels(void);

void contextmenu_item_check(APTR pm, STRPTR name, ULONG action);
void contextmenu_delete_cm(APTR cm);
ULONG contextmenu_addmodes(APTR obj, APTR pm, ULONG index);
ULONG contextmenu_addviews(APTR obj, APTR pm);
ULONG contextmenu_add_mime( APTR cmenu, STRPTR uri, APTR mimetype );
ULONG contextmenu_add_global( APTR cmenu, STRPTR uri, ULONG type );
ULONG contextmenu_add_listview_options(APTR obj, APTR cmenu, ULONG(*callback)(APTR obj, int i, struct menuitem_state *state));
void contextmenu_execute_shortcut(APTR obj, TEXT c, ULONG mask);
void contextmenu_execute(APTR obj, APTR parent, APTR cmobj, ULONG grouped);

#endif /* AMBIENT_CONTEXTMENU_H */
