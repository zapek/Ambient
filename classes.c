/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2006 Ambient Open Source Team
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
 * $Id: classes.c,v 1.22 2025/12/28 14:07:54 kronos Exp $
 */

#include "ambient.h"

/* private */
#include "classes.h"
#include "errorreq.h"


struct classdesc {
	STRPTR name;
	APTR initfunc;
	APTR cleanupfunc;
	ULONG optional;
};

#define CLASSENT(s) {#s, create_##s##class, delete_##s##class, 0}

static const struct classdesc cd[] = {
	CLASSENT(app),
	CLASSENT(window),
	CLASSENT(icon),
	CLASSENT(about),
	CLASSENT(aboutmos),
	CLASSENT(infowin),
	CLASSENT(capacitytext),
	CLASSENT(addtext),
	CLASSENT(execute),
	CLASSENT(ttlistview),
	CLASSENT(smartreq),
	CLASSENT(smarttext),
//	  #if USE_DROP_EFFECT_PREFS
	CLASSENT(dropeffect),
//	  #endif
	CLASSENT(progresswin),
	CLASSENT(formatwin),
	CLASSENT(renamewin),
	CLASSENT(format),
	CLASSENT(formatlist),
	CLASSENT(menuitem),
	CLASSENT(menu),
	CLASSENT(menustrip),
	CLASSENT(makedirwin),
	CLASSENT(makelinkwin),
	CLASSENT(logo),
	CLASSENT(ttlist),
	CLASSENT(cxwin),
	CLASSENT(cxlist),
	CLASSENT(soundwin),
	CLASSENT(bgrender),
	CLASSENT(fasttitlegroup),
	CLASSENT(fasttitletext),
	CLASSENT(mimegroup),
	CLASSENT(mimelisttree),
	CLASSENT(mimeadjustwin),
	CLASSENT(mimeadjustgroup),
	CLASSENT(mimeadjustlist),
	CLASSENT(mimeactionwin),
	CLASSENT(mimeactiongroup),
	CLASSENT(infoicongroup),
	CLASSENT(graph),
	CLASSENT(sysinfowin),
	CLASSENT(navigation),
	#if USE_CRAWLER
	CLASSENT(crawl),
	#endif
	CLASSENT(editstring),
	CLASSENT(viewgroup),
	CLASSENT(viewsizegroup),
	CLASSENT(view),
	CLASSENT(gview),
	/* view subclasses MUST go there */
	CLASSENT(iconview),
	#if USE_VIEW_IMAGE
	CLASSENT(imageview),
	#endif
	#if USE_VIEW_TEXT
	CLASSENT(textview),
	#endif
	#if USE_VIEW_HEX
	CLASSENT(hexview),
	#endif
	CLASSENT(listview),
	CLASSENT(listviewlist),
	CLASSENT(boopsiview),
	/* panel classes */
#if USE_INTERNAL_PANELS    
	CLASSENT(panelbasebutton),
	CLASSENT(panelwin),
	CLASSENT(panelgroup),
	CLASSENT(paneldrag),
	CLASSENT(panelitem_list),
	CLASSENT(panellisttree),           	
	CLASSENT(panelclasslist),
	CLASSENT(panelslidersize),
	CLASSENT(panelspacer),
	CLASSENT(panelseparator),
	CLASSENT(panelviewwatcher),
	CLASSENT(panelbookmarks),
	CLASSENT(panelcommandbutton),
	CLASSENT(panelsubpanelbutton),
	CLASSENT(paneldirpanelbutton),
	CLASSENT(panelexternalsupport),
	CLASSENT(panelsubwin),
#endif
	/* prefs classes */
	CLASSENT(prefswin_list),
	CLASSENT(prefswin_main),
	CLASSENT(prefswin_background),
	CLASSENT(prefswin_icondisplay),
	CLASSENT(prefswin_miscellaneous),
	CLASSENT(prefswin_bookmarks),
	CLASSENT(prefswin_clilaunch),
	#if USE_DROP_EFFECT_PREFS
	CLASSENT(prefswin_dragdrop),
	#endif
	CLASSENT(prefswin_lister),
#if USE_INTERNAL_PANELS    
	CLASSENT(prefswin_panel),
	CLASSENT(panelsliderspeed),
#endif
	CLASSENT(prefswin_mime),
	CLASSENT(prefswin_window),
	CLASSENT(prefswin_keyboard),
	CLASSENT(prefswin_advanced),

	CLASSENT(advancedprefsgroup),
	CLASSENT(advancedprefslist),

	/* toolbar classes */
	CLASSENT(toolbutton),
	CLASSENT(toolbutton_action),
	CLASSENT(toolbutton_spacer),
	CLASSENT(toolbutton_history),
	CLASSENT(toolbutton_viewswitch),
	CLASSENT(toolbutton_bookmarks),
	CLASSENT(toolbar),
	CLASSENT(toolbargroup),
	CLASSENT(clickpath),
	CLASSENT(clickpathbutton),

	/* statusbar class */
	CLASSENT(statusbar),

	/* virtgroup with fixed height */
	CLASSENT(virtgroup),

	/* searchstring */
	CLASSENT(searchbar),
	CLASSENT(searchstring),

	/* destination selector */
	CLASSENT(destselectwin),

	/* action dispatcher */
	CLASSENT(actiondispatcher),

	/* hidden devices list */
	CLASSENT(listviewentry),

	/* actionlist */
	CLASSENT(actionlist),

	/* actionedit */
	CLASSENT(actionedit),
	CLASSENT(actioneditwin),

	/* notification */
	CLASSENT(notification),

	/* selectwin */
	CLASSENT(selectwin),

	/* findwin */
	CLASSENT(findwin),
	CLASSENT(findresultlist),

	/* columns list preferences */
	CLASSENT(columnslist),

	/* view selector */
	CLASSENT(viewselectwin),

	/* keyshortcut */
	CLASSENT(keyshortcut),

	/* action editor */
	CLASSENT(actioneditor),
	CLASSENT(actioneditorwin),

	/* configurable delay for background change */
	CLASSENT(bgrefreshdelayslider),

	/* Rename with pattern window */
	CLASSENT(patternrenamewin),

	/* List containing bookmarked locations */
	CLASSENT(bookmarklist),

	/* Window for bookmark creation */
	CLASSENT(addbookmarkwin),
	
	/* IconZoomSlider to zoom icons in IconView */
	CLASSENT(iconzoomslider),

	CLASSENT(mimetype),

	{0, 0, 0, 0}
};


ULONG classes_init(void)
{
	ULONG i;

	for (i = 0; cd[i].name; i++)
	{
		D(CLASS,bug("creating %s..\n", cd[i].name));
		if (!(*(int(*)(void))cd[i].initfunc)() && !cd[i].optional)
		{
			errorreq("class initialization", "Couldn't create class %s.\nThis shouldn't happen. Contact support.", NULL, cd[i].name);
			return (FALSE);
			D(CLASS,bug("error!\n"));
		}
	}
	return (TRUE);
}


void classes_cleanup(void)
{
	LONG i;

	for (i = sizeof(cd) / sizeof(struct classdesc) - 2; i >= 0; i--)
	{
		D(CLASS,bug("closing %s..\n", cd[i].name));
		(*(void(*)(void))cd[i].cleanupfunc)();
	}
}
