/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber
 * Copyright 2005-2008 Ambient Open Source Team
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
 * $Id: aboutwinclass.c,v 1.24 2025/09/09 12:46:45 jacadcaps Exp $
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
#include "input.h"

#define TXT_APP_LEN   1024

#if USE_CRAWLER
//#define TXT_CRAWL_LEN 1536
#define TXT_CRAWL_LEN 1600  /* this is a wild guess. This should be dynamically allocated (geit) */
#endif

APTR aboutwin;
struct Library *AboutBase = NULL;

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
	STRPTR staffroll;
};


DEFNEW
{
	CONST_STRPTR txt_app_template = "\033c\033b· "APPNAME" ·\033n\n"
			                          "%s\n\n"
	   		                       #ifdef DEBUG
	         		                 APPNAME " %s: "LVERTAG" [debug]\n\n"
	               		           #else
	                     		     #ifdef SHOW_RELEASE
			                          APPNAME " %s: "LVERTAG" [demo]\n\n"
	   		                       #else
	         		                 APPNAME " %s: "LVERTAG"\n\n"
	               		           #endif
	                     		     #endif
			                          "Copyright © 2001-2005 David Gerber\n"
	   		                       "Copyright © 2005-" COPYRIGHTYEAR_END " Ambient Open Source Team\n"
	         		                 "%s.\n"
	               		           "%s."
	                     		     "";

	CONST_STRPTR txt_web = GSI(MSG_ABOUTWIN_WEBSUPPORT);
	CONST_STRPTR txt_weburl = "\033c\033uhttps://morphosambient.sf.net/";
	CONST_STRPTR txt_mui = "\033c\033bMUI\033n © 1992-" COPYRIGHTYEAR_END " Stefan Stuntz.";
	CONST_STRPTR txt_mui_more = GSI(MSG_ABOUTWIN_MUI);
	CONST_STRPTR txt_md5 = GSI(MSG_ABOUTWIN_MD5);
	CONST_STRPTR txt_zlib = "\033c\033bzlib\033n © Jean-loup Gailly and Mark Adler.";
	CONST_STRPTR txt_pnglib = "\033c\033blibpng\033n © Guy Eric Schalnat, Group 42, Inc,\n"
	                    "Andreas Dilger, Glenn Randers-Pehrson.";

	#if USE_ALTIVEC
	CONST_STRPTR txt_altivec = GSI(MSG_ABOUTWIN_ALTIVEC);
	#endif

	#if USE_CRAWLER
	CONST_STRPTR txt_crawl_template = "\n\n\n\n\n\n\033b%s\033n\n"
		"(in alphabetical order)\n\n"
		"Vladimir §scf§ Alaev\n\n"
		"Karoly §Chain-Q§ Balogh\n\n"
		"Fabien §Fab1§ Coeurjoly\n\n"
		"Thomas §bitRocky§ Igracki\n\n"
		"Stefan §kronos§ Kleinheinrich\n\n"
		"Ilkka §itix§ Lehtoranta\n\n"
		"Guido §geit§ Mersmann\n\n"
		"Gunther §gnikl§ Nikl\n\n"
		"Christian §tokai§ Rosentreter\n\n"
		"Harry §piru§ Sintonen\n\n"
		"Lukas §luky§ Stehlik\n\n"
		"Adam §ChaoZer§ Waldenberg\n\n"
		"Michal §kiero§ Wozniak\n\n"
		"\n"

		"\033bTranslations:\033b\n\n"
		"[czech]\n"
		"Lukas §luky§ Stehlik\n\n"
		"[dansk]\n"
		"Benny Damsgaard\n\n"
		"[deutsch]\n"
		"Guido §geit§ Mersmann\n"
		"Carsten §Pegasos-Sigi§ Siegner\n\n"
		"[español]\n"
		"Antonio Noguera\n"
		"Hugo García\n"
		"Jaime Cagigal\n\n"
		"[français]\n"
		"Joël §FALCON-1§ Ehret\n"
		"Jean-Marc §CptBLOOD§ Mossu\n\n"
		"[italiano]\n"
		"Andrea §raistlin77it§ Beretta\n\n"
		"[magyar]\n"
		"Karoly §Chain-Q§ Balogh\n\n"
		"[polski]\n"
		"Grzegorz §grxmrx§ Murdzek\n"
		"Marcin §Kornl§ Kornas\n"
		"Grzegorz §Fei§ Juraszek\n\n"
		"[suomi]\n"
		"Ilkka §itix§ Lehtoranta\n\n"
		"[svenska]\n"
		"Adam §ChaoZer§ Waldenberg\n\n"
		"[russian]\n"
		"Andrei Shestakov\n\n"
		"\n\n"

		"\033bThanks to:\033n\n\n"
		"Nicolas §ocinel§ Szalski\n"
		"for the default icons\n\n"
		"Stefan §polluks§ Haubenthal\n"
		"for providing patches\n\n"
		"André §JoBBo§ Siegel\n"
		"for the 'My MorphOS' icon and prefs gfx\n\n"
		"Nicolas §NicoPPC§ Det\n"
		"for providing patches\n\n"
		"Alexey §AmiS§ Ivanov\n"
		"for something we don't know or remember\n\n"
		"and\n\n"
		"Vladimir §vovka§ Javorsky\n"
		"for the 'blue arrow'\n\n"
		"\n\n"

		"\033bSpecial thanks:\033n\n\n"
		"Nicolas §henes§ Sallin\n\n"
		"Mark §bigfoot§ Olsen\n\n"
		"and\n\n"
		"the rest of the MorphOS Team\n\n"
		"\n\n"
		"\033bWeb\033n\n\n"
		"Nicolas §Leo§ Ramz\n"
		"for the webdesign\n\n"
		"André §JoBBo§ Siegel\n"
		"for the web icons\n\n"
		"Gunne Steen\n"
		"for doing all the hard and manual work to keep the nightlies up-to-date\n\n"
		"Lukas §luky§ Stehlik\n"
		"for creating Ambient homepage and nightly builds\n\n"

		"\n\n"
		"%s\n"
		"\n\n\n\n\n\033i%s...\033n\n\n";
	#endif

	struct Data *data;
	APTR bt_mui, bt_web, bt_zlib, bt_libpng;
	APTR crawl;
	APTR grp_logo;

	#if USE_ALTIVEC
	APTR bt_altivec;
	#endif

	if ((AboutBase = (AboutBase) ? AboutBase : module_open("PROGDIR:modules/about.amod", 2)))
	{
		/* Use templates and make the localized strings... */
		snprintf(txt_app, TXT_APP_LEN, txt_app_template, GSI(MSG_ABOUTWIN_MOSDESKTOP),
			 GSI(MSG_ABOUTWIN_VERSION), GSI(MSG_ABOUTWIN_ALLRIGHTS),GSI(MSG_ABOUTWIN_GPL));

		#if USE_CRAWLER
		snprintf(txt_crawl, TXT_CRAWL_LEN, txt_crawl_template, GSI(MSG_ABOUTWIN_AMBIENTTEAM),
				 GSI(MSG_ABOUTWIN_SCROLLINFO), GSI(MSG_ABOUTWIN_SCROLLRESTART));
		#endif

		obj = DoSuperNew(cl, obj,
			MUIA_Window_PublicScreen, active_screen_name(),
			MUIA_Window_ScreenTitle, screentitle,
			MUIA_Window_Title, GSI( MSG_ABOUTWIN_AMBIENTTITLE ),
			MUIA_Window_ID, MAKE_ID('A','M','A','B'),
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

						Child, RectangleObject,
							MUIA_Rectangle_HBar, TRUE, MUIA_FixHeight, 8,
						End,

						#if USE_CRAWLER
						Child, TextObject,
							MUIA_Text_Contents, "\033cHall of Fame",
						End,

						Child, crawl = NewObject(getcrawlclass(), NULL,
							MUIA_FixHeightTxt, "\n\n\n",
							MA_Crawl_Content,  txt_crawl,
							MA_Crawl_PreParse, MUIX_C,
						End,

						Child, RectangleObject,
								MUIA_Rectangle_HBar, TRUE, MUIA_FixHeight, 8,
						End,

						#endif

						Child, HGroup,

							Child, HSpace(0),

							Child, bt_mui = NewObject(getlogoclass(), NULL,
								MA_Logo_Type, ABOUT_LOGO_MUI,
								MUIA_Bitmap_Alpha, 0xffffffff,
								MUIA_InputMode, MUIV_InputMode_RelVerify,
								MUIA_ShowSelState, FALSE,
							End,

							Child, VGroup, GroupSpacing(0),
								Child, TextObject,
									MUIA_Text_PreParse, "\033c",
									MUIA_Text_Contents, txt_mui,
								End,
								Child, TextObject,
									MUIA_Text_PreParse, "\033c",
									MUIA_Text_Contents, txt_mui_more,
								End,
								Child, VSpace(0),
							End,

							Child, HSpace(0),

						End,

						Child, HGroup,

							Child, HSpace(0),

							Child, bt_zlib = NewObject(getlogoclass(), NULL,
								MA_Logo_Type, ABOUT_LOGO_ZLIB,
								MUIA_Bitmap_Alpha, 0xffffffff,
								MUIA_InputMode, MUIV_InputMode_RelVerify,
								MUIA_ShowSelState, FALSE,
							End,

							Child, TextObject,
								MUIA_Text_Contents, txt_zlib,
							End,

							Child, HSpace(0),

						End,

						Child, HGroup,

							Child, HSpace(0),

							Child, bt_libpng = NewObject(getlogoclass(), NULL,
								MA_Logo_Type, ABOUT_LOGO_LIBPNG,
								MUIA_Bitmap_Alpha, 0xffffffff,
								MUIA_InputMode, MUIV_InputMode_RelVerify,
								MUIA_ShowSelState, FALSE,
							End,

							Child, TextObject,
								MUIA_Text_Contents, txt_pnglib,
							End,

							Child, HSpace(0),

						End,

						Child, HGroup,

							Child, HSpace(0),

							Child, TextObject,
								MUIA_Text_PreParse, "\033c",
								MUIA_Text_Contents, txt_md5,
							End,

							Child, HSpace(0),

						End,

						#if USE_ALTIVEC
						Child, HGroup,

							Child, HSpace(0),

							Child, bt_altivec = NewObject(getlogoclass(), NULL,
								MA_Logo_Type, ABOUT_LOGO_ALTIVEC,
								MUIA_Bitmap_Alpha, 0xffffffff,
								MUIA_InputMode, MUIV_InputMode_RelVerify,
								MUIA_ShowSelState, FALSE,

							End,

							Child, TextObject,
								MUIA_Text_PreParse, "\033c",
								MUIA_Text_Contents, txt_altivec,
							End,

							Child, HSpace(0),

						End,
						#endif
					End,
				End,
			End,
		End;

		if (obj)
		{
			APTR o;

			data = INST_DATA(cl, obj);
			data->crawl = crawl;


			/*  initialize staff roll
			 */
			{
				ULONG slen = strlen(txt_crawl);
				BOOL nicks = (check_qualifier(IEQUALIFIER_RSHIFT) && check_qualifier(IEQUALIFIER_LSHIFT));

				if ((data->staffroll = malloc(slen+1)))
				{
					STRPTR s = txt_crawl;
					STRPTR d = data->staffroll;
					UBYTE c;
					BOOL flipflop = TRUE;

					while(slen)
					{
						c = *s;

						if (c == '§')
						{
							if (nicks)
							{
								*d = '\'';
								d++;
							}
							else
							{
								if (!flipflop) /* jump a char to avoid double spaces */
								{
									s++;
									slen--;
								}

								flipflop = !flipflop;
							}
						}
						else
						{
							if (flipflop)
							{
								*d = c;
								d++;
							}
						}

						s++;
						slen--;
					}

					*d = '\0'; /* terminate string */

					set(crawl, MA_Crawl_Content, data->staffroll);
				}
				/* do not care for error case, but hall of fame will look a bit weird then ;) */
			}

			/*
			 *  Add logos
			 */
			o = VGroup,
				Child, VSpace(20),
				Child, HGroup,
					Child, HSpace(0),
					Child, data->bm_logo1 = NewObject(getlogoclass(), NULL,
						MA_Logo_Type, ABOUT_LOGO_AMBIENT,
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
				DoMethod(data->txt_version, MUIM_SetAsString, MUIA_Text_Contents, txt_app);
				DoMethod(grp_logo, OM_ADDMEMBER, data->txt_version);
			}

			o = VGroup,
				Child, VSpace(20),
				Child, HGroup,
					Child, HSpace(0),
					Child, data->bm_logo2 = NewObject(getlogoclass(), NULL,
						MA_Logo_Type, ABOUT_LOGO_AMBIENT,
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
			 * Register MUI
			 */
			DoMethod(bt_mui, MUIM_Notify, MUIA_Pressed, FALSE,
				obj, 1, MM_About_OpenMUI
			);

			#if USE_ALTIVEC
			/*
			 * AltiVec
			 */
			DoMethod(bt_altivec, MUIM_Notify, MUIA_Pressed, FALSE,
				obj, 2, MM_About_GotoURL, "http://en.wikipedia.org/wiki/AltiVec"
			);
			#endif

			/*
			 * zlib
			 */
			DoMethod(bt_zlib, MUIM_Notify, MUIA_Pressed, FALSE,
				obj, 2, MM_About_GotoURL, "http://www.zlib.org/"
			);

			/*
			 * libpng
			 */
			DoMethod(bt_libpng, MUIM_Notify, MUIA_Pressed, FALSE,
				obj, 2, MM_About_GotoURL, "http://www.libpng.org/"
			);

			/*
			 * Web support
			 */
			DoMethod(bt_web, MUIM_Notify, MUIA_Pressed, FALSE,
				obj, 2, MM_About_GotoURL, "http://morphosambient.sf.net/"
			);

			#if USE_LOGOFLASH
			data->pushid = DoMethod(app, MUIM_Application_PushMethod, obj, (1 | MUIV_PushMethod_Delay(2000)), MM_About_TriggerObscure);
			#endif

			data->delay = 10000;

		}
		else
		{
			if (!aboutmoswin)
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

	if (!aboutmoswin)
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
	aboutwin = NULL;
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

DECSUBCLASS_NC(MUIC_Window, aboutclass)
