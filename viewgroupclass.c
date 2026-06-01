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
 * $Id: viewgroupclass.c,v 1.23 2026/03/21 20:36:34 jacadcaps Exp $
 */

#include "ambient.h"

/* public */
#include <proto/dos.h>
#include <devices/rawkeycodes.h>

/* private */
#include "mui_func.h"
#include "threads.h"
#include "mimeuri.h"
#include "gfx_pen.h"
#include "prefs.h"
#include "viewapi.h"
#include "fmt.h"
#include "vfs.h"
#include "keymap.h"
#include "methodstack.h"
#include "viewapi.h"
#include "rexx.h"
#include "deficonpool.h"
#include "file_func.h"
void dprintf(char *, ...) __attribute__ ((format (printf, 1, 2)));

#define WINDOWTITLESIZE 1024 /* should be enough for everyone (tm) */

//static LONG get_viewindex_from_args(APTR ctx);
// bitRocky: used in MM_View_Focus, should they go into struct Data?
LONG prev_iselect = 0, prev_len = 0;

#define isrootOrExtra (data->isroot || data->isrootExtra)

struct Data {
	ULONG isroot;
	ULONG isrootExtra;
	ULONG screenID;
	ULONG currentview;
	ULONG currentindex;
	ULONG pendingview;
	ULONG pendingindex;
	APTR subgrp;
	APTR subview;
	APTR mimectx;
	/* delayed positioning infos */
	ULONG dopos;
	ULONG top;
	ULONG left;
	ULONG aftersetup;
	ULONG dotitle;
	TEXT windowtitle[WINDOWTITLESIZE];
	/* delayed close */
	APTR winsave;
	/* pens */
	ULONG bgpen;
	/* scrollgroup */
	APTR scrollgrp;

	/* scrollers */

	APTR hscroller;
	APTR vscroller;

	STRPTR previouspath;

	/* event handler */
	struct MUI_EventHandlerNode ehnode;
	APTR str_search;
	ULONG searchedit;

	/* id */

	ULONG viewid;

};
#if 0
static LONG get_viewindex_from_args(APTR ctx)
{
	struct mimeuri_context * ct;
	struct v_args args;
	LONG res = -1;

	memset(&args, 0, sizeof(args));

	if( (ct = mimeuri_create()) )
	{
		mimeuri_setattrs(ct, MIMEURIATTR_ARGS, mimeuri_getattr(ctx, MIMEURIATTR_ARGS), TAG_DONE);

		if(	mimeuri_readargs(ct, VIEW_TEMPLATE, (LONG *) &args) )
		{
			if(args.view)
			{
				struct viewnode * vn = viewapi_findbyname(args.view);
				if(vn)
				{
					res = vn->id;
				}
			}
			mimeuri_delete(ct);
		}
	}
	return (res);
}
#endif

static void doset(APTR obj, struct Data *data, struct TagItem *tags)
{
	ULONG changeview = FALSE;
	char *newpath = NULL;

	FORTAG(tags)
	{
		case MA_Viewgroup_ID:
			data->viewid = tag->ti_Data;
			break;

		case MA_View_IsRoot:
			data->isroot = tag->ti_Data;
			break;
		
		case MA_View_IsRootExtra:
			data->isrootExtra = tag->ti_Data;
			break;
			
		case MA_View_ScreenID:
			data->screenID = tag->ti_Data;
			break;

		case MA_Viewgroup_MIMEctx:
			if (tag->ti_Data)
			{
				if (data->mimectx)
				{
					STRPTR s1, s2;                  
	 
					s1 = mimeuri_getattr(data->mimectx, MIMEURIATTR_MIMETYPE);
					s2 = mimeuri_getattr((APTR)tag->ti_Data, MIMEURIATTR_MIMETYPE);

					/*
					 * We check if currentview can display new contents
					 * (mimetype) specified by s2 string.
					 * viewapi_checkmime() returns FALSE for incompatible types.
					 */

					if (!s1 || !s2 || !viewapi_checkmime(data->currentview, s2))
					{
						/* vfs stuff in case of a view change, sigh */
						STRPTR previouspath = (STRPTR) getv(obj, MA_View_PreviousPath);
						STRPTR temppath = (STRPTR) mimeuri_getattr((APTR)tag->ti_Data, MIMEURIATTR_PATH);

						if(!temppath) // means new view is a device list, release vfs.
						{
							vfs_release(_win(obj), previouspath);
						}
						else // don't release vfs if new view volume hasn't changed.
						{
							if(previouspath && !same_volume_nolock(temppath, previouspath))
							{
								vfs_release(_win(obj), previouspath);
							}
						}

						set(obj, MA_View_PreviousPath, temppath ? temppath : (STRPTR) "");

						/* flag to signal view change */
						changeview = TRUE;
					}
					else
					{
						/*
						 * If view doesn't have to be changed, we obtain path for
						 * new contents of current view.
						 */

						newpath = mimeuri_getattr((APTR)tag->ti_Data, MIMEURIATTR_URI);
					}

					mimeuri_delete(data->mimectx);
				}
				else
				{
					changeview = TRUE;
				}
				data->mimectx = (APTR)tag->ti_Data;
			}
			break;

		case MA_View_Path: /* only used by imageview atm */
			if (data->mimectx)
			{
				mimeuri_setattrs(data->mimectx,
					MIMEURIATTR_PATH, tag->ti_Data,
				TAG_DONE);
			}
			tag->ti_Tag = TAG_IGNORE; /* skip, we don't want it to come back */
			break;

		case MA_View_Scheme:
			if (data->mimectx)
			{
				mimeuri_setattrs(data->mimectx,
					MIMEURIATTR_SCHEME, tag->ti_Data,
				TAG_DONE);
			}
			break;

		case MA_View_PreviousPath:

			if(data->previouspath)
			{
				free(data->previouspath);
			}

			data->previouspath = (STRPTR) malloc(strlen((STRPTR)tag->ti_Data)+1);
			if (data->previouspath)
			{
				strcpy(data->previouspath, (STRPTR) tag->ti_Data);
			}
			break;

		case MA_Viewgroup_ViewChanged: /* Only for notification purposes */
			break;

		case MA_View_ShowSearchString:
		{
			if( tag->ti_Data != 0)
			{
				DoMethod( obj, MUIM_Group_InitChange );
				DoMethod( obj, OM_ADDMEMBER, data->str_search );
				DoMethod( obj, MUIM_Group_ExitChange );

				set(data->str_search, MUIA_ShowMe, TRUE);
				/* XXX: workaround intuition delayed inputevent stuff (mui4 required) */
				DoMethod( app, MUIM_Application_PushMethod, _win(obj), 3 | MUIV_PushMethod_Delay(20), MUIM_NoNotifySet, MUIA_Window_ActiveObject, data->str_search );
				//nnset(_win(obj), MUIA_Window_ActiveObject, data->str_search);

				data->searchedit = TRUE;
			}
			else
			{
				if(data->searchedit)
				{
					nnset(data->str_search, MUIA_String_Contents, "");
					nnset(_win(obj), MUIA_Window_ActiveObject, NULL);

					//set(data->str_search, MUIA_ShowMe, FALSE);
					DoMethod( obj, MUIM_Group_InitChange );
					DoMethod( obj, OM_REMMEMBER, data->str_search );
					DoMethod( obj, MUIM_Group_ExitChange );

					nnset(data->str_search, MUIA_Background, MUII_StringActiveBack ); // bitRocky

					data->searchedit = FALSE;
				}
			}
			break;
		}
	}
	NEXTTAG

	if (changeview)
	{
		STRPTR s;

		s = mimeuri_getattr(data->mimectx, MIMEURIATTR_SCHEME);

		/* We use mode argument to enforce a *compatible* view mode, such as list for example */
		if ((s = mimeuri_getattr(data->mimectx, MIMEURIATTR_MIMETYPE)))
		{
			struct viewnode *vn = NULL;
			struct viewnode *firstvn = NULL;
			ULONG found = FALSE;
			struct v_args args;
			STRPTR preferredview     = NULL;
			STRPTR preferredviewmode = NULL;

			memset( &args, 0, sizeof( struct v_args ) );

			if (DoMethod(obj, MM_View_ReadArgs, VIEW_TEMPLATE, &args))
			{
				if (args.view)
				{
					preferredview = args.view;
				}

				if (args.mode)
				{
					preferredviewmode = args.mode;
				}
			}

			firstvn = vn = viewapi_findbymime( s );

			if ( preferredview )
			{
				while ( vn && !found )
				{
					if(!stricmp(vn->name, preferredview))
					{
						found = TRUE;
						break;
					}

					vn = viewapi_nextviewmime( vn, s );
				}
			}
			else
			{
				struct viewnode * v;

				/*
				 * We try to keep current class if target mimetype can be displayed,
				 * instead of picking first compatible view.
				 */

				v = viewapi_findbyid(data->currentview);

				if(v)
				{
					while(vn && !found)
					{
						if(v->cl == vn->cl)
						{
							found = TRUE;
							break;
						}

						vn = viewapi_nextviewmime(vn, s);
					}		   
				}
			}

			if ( !found )
				vn = firstvn;	/* we use first available one if no match */

			if( vn )
			{
				TEXT mode[ 64 ];

				if ( preferredviewmode )
					snprintf( mode, sizeof( mode ), "%s %s", vn->name, preferredviewmode );
				else
					snprintf( mode, sizeof( mode ), "%s", vn->name );

				DoMethod(obj, MM_Viewgroup_ChangeView, mode);
			}
		}
		else
		{
			DB(("Couldn't get mimetype!\n"));
		}
	}
	else
	{
		/*
		 * If not changeview, we have to send a new URI to the current view.
		 * We shouldn't need to check for root window as at earlier stage
		 * rootwin should always force new window opening, but better safe
		 * than sorry.
		 * TODO: newpath is obtained from newly passed mimectx above. Maybe
		 * remove it and only get path here?
		 */

		if	( !isrootOrExtra && newpath )
		{
			ULONG index;
			STRPTR previouspath = (STRPTR) getv(obj, MA_View_PreviousPath);
			STRPTR temppath = (STRPTR) getv(obj, MA_View_Path);

			if(previouspath && !same_volume_nolock(temppath, previouspath))
			{
				vfs_release(_win(obj), previouspath);
			}
			set(obj, MA_View_PreviousPath, temppath);


			DoMethod( obj, MM_View_LoadURI, newpath  );

			/*
			 * Check if the view decided to change the
			 * index by itself (for examply because URI contained mode param).
			 */

			if ( get(data->subview, MA_View_ModeIndex, &index) )
			{
				/*
				 * We also update pending index to eliminate any further changes
				 * of viewmode (ambient's threads are a mess sometimes and it was
				 * only way i managed to make it working. Kiero.
				 */

				if ( index != data->currentindex )
				{
					data->currentindex = index;
					set( obj, MA_Viewgroup_ViewChanged, TRUE );
				}

				data->pendingindex = index;
			}
		}
	}
}


DEFNEW
{
	APTR searchstring = NULL;

	struct Data *data;

	obj = DoSuperNew(cl, obj,
		MUIA_FillArea, FALSE,
		MUIA_InnerBottom, 0,
		MUIA_InnerLeft, 0,
		MUIA_InnerRight, 0,
		MUIA_InnerTop, 0,
		#if 0
		Child, searchstring = StringObject, StringFrame,
			MUIA_String_Reject, "/:",
			MUIA_String_MaxLen, NAME_SIZE,
			MUIA_CycleChain, 1,
			MUIA_ShowMe, FALSE,
		End,
		#endif
	End;


	if (!obj)
	{
		return ((ULONG)NULL);
	}

	data = INST_DATA(cl, obj);

	data->previouspath = NULL;
	data->searchedit = 0;
	
	doset(obj, data, INITTAGS);
 
	if ( !isrootOrExtra )
	{
		searchstring = StringObject, StringFrame,
			MUIA_String_Reject, "/:",
			MUIA_String_MaxLen, NAME_SIZE,
			MUIA_CycleChain, 1,
			End;

		data->str_search = searchstring;

		DoMethod(data->str_search, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, obj, 3, MUIM_Set, MA_View_ShowSearchString, FALSE);
		DoMethod(data->str_search, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 4, MM_View_Focus, TRUE, MUIV_TriggerValue, FALSE);
	}
	else
	{
		data->str_search = NULL;
	}


	return ((ULONG)obj);
}


DEFDISP
{
	GETDATA;
	ULONG rc;
	LONG viewid = data->viewid;

	if ( data->mimectx )
	{
		mimeuri_delete(data->mimectx);
	}

	if( data->previouspath )
	{
		free(data->previouspath);
	}

	if ( data->str_search && !_parent( data->str_search ) )
	{
		MUI_DisposeObject( data->str_search );
	}

	rc = DOSUPER;

	deficonpool_delete_mime_list(viewid);

	return rc;
}


DEFTMETHOD(Viewgroup_GetBgPen)
{
	GETDATA;

	if (data->aftersetup)
	{
		data->bgpen = gfx_get_penspec_value(muiRenderInfo(obj), isrootOrExtra ? getprefs(DSI_BACKGROUND_ROOT_BGCOLOR) : &_conf(window_bgcolor));
		if (muiRenderInfo(obj) && _win(obj))
		{
			set(_win(obj), MA_Window_BgPen, data->bgpen);
		}
	}
	return (0);
}


DEFMMETHOD(Setup)
{
	ULONG rc;

	if ((rc = DOSUPER))
	{
		GETDATA;

		data->winsave = NULL;

		data->aftersetup = TRUE;

		DoMethod(obj, MM_Viewgroup_GetBgPen);

		if (data->dopos)
		{
			if (muiRenderInfo(obj) && _win(obj))
			{
				SetAttrs(_win(obj),
					MUIA_Window_TopEdge, data->top,
					MUIA_Window_LeftEdge, data->left,
				TAG_DONE);
				
				data->dopos = FALSE;
			}
		}
		if (data->dotitle)
		{
			if (muiRenderInfo(obj) && _win(obj))
			{
				DoMethod(_win(obj), MM_Window_SetTitle, data->windowtitle);
				data->dotitle = FALSE;
			}
		}
		if (data->subview)
		{
			DoMethod(data->subview, MM_View_Setup); /* at this point subview can request pens, etc.. */
		
			if (muiRenderInfo(obj) && _win(obj))
			{
				ULONG val = val; /* shut up gcc */

				if (!get(data->subview, MA_View_NeedsBackfill, &val))
				{
					val = FALSE;
				}
				set(_win(obj), MA_Window_DoBackfill, val);
			}
		}		 

		/* setup eventhandler for 'jump to' function */

		if ( !isrootOrExtra )
		{
			data->ehnode.ehn_Object = obj;
			data->ehnode.ehn_Class = cl;
			data->ehnode.ehn_Events =  IDCMP_RAWKEY;
			data->ehnode.ehn_Priority = 2; /* priority over MUI's areaclass */
			data->ehnode.ehn_Flags = MUI_EHF_GUIMODE;
			DoMethod(_win(obj), MUIM_Window_AddEventHandler, (ULONG)&data->ehnode);

			/* hide string if it becomes deactivated */

			DoMethod(_win(obj), MUIM_Notify, MUIA_Window_ActiveObject, MUIV_EveryTime, obj, 3, MUIM_Set, MA_View_ShowSearchString, FALSE );
		}
	}

	return (rc);
}


DEFMMETHOD(Cleanup)
{
	GETDATA;

	data->aftersetup = FALSE;
	data->dopos = FALSE;
	data->winsave = _win(obj); /* if the window is closed we cannot use _win(obj) anymore so we save it here */

	if ( !isrootOrExtra )
	{
		DoMethod(_win(obj), MUIM_Window_RemEventHandler, (ULONG)&data->ehnode);
		DoMethod(_win(obj), MUIM_KillNotifyObj, MUIA_Window_ActiveObject, obj );
	}

	if (data->subview)
	{
		DoMethod(data->subview, MM_View_Cleanup);
	}

	return (DOSUPER);
}

/* this handler is responsible for search string */
DEFMMETHOD(HandleEvent)
{
	GETDATA;

	if (msg->imsg)
	{
		switch (msg->imsg->Class)
		{
			case IDCMP_RAWKEY:
			{
				struct viewnode *vn = viewapi_findbyid( data->currentview );
				ULONG flags = viewapi_getflags( vn );

				if ( flags & VF_SEARCHABLE )
				{
					ULONG code = msg->imsg->Code & 0x7F;

					if(code)
					{
						switch(code)
						{
							case RAWKEY_ESCAPE:
								if(getv(obj, MA_View_ShowSearchString))
								{
									set(obj, MA_View_ShowSearchString, FALSE);
	                                return (MUI_EventHandlerRC_Eat);
								}
								break;

							case RAWKEY_UP:
							case RAWKEY_DOWN:
								if (data->searchedit)
								{
									PDB(("Cursor %s, prev_iselect = %ld\n", code == RAWKEY_UP ? "up" : "down", prev_iselect));
									if (code == RAWKEY_UP) prev_iselect--; else prev_iselect++;
									PDB(("prev_iselect = %ld\n", prev_iselect));
									DoMethod(obj, MM_View_Focus, TRUE, getv(data->str_search, MUIA_String_Contents), (code == RAWKEY_UP));
                                	return (MUI_EventHandlerRC_Eat);
								}
								break;

							default:
							{
								/* we may have to filter more */
								if ((msg->imsg->Qualifier & (IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND | IEQUALIFIER_NUMERICPAD)) == 0)
								{
									TEXT c = keymap_vanilla(msg->imsg);

									if (c && isprint(c) && c != '/' && !getv(obj, MA_View_ShowSearchString))
									{
										TEXT buf[2];

										buf[0] = c; buf[1] = '\0'; prev_iselect = 0;
										set(obj, MA_View_ShowSearchString, TRUE);
										set(data->str_search, MUIA_String_Contents, buf);
										return (MUI_EventHandlerRC_Eat);
									}
								}
							}

						}
					}
				}
			}
			break;
		}
	}

	return 0;
}

DEFSET
{
	GETDATA;

	doset(obj, data, INITTAGS);

	return (DOSUPER);
}


#define FORWARDGET_SUBVIEW(x) \
	case x: \
		{ \
			if (data->subview) \
			{ \
				ULONG v; \
				if (get(data->subview, x, &v)) \
				{ \
					*msg->opg_Storage = v; \
					return (TRUE); \
				} \
			} \
			return (FALSE); \
		}

DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_Viewgroup_IsView: /* used by _view() */
			*msg->opg_Storage = TRUE;
			return (TRUE);
	
		case MA_View_IsRoot:
			{
				*msg->opg_Storage = data->isroot;
				return (TRUE);
			}
			break;
			
		case MA_View_IsRootExtra:
			{
				*msg->opg_Storage = data->isrootExtra;
				return TRUE;
			}
			break;

		FORWARDGET_SUBVIEW(MA_View_NeedsBackfill)
		FORWARDGET_SUBVIEW(MA_View_HasBackground)

		case MA_View_MIME:
			{
				STRPTR s;

				if ((s = mimeuri_getattr(data->mimectx, MIMEURIATTR_MIMETYPE)))
				{
					*msg->opg_Storage = (ULONG)s;
				}
				return (TRUE);
			}
			break;

		case MA_View_Path:
			{
				STRPTR s;

				/*
				 * We only return the path for local
				 * views, otherwise it's the URI.
				 */
				if ((s = mimeuri_getattr(data->mimectx, MIMEURIATTR_PATH)))
				{
					if (mimeuri_getattr(data->mimectx, MIMEURIATTR_LOCAL))
					{
						*msg->opg_Storage = (ULONG)s;
					}
					else
					{
						if ((s = mimeuri_getattr(data->mimectx, MIMEURIATTR_URI)))
						{
							*msg->opg_Storage = (ULONG)s;
						}
						else
						{
							PDB(("hum.. weird..\n"));
						}
					}
				}
				else
				{
					*msg->opg_Storage = (ULONG)"";
				}
				return (TRUE);
			}
			break;

		case MA_View_URI:
			{
				STRPTR s;

				if ((s = mimeuri_getattr(data->mimectx, MIMEURIATTR_URI)))
				{
					*msg->opg_Storage = (ULONG)s;
				}
				else
				{
					PDB(("hum.. weird..\n"));
					*msg->opg_Storage = (ULONG)"";
				}
				return (TRUE);
			}
			break;

		case MA_View_PreviousPath:
			{
				*msg->opg_Storage = (ULONG) data->previouspath;
				return (TRUE);
			}

		case MA_View_Scheme:
			{
				STRPTR s;

				if ((s = mimeuri_getattr(data->mimectx, MIMEURIATTR_SCHEME)))
				{
						*msg->opg_Storage = (ULONG)s;
						return (TRUE);
				}
				*msg->opg_Storage = 0;
				return (FALSE);
			}
			break;

		case MA_View_BgPen:
			{
				*msg->opg_Storage = data->bgpen;
				return (TRUE);
			}
			break;

		case MA_Viewgroup_ID:
			{
				*msg->opg_Storage = data->viewid;
				return (TRUE);
			}
			break;

		case MUIA_Scrollgroup_HorizBar:
			{
				if (data->scrollgrp)
				{
					*msg->opg_Storage = getv(data->scrollgrp, MUIA_Scrollgroup_HorizBar);
					return (TRUE);
				}
			}
			break;

		case MUIA_Scrollgroup_VertBar:
			{
				if (data->scrollgrp)
				{
					*msg->opg_Storage = getv(data->scrollgrp, MUIA_Scrollgroup_VertBar);
					return (TRUE);
				}
			}
			break;

		case MA_Viewgroup_CurrentView:
			{
				*msg->opg_Storage = (ULONG)data->subview;
				return (TRUE);
			}
			break;

		case MA_Viewgroup_ViewModeIndex:
			{
				*msg->opg_Storage = data->currentindex;
				return (TRUE);
			}
			break;

		case MA_Viewgroup_ViewIndex:
			{
				*msg->opg_Storage = data->currentview;
				return (TRUE);
			}
			break;

		case MA_Viewgroup_ViewChanged:
			{
				*msg->opg_Storage = TRUE;
				return (TRUE);
			}
			break;

		case MA_View_ShowSearchString:
			{
				*msg->opg_Storage = data->searchedit;
				return (TRUE);
			}
			break;
		
		case MA_Viewgroup_MIMEctx:
			{
				*msg->opg_Storage = data->mimectx;
				return (TRUE);
			}
			break;
	}
	return (DOSUPER);
}


#if 0
DEFSMETHOD(View_LoadURI)
{
	execute_command(obj, AC_INTERNAL, "LoadURI <uri_here>"); /* XXX: that sucks.. it should just take a command I think! */
}
#endif


DEFSMETHOD(Viewgroup_ChangeView)
{
	GETDATA;
	struct viewnode *vn;
	STRPTR p;
	TEXT buf[64]; /* should be enough for everyone (tm) */

	stccpy(buf, msg->name, sizeof(buf));

	if ((p = strchr(buf, ' ')))
	{
		*p = '\0';
	}

	//ASSERT( viewapi_findbyname(buf) );

	if( (vn = viewapi_findbyname(buf)) )  /* if is better than ASSERT() - If there is a problem, blame me! Better than dealing with NULL pointer (geit) */
	{
		data->pendingview = vn->id;

		if (p && *(++p))
		{
			data->pendingindex = viewapi_getmodeindex(vn, p);
		}
		else
		{
			data->pendingindex = 0; /* default */
		}

		/*
		 * We adjust URI args in mimectx to reflect the change of viewmode and path.
		 * Use readargs and rebuild the args string. This args string is later used by
		 * views to get position and mode so should reflect current state.
		 */

		{
			struct v_args args;
			memset( &args, 0, sizeof( struct v_args ) );

			if (DoMethod(obj, MM_View_ReadArgs, VIEW_TEMPLATE, &args))
			{
				TEXT newargs[258]; // to remove the strncat() warnings
				TEXT buf[256];

				newargs[0] = '\0';

				if(args.left)
				{
					snprintf(buf, sizeof(buf), "&left=%ld", *args.left);
					strncat(newargs, buf, sizeof(newargs)-1);
				}

				if(args.top)
				{
					snprintf(buf, sizeof(buf), "&top=%ld", *args.top);
					strncat(newargs, buf, sizeof(newargs)-1);
				}

				if(args.width)
				{
					snprintf(buf, sizeof(buf), "&width=%ld", *args.width);
					strncat(newargs, buf, sizeof(newargs)-1);
				}

				if(args.height)
				{
					snprintf(buf, sizeof(buf), "&height=%ld", *args.height);
					strncat(newargs, buf, sizeof(newargs)-1);
				}

				if(args.sortby)
				{
					snprintf(buf, sizeof(buf), "&sortby=%s", args.sortby);
					strncat(newargs, buf, sizeof(newargs)-1);
				}

				if(args.sortorder)
				{
					snprintf(buf, sizeof(buf), "&sortorder=%s", args.sortorder);
					strncat(newargs, buf, sizeof(newargs)-1);
				}
				
				if (p && *p)
				{
					snprintf(buf, sizeof(buf), "&mode=%s", viewapi_getmodename( vn, data->pendingindex ));
					strncat(newargs, buf, sizeof(newargs)-1);
				}
				else if(args.mode)
				{
					data->pendingindex = viewapi_getmodeindex(vn, args.mode);

					snprintf(buf, sizeof(buf), "&mode=%s", args.mode);
					strncat(newargs, buf, sizeof(newargs)-1);
				}
				else
				{
					data->pendingindex = 0; /* default */
				}

				snprintf(buf, sizeof(buf), "&view=%s&type=%s",
										vn->name,
										args.type ? args.type : (STRPTR)"" );
				strncat(newargs, buf, sizeof(newargs)-1);

				mimeuri_setattrs( data->mimectx, MIMEURIATTR_ARGS, newargs, TAG_DONE );
			}
		}

		if (data->pendingview != data->currentview)
		{
			if (data->subview)
			{
				DoMethod(data->subview, MM_View_Abort); /* follows in MM_Viewgroup_Aborted */
			}
			else
			{
				DoMethod(obj, MM_Viewgroup_ChangeView2);
			}
		}
		else
		{
			/*
			 * We can try to change the index otherwise.
			 * XXX: should we send a MM_View_Abort ?
			 */
			if (data->pendingindex != data->currentindex)
			{
				/*
				 * Views are responsible to abort their
				 * own stuff themselves.
				 */
				DoMethod(obj, MM_Viewgroup_ChangeView2);
			}
		}
	}
	return (0);
}


DEFTMETHOD(Viewgroup_Aborted)
{
	GETDATA;

	/*
	 * This is true only after MUIM_Cleanup.
	 */
	if (data->winsave)
	{
		DoMethod(app, MUIM_Application_PushMethod, data->winsave, 1, MM_Window_Aborted); /* windowclass will ignore the following method if there's no pending close */
	}
	else
	{
		/*
		 * The window is not closing, so we
		 * might want to change the view.
		 */
		DoMethod(obj, MM_Viewgroup_ChangeView2);
	}
	return (0);
}


DEFTMETHOD(Viewgroup_ChangeView2)
{
	GETDATA;

	if (data->pendingview != data->currentview)
	{
		ULONG flags = 0;
		struct IClass *classptr = NULL;
		struct viewnode *vn;

		DoMethod(obj, MUIM_Group_InitChange);

		/* remove fastsearch first */

		//DoMethod( obj, OM_REMMEMBER, data->str_search );

		if (data->subgrp)
		{
			if (data->aftersetup)
			{
				DoMethod(data->subview, MM_View_Cleanup);
			}
			DoMethod(obj, OM_REMMEMBER, data->subgrp);
			MUI_DisposeObject(data->subgrp);
			data->subgrp = NULL;
			data->subview = NULL;
		}

		//dprintf("changing subview to %ld\n", msg->type);

		/*
		 * XXX: here we should have a mechanism which fetches what the
		 * view needs. For now it's hardcoded.
		 */

		//ASSERT( viewapi_findbyid(data->pendingview) );

		if( (vn = viewapi_findbyid(data->pendingview)) ) /* if is better than ASSERT() - If there is a problem, blame me! Better than dealing with NULL pointer (geit) */
		{
			classptr = vn->cl;

			flags |= viewapi_getflags( vn );

			if (isrootOrExtra)
			{
				flags &= ~(VF_SCROLLWIN | VF_SCROLLGROUP | VF_SCROLLERS); /* XXX: hm, it could have some scrollers actually.. check */
			}

			if (classptr)
			{
				ULONG rc = FALSE;

				/*
				 * Create a group container that will have a
				 * proper size but is invisible. We are just
				 * interested in the size.
				 */
				data->subgrp = NewObject(getviewsizegroupclass(), NULL,
						MA_Viewsizegroup_IsRoot, isrootOrExtra,
						MUIA_Group_Horiz, TRUE,
						(flags & VF_SPACE) ? TAG_IGNORE : MUIA_FillArea, FALSE,
						(flags & VF_SPACE) ? TAG_IGNORE : MUIA_InnerLeft, 0,
						(flags & VF_SPACE) ? TAG_IGNORE : MUIA_InnerRight, 0,
						(flags & VF_SPACE) ? TAG_IGNORE : MUIA_InnerTop, 0,
						(flags & VF_SPACE) ? TAG_IGNORE : MUIA_InnerBottom, 0,
						(flags & VF_SPACE) ? TAG_IGNORE : MUIA_Group_Spacing, 0,
					End;

				if (data->subgrp)
				{
					APTR vscroller = NULL, hscroller = NULL;
					
					/*
					 * Check if we need ScrollbarObjects.
					 */
					if (flags & VF_SCROLLERS)
					{
						vscroller = ScrollbarObject,
							MUIA_Prop_Horiz, FALSE,
							MUIA_Prop_UseWinBorder, MUIV_Prop_UseWinBorder_Right,
						End;

						hscroller = ScrollbarObject,
							MUIA_Prop_Horiz, TRUE,
							MUIA_Group_Horiz, TRUE,
							MUIA_Prop_UseWinBorder, MUIV_Prop_UseWinBorder_Bottom,
						End;

						if (!(vscroller && hscroller))
						{
							if (vscroller) MUI_DisposeObject(vscroller);
							if (hscroller) MUI_DisposeObject(hscroller);

							/* XXX: mark failure and dispose subgrp! how? */
							PDB(("argh, failed\n"));
						}

					}

					/*
					 * Flush deficon pool and create the actual view.
					 */

					deficonpool_flush_mime_list(data->viewid);

					if ((data->subview = NewObject(classptr, NULL,
						MA_Viewgroup_VScroller, vscroller,
						MA_Viewgroup_HScroller, hscroller,
						MA_View_IsRoot, data->isroot,
						MA_View_IsRootExtra, data->isrootExtra,
						MA_View_ModeIndex, data->pendingindex,
						MA_Viewgroup_ID, data->viewid,
						TAG_DONE))) /* XXX: MA_View_IsRoot.. is it the best way ? */
					{
						APTR grp;

						/*
						 * A VGroup will hold everything.
						 */
						grp = VGroup,
								(flags & VF_SPACE) ? TAG_IGNORE : MUIA_FillArea, FALSE,
								(flags & VF_SPACE) ? TAG_IGNORE : MUIA_InnerLeft, 0,
								(flags & VF_SPACE) ? TAG_IGNORE : MUIA_InnerRight, 0,
								(flags & VF_SPACE) ? TAG_IGNORE : MUIA_InnerTop, 0,
								(flags & VF_SPACE) ? TAG_IGNORE : MUIA_InnerBottom, 0,
								(flags & VF_SPACE) ? TAG_IGNORE : MUIA_Group_Spacing, 0,
						End;

						if (grp)
						{
							DoMethod(data->subgrp, OM_ADDMEMBER, grp);
							
							/*
							 * And the scrollgroup contains the
							 * view.
							 */

							if (flags & (VF_SCROLLWIN | VF_SCROLLGROUP))
							{
								data->scrollgrp = ScrollgroupObject,
									(flags & VF_SPACE) ? TAG_IGNORE : MUIA_FillArea, FALSE,
									(flags & VF_SPACE) ? TAG_IGNORE : MUIA_InnerLeft, 0,
									(flags & VF_SPACE) ? TAG_IGNORE : MUIA_InnerRight, 0,
									(flags & VF_SPACE) ? TAG_IGNORE : MUIA_InnerTop, 0,
									(flags & VF_SPACE) ? TAG_IGNORE : MUIA_InnerBottom, 0,
									MUIA_Scrollgroup_UseWinBorder, (flags & VF_SCROLLWIN) ? TRUE : FALSE,
									MUIA_Scrollgroup_FreeHoriz, TRUE,
									MUIA_Scrollgroup_FreeVert, TRUE,
									MUIA_Scrollgroup_Contents, data->subview,
								End;
							}
							else
							{
								data->scrollgrp = data->subview;
							}

							if (data->scrollgrp)
							{
								DoMethod(grp, OM_ADDMEMBER, data->scrollgrp);
								
								if (hscroller)
									DoMethod(grp, OM_ADDMEMBER, hscroller);
								
								if (vscroller)
									DoMethod(data->subgrp, OM_ADDMEMBER, vscroller);
								
								rc = TRUE;
							}

							/* XXX */
						}
						
						if (!rc)
						{
							MUI_DisposeObject(data->subview);
						}
					}
					
					if (!rc)
					{
						PDB(("viewgroup failed!\n"));
						MUI_DisposeObject(data->subgrp);
						data->subgrp = NULL;
						/* XXX: be more informative maybe ? */
					}
				}
			}

			#if 0
			switch (msg->type)
			{
				case MV_View_Type_Fastlist:
					{
						APTR vscroller, hscroller;

						data->subgrp = HGroup,
								InnerSpacing(0, 0),
								MUIA_Group_Spacing, 0,
								Child, vscroller = ScrollbarObject,
									MUIA_Prop_UseWinBorder, MUIV_Prop_UseWinBorder_Right,
								End,
								Child, hscroller = ScrollbarObject,
									MUIA_Prop_UseWinBorder, MUIV_Prop_UseWinBorder_Bottom,
								End,
								Child, VGroup,
									InnerSpacing(0, 0),
									GroupSpacing(0),
									Child, NewObject(getfasttitlegroupclass(), NULL, TAG_DONE),
									Child, /*grp_view =*/ NewObject(getfastlistclass(), NULL, MA_Fastlist_VScroller, vscroller, MA_Fastlist_HScroller, hscroller, TAG_DONE),
									Child, /*view_str =*/ NewObject(getfaststringclass(), NULL, TAG_DONE), /* XXX: make that an attribute I guess.. hm, part of main group also */
								End,
							End;

						#if 0
						if (data->subgrp)
						{
							DoMethod(view_str, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
								grp_view, 2, MM_Fastlist_LoadPath, MUIV_TriggerValue
							);
						}
						#endif
					}
					break;

				case MV_View_Type_Iconview:
					if (data->isroot)
					{
						data->subgrp = HGroup,
								InnerSpacing(0, 0),
								Child, /*grp_view =*/ NewObject(geticonviewclass(), NULL, MA_View_IsRoot, data->isroot, /* msg->type,*/ TAG_DONE),
							End;
					}
					else
					{
						data->subgrp = ScrollgroupObject,
								InnerSpacing(0, 0),
								MUIA_Scrollgroup_UseWinBorder, TRUE,
								MUIA_Scrollgroup_FreeHoriz, TRUE,
								MUIA_Scrollgroup_FreeVert, TRUE,
								MUIA_Scrollgroup_Contents, /*grp_view =*/ NewObject(geticonviewclass(), NULL, /*MA_Window_Type, type, MA_Iconview_ViewMode, viewmode,*/ TAG_DONE), /* XXX: need the viewmode as attribute also */
								End;
					}
					break;

				case MV_View_Type_None:
					data->subgrp = HVSpace;
					break;

				#ifdef DEBUG
				default:
					PDB(("ARGH! unknown window type\n"));
					break;
				#endif
			}
			#endif
		}

		if (data->subgrp)
		{
			ULONG index;

			DoMethod(obj, OM_ADDMEMBER, data->subgrp);

			data->currentview = data->pendingview;
			data->currentindex = data->pendingindex;


			/*
			 * That method comes before MUIM_Setup if the view
			 * is a new one. That way we can set window positions safely.
			 */

			DoMethod(data->subview, MM_View_LoadURI);
		
			/*
			 * Check if the view decided to change the
			 * index by itself.
			 */

			if (get(data->subview, MA_View_ModeIndex, &index))
			{
				/*
				 * We also setup pendingindex to prohibit any further changes.
				 */

				data->currentindex = index;
				data->pendingindex = index;

			}

			if (data->aftersetup)
			{
				DoMethod(data->subview, MM_View_Setup);
				DoMethod(_win(data->subview), MM_Window_UpdateStatusbar);
			}
		}
		/* XXX: huho.. and put data->currentview to something useful */

		/* add fastsearch as last one */

		//DoMethod( obj, OM_ADDMEMBER, data->str_search );

		DoMethod(obj, MUIM_Group_ExitChange);

		/* Trigger notify now that view change is finalized */

		set( obj, MA_Viewgroup_ViewChanged, TRUE );

	}
	else if (data->pendingindex != data->currentindex)
	{
		if (data->subview)
		{
			set(data->subview, MA_View_ModeIndex, data->pendingindex);
		}
		
		data->currentindex = data->pendingindex;

		/* Trigger notify now that view change is finalized */

		set( obj, MA_Viewgroup_ViewChanged, TRUE );

	}

	return (0);
}


/* public */
DEFSMETHOD(View_ReadArgs)
{
	GETDATA;

	if (data->mimectx && msg->templ && msg->array)
	{
		return (mimeuri_readargs(data->mimectx, msg->templ, msg->array));
	}

	return (FALSE);
}


/* public */
DEFSMETHOD(View_SetWindowPosition)
{
	GETDATA;

	/*
	 * We just ignore position requests after MUIM_Setup for now.
	 * It makes no sense to annoy the user with jumping windows.
	 * XXX: well, maybe when we implement JS or so then :) but that
	 * needs ChangeWindowBox() and after MUIM_Show.. hm, and SetWindowPosition
	 * would need a size argument as well.. I don't like this much
	 */
	if (!data->aftersetup)
	{
		data->dopos = TRUE;
		data->top = msg->top;
		data->left = msg->left;
	}
	return (0);
}


DEFSMETHOD(View_SetStatus)
{
	GETDATA;

	if (msg->flags & MF_View_SetStatus_Window) /* XXX: check if the status bar is NOT here and MF_View_SetStatus_Bar is specified as well, then it ends in window too */
	{
		snprintf_array(data->windowtitle, sizeof(data->windowtitle), msg->fmt, &msg->val);

		if (data->aftersetup)
		{
			if (muiRenderInfo(obj) && _win(obj))
			{
				DoMethod(_win(obj), MM_Window_SetTitle, data->windowtitle);
			}
		}
		else
		{
			data->dotitle = TRUE;
		}
	}
	return (0);
}


DEFSMETHOD(View_ContextMenuMerge)
{
	APTR subviewmenu = NULL; /* shut up gcc */
	APTR viewmenu = NULL; /* shut up gcc */
	STRPTR s;

	/* menustrip -> menu */
	FORCHILD(msg->from, MUIA_Family_List)
	{
		subviewmenu = child;
		break;
	}
	NEXTCHILD

	ASSERT(subviewmenu);

	/* ditto */
	FORCHILD(msg->to, MUIA_Family_List)
	{
		viewmenu = child;
		break;
	}
	NEXTCHILD

	ASSERT(viewmenu);

	DoMethod(subviewmenu, MUIM_Family_Transfer, viewmenu);

	/* transfer the title as well */
	if ((s = (STRPTR)getv(subviewmenu, MUIA_Menu_Title)))
	{
		set(viewmenu, MUIA_Menu_Title, s);
	}
	return (0);
}


DEFSMETHOD(View_Refresh)
{
	GETDATA;

	if (msg->flags & MF_View_Refresh_Background)
	{
		DoMethod(obj, MM_Viewgroup_GetBgPen);
	}

	if (data->subview)
	{
		DoMethodA(data->subview, (Msg)msg);
	}
	return (0);
}


DEFSMETHOD(View_DoMethod)
{
	GETDATA;

	if (data->subview)
	{
		return (DoMethodA(data->subview, (Msg)msg->args));
	}
	return (0);
}

DEFSMETHOD(Thread_Finished)
{
	switch (msg->action)
	{
		case TA_Devices_RemoveAll:
			do_action(obj, TA_Devices_Add, TT_Devices_Fade, FALSE, TAG_DONE);
			break;
	}
	return (0);
}


/* string functions for search mode */

ULONG match(CONST_STRPTR buf, CONST_STRPTR sub);
LONG distance(CONST_STRPTR buf, CONST_STRPTR sub);
void strlower(STRPTR s);

ULONG match(CONST_STRPTR buf, CONST_STRPTR sub)
{
	CONST_STRPTR bp;
	CONST_STRPTR sp;
	ULONG cnt = 0;

	if (!*sub)
		return FALSE;

	bp = buf;
	sp = sub;

	while(*sp && *bp)
	{
		if(*bp == *sp)
		{
			bp++;
			sp++;
			cnt++;
		}
		else
		{
			break;
		}
	}

	return cnt;
}

LONG distance(CONST_STRPTR buf, CONST_STRPTR sub)
{
	int d = 0;
	CONST_STRPTR bp;
	CONST_STRPTR sp;

	if (!*sub)
		return FALSE;

	bp = buf;
	sp = sub;

	while(*sp && *bp)
	{
		if(*bp == *sp)
		{
			bp++;
			sp++;
		}
		else
		{
			d = *buf - *sub;
			break;
		}
	}

  return d;
}

void strlower(STRPTR s)
{
	UBYTE c;

	while ((c = *s))
	{
		*s++ = tolower(c);
	}
}

enum { SEARCH_FILE, SEARCH_DIR };

DEFSMETHOD(View_Focus)
{
	GETDATA;

	LONG iselect = -1;
	LONG len;
	TEXT name[NAME_SIZE];
	TEXT partialname[NAME_SIZE];
	TEXT partialname2[NAME_SIZE];
	ULONG searchmode = SEARCH_FILE;
	STRPTR parsepat = NULL; // bitRocky: for pattern search
	ULONG patsize; // bitRocky: for pattern search

	/* if it's not partial name then we can focus immediately */

	if ( !msg->partial )
	{
		return DoMethodA( data->subview, msg );
	}

	/* otherwise we lookup closest match */

	if( msg->name == NULL || *msg->name == '\0' )
	{
		set(data->str_search, MUIA_Background, MUII_StringActiveBack); prev_iselect = 0;
		return 0;
	}

	/* if text is uppercase, then we search in dirs instead of files */

	if( isalnum( msg->name[ 0 ] ) )
	{
		if( tolower( msg->name[ 0 ] ) == msg->name[ 0 ] )
		{
			searchmode = SEARCH_FILE;
		}
		else
		{
			searchmode = SEARCH_DIR;
		}
	}

	stccpy( partialname, msg->name, sizeof(partialname) );
	strlower( partialname );

	strcpy( partialname2, partialname );
	len = strlen( partialname2 );

	if (prev_iselect < 0) prev_iselect = 0;
	//if (len != prev_len) { prev_iselect = 0; prev_len = len; }
	PDB(("iselect = %ld, prev_iselect = %ld, len = %ld, prev_len = %ld\n", iselect, prev_iselect, len, prev_len));

	//while (len > 0 && iselect == -1)
	{
		ULONG matchcount = 0;
		LONG i;

		// bitRocky: for pattern search
		patsize = len * 2 + 2;
		if ((parsepat = malloc( patsize )))
		{
			// check, if there are wildcards in search name
			if ( (ParsePattern( partialname2, parsepat, patsize ) != 1) ) // no wildcards or error
			{
				free( parsepat ); parsepat = NULL;
			}
		}
		
		for(i = prev_iselect; ; msg->searchUp ? i-- : i++)//i++)// bitRocky 30-Apr-2018
		{
			APTR entry = (APTR)DoMethod( data->subview, MM_View_GetEntry, i );
			ULONG type = type;

			if ( entry )
			{
				type = getv( entry, MA_Icon_FileType );
				if ( type == MV_Icon_FileType_Device )
				{
					type = MV_Icon_FileType_Directory;
					searchmode = SEARCH_DIR; // bitRocky: to be able to search in MyMorphOS with lower case
				}
			}

			if ( entry && ( searchmode == SEARCH_FILE ? ( type != MV_Icon_FileType_Directory ) : ( type == MV_Icon_FileType_Directory ) ) )
			{
				STRPTR filename = (STRPTR)getv( entry, MA_Icon_Path );
				if ( filename )
				{
					ULONG cnt=-1;
					STRPTR filepart = FilePart( filename );

					if ( filepart && filepart[ 0 ] )
						filename = filepart;

					stccpy( name, filename, sizeof(name) );
					strlower( name );
					
					// bitRocky: test pattern matching search, only if there were wildcards in the search name
					if ( parsepat ) 
					{
						cnt = MatchPattern( parsepat, name ) ? len : 0;
					}
					else
					{
						cnt	= match( name, partialname2 );
					}

					if ( cnt > matchcount )
					{
						matchcount = cnt;
					}

					if ( cnt == len )
					{
						iselect = i; PDB(("found, i = %ld, cnt = %ld, len = %ld, matchcount = %ld, filename = '%s', name = '%s', partialname2 = '%s'\n", i, cnt, len, matchcount, filename, name, partialname2));
						break;
					}
				}
			}

			if( !entry )
			{
				break;
			}
		}
/* // bitRocky
		if( iselect == -1 )
		{
			partialname2[ matchcount ] = 0;
			len = matchcount;
		}
*/
	}

	if((iselect == -1) && (len == 1)) // only for the first letter
	{
		LONG d, i;
		LONG mind = 0x7FFFFFFF;

		for(i = prev_iselect; ; msg->searchUp ? i-- : i++)//i++)// bitRocky 30-Apr-2018
		{
			APTR entry = (APTR)DoMethod( data->subview, MM_View_GetEntry, i );
			ULONG type = type;

			if ( entry )
				type = getv( entry, MA_Icon_FileType );

			if ( entry && ( searchmode == SEARCH_FILE ? ( type != MV_Icon_FileType_Directory ) : ( type == MV_Icon_FileType_Directory ) ) )
			{
				STRPTR filename = (STRPTR)getv( entry, MA_Icon_Path );

				if ( filename )
				{
					STRPTR filepart = FilePart( filename );

					if ( filepart && filepart[ 0 ] )
						filename = filepart;

					stccpy( name, filename, sizeof(name) );
					strlower( name );
					d = distance( name, partialname );

					if( abs( d ) < mind )
					{
						mind = abs( d );
						iselect = i; PDB(("found, i = %ld, filename = '%s', name = '%s', partialname = '%s'\n", i, filename, name, partialname));
					}
				}
			}

			if( !entry )
			{
				break;
			}
		}
	}

	if( iselect != -1 )
	{
		APTR entry = (APTR)DoMethod( data->subview, MM_View_GetEntry, iselect );
		STRPTR filename = (STRPTR)getv( entry, MA_Icon_Path );

		if ( filename )
		{
			STRPTR filepart = FilePart( filename );
			ULONG colon=FALSE;

			if ( filepart && *filepart )
				filename = filepart;

			PDB(("iselect = %ld, filename = '%s', name '%s'\n", iselect, filename, name));
			// if no entry for the first letter was found, "iselect" is set to the nearest filename, so we have to check it
			// use PushMethod with delay because if the str gadget appears for the first time, we have to set it after it gets activated!
			DoMethod( app, MUIM_Application_PushMethod, data->str_search, 3 | MUIV_PushMethod_Delay(100),
				MUIM_Set, MUIA_Background, (stricmp(filename, name)==0 || (len==1 && partialname[0]==0)) ? (STRPTR)MUII_StringActiveBack : "2:ffffffff,00000000,00000000" );
			//set(data->str_search, MUIA_Background, (stricmp(filename, name)==0) ? (STRPTR)MUII_StringActiveBack : "2:ffffffff,00000000,00000000");
			prev_iselect = ( (partialname[0] == ToLower(filename[0])) || parsepat ) ? iselect : 0 ;
			prev_len = len;

			// check if the found filename has a ":" at the end (is a device or assign), then remove it! because in MyMorphOS View, the ":" isn't shown!
			len = strlen(filename);
			if ((len > 1) && (filename[len-1] == ':')) { filename[len-1] = 0; colon = TRUE; }
			DoMethod( data->subview, MM_View_Focus, FALSE, filename, msg->searchUp );
			if (colon) filename[len-1] = ':'; // put it back!
		}
	}
	else
	{
		PDB(("prev_iselect = %ld\n", prev_iselect));
		set(data->str_search, MUIA_Background, "2:ffffffff,00000000,00000000");
		prev_iselect = 0;
	}
	if (parsepat) { free( parsepat ); }; // bitRocky: for pattern search

	return (0);
}

DEFSMETHOD(View_InvalidateMimeType)
{
	GETDATA;
	APTR entry;
	ULONG i = 0;

	do
	{
		entry = (APTR)DoMethod( data->subview , MM_View_GetEntry, i );

		if ( entry )
		{
			if ( msg->mimetype )
			{
				APTR mimetype = (APTR)getv( entry, MA_Icon_MimeType );

				if ( mimetype == msg->mimetype )
				{
					set( entry, MA_Icon_MimeType, NULL );
				}
			}
			else
			{
				set( entry, MA_Icon_MimeType, NULL );
			}
		}
		i++;
	} while ( entry );

	return (0);
}

BEGINMTABLE
DECNEW
DECDISP
DECSET
DECGET
DECTMETHOD(Viewgroup_GetBgPen)
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECMMETHOD(HandleEvent)
DECSMETHOD(Viewgroup_ChangeView)
DECTMETHOD(Viewgroup_Aborted)
DECTMETHOD(Viewgroup_ChangeView2)
DECSMETHOD(View_InvalidateMimeType)
DECSMETHOD(View_ReadArgs)
DECSMETHOD(View_SetWindowPosition)
DECSMETHOD(View_SetStatus)
DECSMETHOD(View_ContextMenuMerge)
DECSMETHOD(View_Refresh)
DECSMETHOD(View_DoMethod)
DECSMETHOD(View_Focus)
DECSMETHOD(Thread_Finished)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, viewgroupclass)
