/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2010 Ambient Open Source Team
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
 * $Id: menus.c,v 1.28 2023/01/18 02:53:09 jacadcaps Exp $
 */

#include "ambient.h"

/* public */
#include <libraries/gadtools.h>

/* private */
#include "menus.h"
#include "ambient_cat.h"
#include "mui_func.h"
#include "prefs.h"
#include "command.h"
#include "action.h"
#include "actiondispatcherclass.h"
#include "mimetype.h"
#include "dragdrop.h"


/*
 * Shortcuts:
 *
 * ABCDEFGHIJKLMNOPQRSTUVWXYS?.,
 * xxx x   x xx xxxxxx x     xx   (old or wb shortcuts?)
 * x   x  xx    x  xx        x    (current)
 *
 */

#define MENU(x) (STRPTR)MSG_MENU_##x
#define MENUID(x) (APTR)MENU_##x

#define TICK (CHECKIT | MENUTOGGLE)

#define DEBUGMENU(x)   {NM_ITEM, (STRPTR)#x, 0, CHECKIT | MENUTOGGLE, 0, (APTR)(MENU_DEBUG_BASE + DB_##x)}
#define SEPARATOR      {NM_ITEM, NM_BARLABEL, 0, 0, 0, 0},
#define MENUEND        {NM_END, NULL, 0, 0, 0, 0}


//APTR menu;
struct NewMenu newmenus[] = {
/* Workbench */
{NM_TITLE, "Ambient",                  0,                    0, 0, 0},
 {NM_ITEM, MENU(WB_DEVICELIST),        0,                    0, 0, MENUID(WB_DEVICELIST)},
 SEPARATOR
 {NM_ITEM, MENU(WB_EXECUTE),           "E",                  0, 0, MENUID(WB_EXECUTE)},
 {NM_ITEM, MENU(WB_NEWSHELL),          "N",                  0, 0, MENUID(WB_NEWSHELL)},
// NM_ITEM, MENU(WB_LASTMESSAGE),        0,                    0, 0, MENUID(WB_LASTMESSAGE),
 SEPARATOR
 {NM_ITEM, MENU(WB_ABOUT),             "?",                  0, 0, MENUID(WB_ABOUT)},
 {NM_ITEM, MENU(WB_ABOUTMOS),          0,                    0, 0, MENUID(WB_ABOUTMOS)},
 SEPARATOR
 {NM_ITEM, MENU(WB_QUIT),              "Q",                  0, 0, MENUID(WB_QUIT)},

/* Edit */
{NM_TITLE, MENU(EDIT),                 0,                    0, 0, MENUID(EDIT)},
 {NM_ITEM, MENU(EDIT_CUT),             "X",                  0, 0, MENUID(EDIT_CUT)},
 {NM_ITEM, MENU(EDIT_COPY),            "C",                  0, 0, MENUID(EDIT_COPY)},
 {NM_ITEM, MENU(EDIT_PASTE),           "V",                  0, 0, MENUID(EDIT_PASTE)},
// {NM_ITEM, MENU(EDIT_PASTEINTO),       0,                    0, 0, MENUID(EDIT_PASTEINTO)},
 SEPARATOR
 {NM_ITEM, MENU(EDIT_SELECTALL),       "A",                  0, 0, MENUID(EDIT_SELECTALL)},
 {NM_ITEM, MENU(EDIT_INVERT),          0,                    0, 0, MENUID(EDIT_INVERT)},

/* View */
{NM_TITLE, MENU(VIEW),                 0,                    0, 0, 0},
 {NM_ITEM, MENU(VIEW_NEWDRAWER),       0,                    0, 0, MENUID(VIEW_NEWDRAWER)},
 {NM_ITEM, MENU(VIEW_NETWORKSCONNECT), 0,                    0, 0, MENUID(VIEW_NETWORKSCONNECT)},
 #if 0
 // this mambo jambo doesnt work
 SEPARATOR
 {NM_ITEM, MENU(VIEW_LIST),            "L",                  TICK, 0, MENUID(VIEW_LIST)},
 #endif
 SEPARATOR
 {NM_ITEM, MENU(VIEW_SORT),            0,                    0, 0, MENUID(VIEW_SORT)},
  {NM_SUB, MENU(VIEW_SORTBYNAME),      ".",                  TICK, 0, MENUID(VIEW_SORT_BYNAME)},
  {NM_SUB, MENU(VIEW_SORTBYTYPE),      0,                    TICK, 0, MENUID(VIEW_SORT_BYTYPE)},
  {NM_SUB, MENU(VIEW_SORTBYSIZE),      0,                    TICK, 0, MENUID(VIEW_SORT_BYSIZE)},
  {NM_SUB, MENU(VIEW_SORTBYDATE),      0,                    TICK, 0, MENUID(VIEW_SORT_BYDATE)},

/* Icons */
{NM_TITLE, MENU(ICONS),                0,                    0, 0, 0},
 {NM_ITEM, MENU(ICONS_INFORMATION),    "I",                  0, 0, MENUID(ICONS_INFORMATION)},
 SEPARATOR
 {NM_ITEM, MENU(ICONS_PUTAWAY),        0,                    0, 0, MENUID(ICONS_PUTAWAY)},
 {NM_ITEM, MENU(ICONS_EJECT),          "J",                  0, 0, MENUID(ICONS_EJECT)},
 SEPARATOR
 {NM_ITEM, MENU(ICONS_RENAME),         "R",                  0, 0, MENUID(ICONS_RENAME)},
 {NM_ITEM, MENU(ICONS_DELETE),         "Del",                0, 0, MENUID(ICONS_DELETE)},
 SEPARATOR
 {NM_ITEM, MENU(ICONS_TRASH),          "T",                  0, 0, MENUID(ICONS_TRASH)},
 {NM_ITEM, MENU(ICONS_RESTORE),        "Y",                  0, 0, MENUID(ICONS_RESTORE)}, 
 {NM_ITEM, MENU(ICONS_EMPTY),          0,                    0, 0, MENUID(ICONS_EMPTY)}, 
 SEPARATOR
 {NM_ITEM, MENU(ICONS_FORMAT),         0,                    0, 0, MENUID(ICONS_FORMAT)},

/* Utilities */
{NM_TITLE, MENU(UTILITIES),            0,                    0, 0, 0},
 {NM_ITEM, MENU(UTILITIES_EXCHANGE),   "H",                  0, 0, MENUID(UTILITIES_EXCHANGE)},
// {NM_ITEM, MENU(UTILITIES_SOUND),      0,                    0, 0, MENUID(UTILITIES_SOUND)},
 {NM_ITEM, MENU(UTILITIES_SYSTEMINFO), 0,                    0, 0, MENUID(UTILITIES_SYSTEMINFO)},
#if !USE_LEGACY
 {NM_ITEM, MENU(UTILITIES_SYSTEMLOG),  0,                    0, 0, MENUID(UTILITIES_SYSTEMLOG)},
#endif
 SEPARATOR
 {NM_ITEM, MENU(UTILITIES_FORMAT),     0,                    0, 0, MENUID(UTILITIES_FORMAT)},
 {NM_ITEM, MENU(UTILITIES_FIND),       "F",                  0, 0, MENUID(UTILITIES_FIND)},

/* Settings */
{NM_TITLE, MENU(SETTINGS),             0,                    0, 0, 0},
 {NM_ITEM, MENU(SETTINGS_DESKTOP),     0,                    0, 0, MENUID(SETTINGS_DESKTOP)},
 {NM_ITEM, MENU(SETTINGS_MUI),         0,                    0, 0, MENUID(SETTINGS_MUI)},
 SEPARATOR
 {NM_ITEM, MENU(SETTINGS_SYSTEM),      0,                    0, 0, MENUID(SETTINGS_SYSTEM)},
 {NM_ITEM, MENU(SETTINGS_MUI_GLOBAL),  0,                    0, 0, MENUID(SETTINGS_MUI_GLOBAL)},
// {NM_ITEM, NM_BARLABEL,                0,                    0, 0, 0},
// {NM_ITEM, MENU(SETTINGS_SAVE),        0,                    0, 0, MENUID(SETTINGS_SAVE)},

/* Tools */
//NM_TITLE, MENU(TOOLS),                 0,                    0, 0, 0,
// NM_ITEM, MENU(TOOLS_RESETWB),         0,                    0, 0, 0,

/* Debug Actions */
#ifdef DEBUG
{NM_TITLE, MENU(DEBUGACTION),          0,                    0, 0, 0},
 #if USE_MEMTRACK_MEMLIST
 {NM_ITEM, MENU(DEBUGACTION_MEMCHECK), 0, 0, 0, MENUID(DEBUGACTION_MEMCHECK)},
 {NM_ITEM, MENU(DEBUGACTION_MEMSTATS), 0, 0, 0, MENUID(DEBUGACTION_MEMSTATS)},
 {NM_ITEM, MENU(DEBUGACTION_MEMSTATS_ALL), 0, 0, 0, MENUID(DEBUGACTION_MEMSTATS_ALL)},
 #endif
 #if USE_MEMTRACK_RECORD
 {NM_ITEM, MENU(DEBUGACTION_STARTMEMRECORD), 0, 0, 0, MENUID(DEBUGACTION_STARTMEMRECORD)},
 {NM_ITEM, MENU(DEBUGACTION_STOPMEMRECORD), 0, 0, 0, MENUID(DEBUGACTION_STOPMEMRECORD)},
 #endif
#endif

/* Debug, new entries at the end */
#ifdef DEBUG
{NM_TITLE, MENU(DEBUG),                0,                    0, 0, 0},
 DEBUGMENU(ICONIO),
 DEBUGMENU(PROC),
 DEBUGMENU(DUMPIMAGE),
 DEBUGMENU(WBSTART),
 DEBUGMENU(WBSTARTUP),
 DEBUGMENU(REXX),
 DEBUGMENU(LAYOUT),
 DEBUGMENU(DEVICEIO),
 DEBUGMENU(DRAGDROP),
 DEBUGMENU(INIT),
 DEBUGMENU(ARGS),
 DEBUGMENU(CLASS),
 DEBUGMENU(LIB),
 DEBUGMENU(DOSLISTCACHE),
 DEBUGMENU(PATH),
 DEBUGMENU(COPY),
 DEBUGMENU(DEFICON),
 DEBUGMENU(DEFICONPOOL),
 DEBUGMENU(EXDIR),
 DEBUGMENU(DOMETHOD),
 DEBUGMENU(RECURSE),
 DEBUGMENU(SCANDIR),
 DEBUGMENU(PREFSIO),
 DEBUGMENU(LABELSPLIT),
 DEBUGMENU(SHORTCUT),
 DEBUGMENU(MIMEURI),
 DEBUGMENU(RDARGS),
 DEBUGMENU(RECOG),
 DEBUGMENU(PREFSPOOL),
 DEBUGMENU(SNDDRV),
 DEBUGMENU(SOUND),
 DEBUGMENU(NOTIFY),
 DEBUGMENU(MIMEACTIONED),
 DEBUGMENU(MIMETYPE),
 DEBUGMENU(ADVANCEDPREFS),
 DEBUGMENU(ACTIONDISP),
 DEBUGMENU(APPICON),
 DEBUGMENU(CREATEICON),
#endif

MENUEND };

/* User menus */
struct menu_user_node {
	struct MinNode n;
	STRPTR id;
	STRPTR parentid;
	STRPTR label;
	STRPTR shortcut;
	STRPTR type;
	STRPTR command;
	ULONG  commandtype;
};

static struct MinList menu_user_entries;

void menus_user_init(void);
void menus_user_cleanup(void);
void menus_user_remove_from_view(APTR obj);
void menus_user_refresh(void);

/* */

ULONG menus_init(void)
{
	struct NewMenu *nmp = newmenus;

	nmp++;
	while (nmp->nm_Type)
	{
		if (nmp->nm_Label != NM_BARLABEL)
		{
			#ifdef DEBUG
			if ((ULONG)nmp->nm_UserData < MENU_DEBUG_BASE)
			#endif
				nmp->nm_Label = (STRPTR)GSI((ULONG)nmp->nm_Label);
		}
		nmp++;
	}

	menus_user_init();

	return (TRUE);
}


void menus_cleanup(void)
{
	menus_user_cleanup();
}


void menus_execute(APTR app, APTR viewobj, LONG menunum)
{
	switch (menunum)
	{
		/*
		 * Ambient
		 */

		/* Devicelist */
		case MENU_WB_DEVICELIST:
			DoMethod(app, MM_Application_OpenDevicesWindow, 0);
			break;

		/* Execute command... */
		case MENU_WB_EXECUTE:
			DoMethod(app, MM_Application_Open_ExecuteWindow);
			break;

		/* Newshell */
		case MENU_WB_NEWSHELL:
			DoMethod(app, MM_Application_NewShell);
			break;

		/* About */
		case MENU_WB_ABOUT:
			DoMethod(app, MM_Application_Open_AboutWindow);
			break;

		/* About MorphOS */
		case MENU_WB_ABOUTMOS:
			DoMethod(app, MM_Application_Open_AboutMorphOSWindow, TRUE);
			break;

		/* Quit */
		case MENU_WB_QUIT:
			DoMethod(app, MUIM_Application_PushMethod, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
			break;

		/*
		 * Edit & View
		 *
		 * Not handled here
		 */

		/*
		 * Utilities
		 */

		/* Exchange */
		case MENU_UTILITIES_EXCHANGE:
			DoMethod(app, MM_Application_Open_CxWindow);
			break;

		/* Sound */
//		case MENU_UTILITIES_SOUND:
//			DoMethod(app, MM_Application_DoRexx, TRUE, NULL, "Sound Opencontrol", NULL, NULL, 0, NULL);
//			break;

		/* SystemInfo */
		case MENU_UTILITIES_SYSTEMINFO:
			DoMethod(app, MM_Application_Open_SystemInfoWindow);
			break;

		#if !USE_LEGACY
		/* SystemInfo */
		case MENU_UTILITIES_SYSTEMLOG:
			DoMethod(app, MM_Application_Open_SystemLog);
			break;
		#endif

		/* Format disk.. */
		case MENU_UTILITIES_FORMAT:
			DoMethod(app, MM_Application_DoRexx, TRUE, NULL, "Format", NULL, NULL, 0, NULL);
			break;

		/* Find */
		case MENU_UTILITIES_FIND:
			DoMethod(app, MM_Application_DoRexx, TRUE, NULL, "Find [VOLUMES]", NULL, NULL, 0, NULL);
			break;

		/*
		 * Settings
		 */

		/* Desktop... */
		case MENU_SETTINGS_DESKTOP:
			DoMethod(app, MM_Application_DoRexx, TRUE, NULL, "Settings", NULL, NULL, 0, NULL);
			break;

		/* System... */
		case MENU_SETTINGS_SYSTEM:
			DoMethod(app, MM_Application_DoRexx, TRUE, NULL, "Settings PAGE=SYSTEM", NULL, NULL, 0, NULL);
			break;

		/* MUI... */
		case MENU_SETTINGS_MUI:
			DoMethod(app, MUIM_Application_OpenConfigWindow, 0, NULL); /* XXX: make it hide all the screen selections everywhere */
			break;

		/* MUI (Global)... */
		case MENU_SETTINGS_MUI_GLOBAL:
			DoMethod(app, MM_Application_DoRexx, TRUE, NULL, "Run MOSSYS:Prefs/MUI", NULL, NULL, 0, NULL);
			break;

		#if 0
		/* Save settings */
		case MENU_SETTINGS_SAVE:
			DoMethod(app, MM_Application_SavePrefs, FALSE);
			break;
		#endif

		#ifdef DEBUG
		/*
		 * Debug Action
		 */
		#if USE_MEMTRACK_MEMLIST
		case MENU_DEBUGACTION_MEMCHECK:
			DoMethod(app, MM_Application_DoDebug, DBA_MEMCHECK);
			break;

		case MENU_DEBUGACTION_MEMSTATS:
			DoMethod(app, MM_Application_DoDebug, DBA_MEMSTATS);
			break;

		case MENU_DEBUGACTION_MEMSTATS_ALL:
			DoMethod(app, MM_Application_DoDebug, DBA_MEMSTATS_ALL);
			break;
		#endif
		#if USE_MEMTRACK_RECORD
		case MENU_DEBUGACTION_STARTMEMRECORD:
			DoMethod(app, MM_Application_DoDebug, DBA_STARTMEMRECORD);
			break;

		case MENU_DEBUGACTION_STOPMEMRECORD:
			DoMethod(app, MM_Application_DoDebug, DBA_STOPMEMRECORD);
			break;
		#endif
		#endif

		default:
		{
			struct menu_user_node * n = (struct menu_user_node *) menunum;
			APTR action;
			APTR wo = _win( viewobj );

			if(menunum < MENU_USER_BASE)
			{
				break;
			}

			if(n->command == NULL) /* Means it's a menu node, nothing to do */
			{
				break;
			}

			action = actionnode_create();

			if ( action )
			{
				/* create action */

				actionnode_addcommand( action, n->commandtype, n->command );

				actionnode_setup( action );
/*
				actionnode_setattrs(action,
									ACTIONNODETAG_FLAGS, ((ULONG) actionnode_getattr(action, ACTIONNODETAG_FLAGS )) |  n->flags,
									TAG_DONE);
*/
				if ( getv( wo, MA_Window_Type ) == MV_Window_Type_View || getv( wo, MA_Window_Type ) == MV_Window_Type_Rootview )
				{
					/* get view object */
					APTR vo = _view( viewobj );
					APTR action_copy;

					action_copy = actionnode_duplicate( action );

					if ( vo && action_copy)
					{
						APTR dispatcher = (APTR)DoMethod( app, MM_Application_CreateActionDispatcher );

						if ( dispatcher )
						{
							struct MinList ml;
							ULONG needfiles = (ULONG)actionnode_getattr( action_copy, ACTIONNODETAG_FLAGS ) & ACTION_FLAG_NEED_ENTRIES;

							NEWLIST(&ml);

							if ( needfiles )
							{
								DoMethod(vo, MM_View_GetSelectionList, &ml);
							}

							SetAttrs(dispatcher,
								MA_ActionDispatcher_SrcURI, (APTR)getv(vo, MA_View_Path),
								MA_ActionDispatcher_SrcID, (APTR)getv(vo, MA_Viewgroup_ID),
								MA_ActionDispatcher_RefWin, _win(viewobj),
								MA_ActionDispatcher_Action, action_copy,
							TAG_DONE);

							if ( !ISLISTEMPTY( &ml ) )
							{
								struct dragdropnode *ddn, *nextddn;

								ITERATELISTSAFE(ddn, nextddn, &ml)
								{
									DoMethod( dispatcher, MM_ActionDispatcher_AddURI, ddn->path, TRUE );
									/* XXX: careful here, if you break out you must still free the nodes */
									free(ddn);
								}
							}
							else if ( !needfiles )
							{
								/*
								 * No files needed, but we need at least one, so give it dummy one.
								*/
								DoMethod( dispatcher, MM_ActionDispatcher_AddURI, "dummy", TRUE );
							}

							DoMethod( dispatcher, MM_ActionDispatcher_Execute );
						}
						else
						{
							actionnode_delete( action_copy );
						}
					}
				}

				actionnode_delete( action );
			}
		}
	}
}


void menus_shutdown(APTR menu)
{
	set(FINDMENU(MENU_WB_QUIT), MUIA_Menuitem_Title, GSI(MSG_MENU_SHUTDOWN));
}


#ifdef DEBUG
static void setmenu(APTR menu, ULONG menuid, int state)
{
	set(FINDMENU(menuid), MUIA_Menuitem_Checked, state);
}

void menus_bind_debug(APTR menu)
{
	ULONG i;
	APTR o;

	for (i = 0; db_a[i].prefs ; i++)
	{
		if (!db_a[i].active)
		{
			db_a[i].active = getprefslong(db_a[i].prefs);
		}
		setmenu(menu, MENU_DEBUG_BASE + i, db_a[i].active);

		if( (o = FINDMENU(MENU_DEBUG_BASE + i)) )
		{
			DoMethod(o, MUIM_Notify, MUIA_Menuitem_Checked, MUIV_EveryTime,
				MUIV_Notify_Application, 3, MM_Application_SetDebug, i, MUIV_TriggerValue
			);
		}
		else
		{
			PDB(("argh, menus debug num %ld is not set! fix!\n", i));
		}
	}
}
#endif /* DEBUG */

void menus_user_init(void)
{
	NEWLIST(&menu_user_entries);
}

void menus_user_cleanup(void)
{
	struct menu_user_node *n, *nextn;

	ITERATELISTSAFE(n, nextn, &menu_user_entries)
	{
		free(n->command);
		free(n->label);
		free(n->shortcut);
		free(n->id);
		free(n->parentid);
		free(n);
	}
	NEWLIST(&menu_user_entries);
}

static STRPTR menus_user_find_id(CONST_STRPTR label)
{
	struct menu_user_node * mnode;

	ITERATELIST(mnode, &menu_user_entries)
	{
		if(label && mnode->label && !stricmp(mnode->label, label))
		{
			return mnode->id;
		}
	}

	return NULL;
}

static APTR menus_user_find_node(CONST_STRPTR id)
{
	struct menu_user_node * mnode;

	ITERATELIST(mnode, &menu_user_entries)
	{
		if(mnode->id && !stricmp(mnode->id, id))
		{
			return (APTR) mnode;
		}
	}

	return NULL;
}

void menus_user_add(CONST_STRPTR id, CONST_STRPTR title, CONST_STRPTR shortcut, CONST_STRPTR type, CONST_STRPTR parentid, CONST_STRPTR command, ULONG commandtype)
{
	//struct NewMenu *nmp = newmenus;
	struct menu_user_node * mnode;
	//ULONG nm_Type = NM_END;

	mnode = (struct menu_user_node *) malloc( sizeof( *mnode ) );

	if( mnode )
	{
		int len;

		/* Type */
		if( !stricmp( "separator", type ) )
		{
			mnode->type = NM_BARLABEL;
		}
		else if( !stricmp( "menu", type ) || !stricmp( "item", type ) ) /* we don't really care what it is */
		{
			mnode->type = 0;
		}

		/* Id */
		len = strlen( id ) + 1;
		mnode->id = (STRPTR) malloc( len );

		if( mnode->id )
		{
			strcpy( mnode->id, id );
		}

		/* Parent Id */
		if( parentid )
		{
			len = strlen( parentid ) + 1;
			mnode->parentid = (STRPTR) malloc( len );

			if( mnode->parentid )
			{
				strcpy( mnode->parentid, parentid );
			}
		} else {
			mnode->parentid = NULL;
		}

		/* Title */
		if(title)
		{
			len = strlen(title) + 1;
			mnode->label = (STRPTR) malloc(len);

			if(mnode->label)
			{
				strcpy(mnode->label, title);
			}
		} else {
			mnode->label = NULL;
		}

		/* Shortcut */
		if( shortcut )
		{
			len = strlen( shortcut ) + 1;
			mnode->shortcut = (STRPTR) malloc( len );

			if( mnode->shortcut )
			{
				strcpy( mnode->shortcut, shortcut );
			}
		} else {
			mnode->shortcut = NULL;
		}

		/* Command */

		if( command )
		{
			len = strlen( command ) + 1;
			mnode->command = (STRPTR) malloc( len );

			if(mnode->command)
			{
				strcpy( mnode->command, command );
			}
		} else {
			mnode->command = NULL;
		}

		/* Command Type */
		mnode->commandtype = commandtype;

		ADDTAIL( &menu_user_entries, mnode );
	}

	menus_user_refresh();
}

void menus_user_remove( void )
{
	struct menu_user_node *n, *nextn;

	FORCHILD( app, MUIA_Application_WindowList )
	{
		LONG type = getv(child, MA_Window_Type);
		if( type == MV_Window_Type_Rootview || type == MV_Window_Type_View )
		{
			menus_user_remove_from_view( child );
		}
	}
	NEXTCHILD

	ITERATELISTSAFE( n, nextn, &menu_user_entries )
	{
		free( n->command );
		free( n->label   );
		free( n->shortcut);
		free( n->id      );
		free( n->parentid);
		free( n );
	}
	NEWLIST( &menu_user_entries );
}

void menus_user_add_to_view(APTR obj)
{
	struct menu_user_node * mnode;
	APTR strip = (APTR) getv(obj, MUIA_Window_Menustrip);

	ITERATELIST(mnode, &menu_user_entries)
	{
		if(mnode->parentid)
		{
			APTR menu = NULL;
			APTR node = menus_user_find_node(mnode->parentid);

			if(node)
			{
				APTR menu = (APTR) DoMethod(strip, MUIM_FindUData, node);

				if(menu)
				{
					STRPTR shortcut = mnode->shortcut && *mnode->shortcut ? mnode->shortcut : NULL;
					DoMethod(menu, MUIM_Menustrip_InitChange);
					DoMethod(menu, MUIM_Family_AddTail, MUI_MakeObject(MUIO_Menuitem, mnode->type == NM_BARLABEL ? NM_BARLABEL : mnode->label, shortcut, 0, mnode) );
					DoMethod(menu, MUIM_Menustrip_ExitChange);
				}
			}

			if(!menu)
			{
				FORCHILD(strip, MUIA_Family_List)
				{
					STRPTR id = menus_user_find_id((STRPTR) getv(child, MUIA_Menu_Title));


					if(id && !stricmp(mnode->parentid, id))
					{
						STRPTR shortcut = mnode->shortcut && *mnode->shortcut ? mnode->shortcut : NULL;
						DoMethod(child, MUIM_Menustrip_InitChange);
						DoMethod(child, MUIM_Family_AddTail, MUI_MakeObject(MUIO_Menuitem, mnode->type == NM_BARLABEL ? NM_BARLABEL : mnode->label, shortcut, 0, mnode) );
						DoMethod(child, MUIM_Menustrip_ExitChange);
					}
				}
				NEXTCHILD
			}
		}
		else
		{
			APTR menuobj = MenuObject, MUIA_Menu_Title, mnode->label, End;
			DoMethod(strip, MUIM_Family_AddTail, menuobj);
		}
	}
}

void menus_user_remove_from_view(APTR obj)
{
	struct menu_user_node * mnode;
	APTR strip = (APTR) getv(obj, MUIA_Window_Menustrip);

	ITERATELIST(mnode, &menu_user_entries)
	{
		if(!mnode->parentid)
		{
			FORCHILD(strip, MUIA_Family_List)
			{
				STRPTR label = (STRPTR) getv(child, MUIA_Menu_Title);
				if(label && !stricmp(label, mnode->label))
				{
					DoMethod(strip, MUIM_Family_Remove, child);
					break;
				}
			}
			NEXTCHILD;
		}
	}
}

void menus_user_refresh(void)
{
	FORCHILD(app, MUIA_Application_WindowList)
	{
		LONG type = getv(child, MA_Window_Type);
		if(type == MV_Window_Type_Rootview || type == MV_Window_Type_View)
		{
			menus_user_remove_from_view(child);
			menus_user_add_to_view(child);
		}
	}
	NEXTCHILD
}
