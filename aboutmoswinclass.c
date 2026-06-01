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
 * $Id: aboutmoswinclass.c,v 1.11 2025/09/09 12:46:45 jacadcaps Exp $
 */

#include "ambient.h"

/* public */
#include <exec/system.h>
#include <exec/resident.h>
#include <proto/openurl.h>
#include <proto/exec.h>

/* private */
#include "ambient_cat.h"
#include "methodstack.h"
#include "mui_func.h"
#include "copyright.h"
#include "screen.h"
#include "crypto.h"
#include "smartreq.h"
#include "modules.h"
#include "modules/about/libraries/about.h"
#include "modules/about/ppcinline/about.h"
#include "ambient_altivec.h"

#define TXT_APP_LEN   1024

#if USE_CRAWLER
#define TXT_CRAWL_LEN 1536
#endif

APTR aboutmoswin;
extern struct Library *AboutBase; /* Defined in aboutwinclass. */

static TEXT txt_app[TXT_APP_LEN];

#if USE_CRAWLER
static TEXT txt_crawl[TXT_CRAWL_LEN];
#endif

struct Data {
	APTR win_mui;
	APTR bm_logo1;
	APTR bm_logo2;
	APTR crawl;
	ULONG delay;
	ULONG pushid;
	APTR txt_version;
};


DEFNEW
{
	CONST_STRPTR txt_app_template = "\033c\033b· MorphOS ·\033n\n%s";
	CONST_STRPTR txt_web = GSI(MSG_ABOUTWIN_WEBSUPPORT);
	CONST_STRPTR txt_weburl = "\033c\033uhttp://support.morphos-team.net/";
	CONST_STRPTR txt_webdev = GSI(MSG_ABOUTWIN_DEVSUPPORT);
	CONST_STRPTR txt_webdevurl = "\033c\033uhttp://developer.morphosppc.com/";
	CONST_STRPTR txt_mail = GSI(MSG_ABOUTWIN_MAILSUPPORT);
	CONST_STRPTR txt_mailurl = "\033c\033usupport@morphos.de";

	#if USE_CRAWLER
	CONST_STRPTR txt_crawl_template = "\n\n\n\n\n\n\033b%s\033n\n\n\n"
		"Ralph Schmidt\n\n\n"           "Frank Mariak\n\n\n"
		"David Gerber\n\n\n"            "Emmanuel Lesueur\n\n\n"
		"Nicholai Benalal\n\n\n"        "Nicolas Sallin\n\n\n"
		"Harry Sintonen\n\n\n"          "Teemu Suikki\n\n\n"
		"Sigbjørn Skjæret\n\n\n"        "Mark Olsen\n\n\n"
		"Jacek Piszczek\n\n\n"          "Chris Hodges\n\n\n"
		"Benjamin Vernoux\n\n\n"        "Oliver Wagner\n\n\n"
		"Stefan Stuntz\n\n\n"           "Bertrand Presles\n\n\n"
		"Matthieu Leroyer\n\n\n"        "Felix Schwarz\n\n\n"
		"Marcin Kurek\n\n\n"            "Michal Rybinski\n\n\n"
		"Grzegorz Kraszewski\n\n\n"     "Martin Blom\n\n\n"
		"Gurer Özen\n\n\n"              "Michal Wozniak\n\n\n"
		"Troels Walsted Hansen\n\n\n"   "Tomi Ollila\n\n\n"
		"Pekka Pessi\n\n\n"             "Jarno Rajahalme\n\n\n"
		"Markus Peuhkuri\n\n\n"         "John Hendrikx\n\n\n"
		"Bernhard Möllemann\n\n\n"      "Trond Werner Hansen\n\n\n"
		"Marek Szyprowski\n\n\n"        "Ilkka Lehtoranta\n\n\n"
		"Christian Rosentreter\n\n\n"   "Pawel Stefanski\n\n\n"
		"Fabien Coeurjoly\n\n\n"        "AROS\n\n\n"

		"\033b%s\033n\n\n\n"
		"Nicolas Szalski\n\n\n"         "Treveur Bretaudiere\n\n\n"
		"André Siegel\n\n\n"

		"\033b%s\033n\n\n\n"
		"Johan Rönnblom\n\n\n"          "Nicholas Blachford\n\n\n"
		"Emanuel Steen\n\n\n"

		"\033b%s\033n\n\n\n"
		"Robert Reiswig\n\n\n\n\n"

		"%s\n"
		"\n\n\n\n\n\033i%s...\033n\n\n";
	#endif

	struct Data *data;
	APTR bt_web, bt_webdev, bt_mail;
	APTR crawl;
	APTR grp_logo;

	if ((AboutBase = ((AboutBase) ? AboutBase : module_open("PROGDIR:modules/about.amod", 2))))
	{
		/* Use templates and make the localized strings... */
		snprintf(txt_app, TXT_APP_LEN, txt_app_template,
			 GSI(MSG_ABOUTWIN_MOSRELEASE));

		#if USE_CRAWLER
		snprintf(txt_crawl, TXT_CRAWL_LEN, txt_crawl_template, GSI(MSG_ABOUTWIN_PROGRAMMERS),
				 GSI(MSG_ABOUTWIN_GRAPHICS), GSI(MSG_ABOUTWIN_DOCS), GSI(MSG_ABOUTWIN_MISC),
				 GSI(MSG_ABOUTWIN_SCROLLINFO), GSI(MSG_ABOUTWIN_SCROLLRESTART));
		#endif

		obj = DoSuperNew(cl, obj,
			MUIA_Window_PublicScreen, active_screen_name(),
			MUIA_Window_ScreenTitle, screentitle,
			MUIA_Window_Title, "Ambient · About MorphOS",
			MUIA_Window_ID, MAKE_ID('A','M','A','M'),
			MUIA_Window_NoMenus, TRUE,
			MUIA_Window_ShowIconify, FALSE,
			MUIA_Window_ShowSnapshot, FALSE,
			MUIA_Window_ShowAbout, FALSE,
			MUIA_Background, MUII_RequesterBack,
			WindowContents, VGroup,
				MUIA_Background, MUII_RequesterBack,
				Child, ScrollgroupObject,
					MUIA_CycleChain, TRUE,
					MUIA_Scrollgroup_FreeHoriz, FALSE,
					MUIA_Scrollgroup_AutoBars, TRUE,
					MUIA_Scrollgroup_Contents, VirtgroupObject,
						VirtualFrame,
						MUIA_Background, MUII_TextBack,
						Child, grp_logo = HGroup,
						End,

						Child, RectangleObject,
							MUIA_Rectangle_HBar, TRUE,
							MUIA_FixHeight, 8,
						End,

						Child, TextObject,
							MUIA_Text_PreParse, "\033c\033b",
							MUIA_Text_Contents, txt_web,
						End,

						Child, bt_web = TextObject,
							MUIA_Text_Contents, txt_weburl,
							MUIA_InputMode, MUIV_InputMode_RelVerify,
							MUIA_ShowSelState, FALSE,
						End,

						Child, HGroup,
							Child, HSpace(0),
							Child, RectangleObject,
								MUIA_Rectangle_HBar, TRUE, MUIA_FixHeight, 8, MUIA_Weight, 200,
							End,
							Child, HSpace(0),
						End,

						Child, TextObject,
							MUIA_Text_PreParse, "\033c\033b",
							MUIA_Text_Contents, txt_webdev,
						End,

						Child, bt_webdev = TextObject,
							MUIA_Text_Contents, txt_webdevurl,
							MUIA_InputMode, MUIV_InputMode_RelVerify,
							MUIA_ShowSelState, FALSE,
						End,

						Child, HGroup,
							Child, HSpace(0),
							Child, RectangleObject,
								MUIA_Rectangle_HBar, TRUE, MUIA_FixHeight, 8, MUIA_Weight, 200,
							End,
							Child, HSpace(0),
						End,

						Child, TextObject,
							MUIA_Text_PreParse, "\033c\033b",
							MUIA_Text_Contents, txt_mail,
						End,

						Child, bt_mail = TextObject,
							MUIA_Text_Contents, txt_mailurl,
							MUIA_InputMode, MUIV_InputMode_RelVerify,
							MUIA_ShowSelState, FALSE,
						End,

						Child, RectangleObject,
							MUIA_Rectangle_HBar, TRUE, MUIA_FixHeight, 8,
						End,

						#if USE_CRAWLER
						Child, TextObject,
							MUIA_Text_Contents, "\033cHall of Fame",
						End,

						Child, crawl = NewObject(getcrawlclass(), NULL,
							MUIA_FixHeightTxt, "\n\n\n",
							MA_Crawl_Content, txt_crawl,
							MA_Crawl_PreParse, MUIX_C,
						End,
						#endif

						Child, RectangleObject,
							MUIA_Rectangle_HBar, TRUE, MUIA_FixHeight, 8,
						End,

						Child, TextObject,
							MUIA_Text_PreParse, "\33c",
							MUIA_Text_Contents, TaggedOpenLibrary(-2),
						End,

						Child, TextObject,
							MUIA_Text_PreParse, "\33c",
							MUIA_Text_Contents, TaggedOpenLibrary(-3),
						End,

						Child, TextObject,
							MUIA_Text_PreParse, "\33c",
							MUIA_Text_Contents, TaggedOpenLibrary(-4),
						End,

					End,
				End,
			End,
		End;

		if (obj)
		{
			APTR o;

			data = INST_DATA(cl, obj);
			data->crawl = crawl;

			/*
			 * Add logos
			 */
			o = VGroup,
				Child, VSpace(20),
				Child, HGroup,
					Child, HSpace(0),
					Child, data->bm_logo1 = NewObject(getlogoclass(), NULL,
						MA_Logo_Type, ABOUT_LOGO_MORPHOS,
						#if USE_LOGOFLASH
						MA_Logo_Flashing, TRUE,
						MUIA_Bitmap_Alpha, 0x00000001,
						#else
						MUIA_Bitmap_Alpha, 0xffffffff,
						#endif
						MUIA_InputMode, MUIV_InputMode_RelVerify,
						MUIA_ShowSelState, FALSE,
						MUIA_Dropable, TRUE,
					End,
					Child, HSpace(0),
				End,
				Child, VSpace(0),
			End;

			if (o)
			{
				DoMethod(grp_logo, OM_ADDMEMBER, o);

				DoMethod(o, MUIM_Notify, MA_Logo_Egg, TRUE,
					app, 4, MUIM_Application_PushMethod, obj, 1, MM_About_Gag
				);
			}

			data->txt_version = text2(NULL);

			if (data->txt_version)
			{
				struct Resident *mos;

				mos = FindResident("MorphOS");
				DoMethod(data->txt_version, MUIM_SetAsString, MUIA_Text_Contents, txt_app, mos ? mos->rt_Version : 666, mos ? mos->rt_Revision : 666);
				DoMethod(grp_logo, OM_ADDMEMBER, data->txt_version);
			}

			o = VGroup,
				Child, VSpace(20),
				Child, HGroup,
					Child, HSpace(0),
					Child, data->bm_logo2 = NewObject(getlogoclass(), NULL,
						MA_Logo_Type, ABOUT_LOGO_MORPHOS,
						#if USE_LOGOFLASH
						MA_Logo_Flashing, TRUE,
						MUIA_Bitmap_Alpha, 0x00000001,
						#else
						MUIA_Bitmap_Alpha, 0xffffffff,
						#endif
						MUIA_InputMode, MUIV_InputMode_RelVerify,
						MUIA_ShowSelState, FALSE,
					End,
					Child, HSpace(0),
				End,
				Child, VSpace(0),
			End;

			if (o)
			{
				DoMethod(grp_logo, OM_ADDMEMBER, o);
			}

			/*
			 * Close
			 */
			DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
				obj, 1, MM_About_Close
			);

			/*
			 * Web support
			 */
			DoMethod(bt_web, MUIM_Notify, MUIA_Pressed, FALSE,
				obj, 2, MM_About_GotoURL, "http://support.morphos-team.net/"
			);

			/*
			 * Developer support
			 */
			DoMethod(bt_webdev, MUIM_Notify, MUIA_Pressed, FALSE,
				obj, 2, MM_About_GotoURL, "http://developer.morphosppc.com/"
			);

			/*
			 * Mail support
			 */
			DoMethod(bt_mail, MUIM_Notify, MUIA_Pressed, FALSE,
				obj, 2, MM_About_GotoURL, "mailto:support@morphos.de?subject=MorphOS support request"
			);

			#if USE_LOGOFLASH
			data->pushid = DoMethod(app, MUIM_Application_PushMethod, obj, (1 | MUIV_PushMethod_Delay(2000)), MM_About_TriggerObscure);
			#endif

			data->delay = 10000;

		}
		else
		{
			if (!aboutwin)
			{
				module_close(AboutBase);
				AboutBase = NULL;
			}
		}
	}
	else
	{
		smartreq_info("Ambient About", MV_Notification_Error, "Couldn't open about.amod.", NULL);
		return ((ULONG)NULL);
	}
	return ((ULONG)obj);
}


DEFDISPOSE
{
	GETDATA;

	if (!aboutwin)
	{
		module_close(AboutBase);
		AboutBase = NULL;
	}

	if (data->pushid)
	{
		DoMethod(app, MUIM_Application_KillPushMethod, obj, data->pushid);
		data->pushid = 0;
	}
	return (DOSUPER);
}


DEFGET
{
	switch (msg->opg_AttrID)
	{
		case MA_Window_ID:
			*msg->opg_Storage = 0;
			return (TRUE);

		case MA_Window_Path:
			*msg->opg_Storage = MV_Window_Path_None;
			return (TRUE);

		case MA_Window_Type:
			*msg->opg_Storage = MV_Window_Type_About;
			return (TRUE);
	}
	return (DOSUPER);
}


DEFTMETHOD(About_Close)
{
	methodstack_push(_app(obj), 2, MM_Application_DisposeWindow, obj);
	aboutmoswin = NULL;
	return (0);
}


DEFTMETHOD(About_OpenMUI)
{
	GETDATA;

	if (!data->win_mui)
	{
		data->win_mui = AboutmuiObject,
			MUIA_Window_RefWindow, obj,
			MUIA_Aboutmui_Application, app,
		End;
	}

	if (data->win_mui)
	{
		set(data->win_mui, MUIA_Window_Open, TRUE);
	}
	else
	{
		errormsg(ERR_NOMEM);
	}

	return (0);
}


DEFSMETHOD(About_GotoURL)
{
	if (OpenURLBase)
	{
		URL_OpenA(msg->url, NULL);
	}
	return (0);
}


#if USE_LOGOFLASH
DEFTMETHOD(About_TriggerObscure)
{
	GETDATA;

	data->pushid = DoMethod(app, MUIM_Application_PushMethod, obj, (1 | MUIV_PushMethod_Delay(data->delay)), MM_About_TriggerObscure);

	DoMethod(data->bm_logo1, MM_Logo_Flash);
	DoMethod(data->bm_logo2, MM_Logo_Flash);

	return (0);
}
#endif


DEFTMETHOD(About_Gag)
{
	GETDATA;

	#if USE_CRAWLER
	set(data->crawl, MA_Crawl_Content, crypto_decrypt_txt(About_GetText(ABOUT_TEXT_QUOTES_CRYPT)));
	#endif

	return (0);
}


BEGINMTABLE
DECNEW
DECGET
DECDISPOSE
DECTMETHOD(About_Close)
DECTMETHOD(About_OpenMUI)
DECSMETHOD(About_GotoURL)
#if USE_LOGOFLASH
DECTMETHOD(About_TriggerObscure)
#endif
DECTMETHOD(About_Gag)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, aboutmosclass)
