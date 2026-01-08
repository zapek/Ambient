/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2007 Ambient Open Source Team
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
 * $Id: viewapi.c,v 1.15 2022/07/23 02:20:35 jacadcaps Exp $
 */

#include "ambient.h"

/* private */
#include "viewapi.h"
#include "mimeuri.h"
#include "hash.h"
#include "mui_func.h"
#include "ambient_cat.h"
#include "tags.h"

static struct MinList viewapi_list;


ULONG viewapi_init(void)
{
	NEWLIST(&viewapi_list);

	return (TRUE);
}


void viewapi_cleanup(void)
{
	struct viewnode *vn, *nextvn;

	ITERATELISTSAFE(vn, nextvn, &viewapi_list)
	{
		/* XXX: deleteclass or so. but not for internal ones */
		if (!vn->internal)
		{
			/* XXX: there.. */
		}
		else
		{
			/* delete querytags (cloned) */

			tags_free( vn->querytagarray );
		}

		free(vn);
	}
}

static ULONG viewapi_scan_internal(void)
{
	static ULONG internal_scanned;
	ULONG rc = TRUE;

	if (!internal_scanned)
	{
		struct viewnode *vn;
		ULONG id = 1;

		/*
		 * Add iconview for root and directory view.
		 */
		if ( (vn = malloc(sizeof(*vn) + sizeof("Icon"))) )
		{
			struct TagItem iconviewtagarray[] = {
				{AVIEW_Query_Version, 1},
				{AVIEW_Query_Revision, 0},
				{AVIEW_Query_Copyright, (ULONG)"Copyright 2003-2004 by David Gerber <zapek@morphos.net>, Copyright 2006 by Ambient Open Source Team, All Rights Reserved"},
				{AVIEW_Query_Info, (ULONG)"Displays icons, newicons, glowicons, pngicons and picture thumbnails"},
				{AVIEW_Query_MimeType, (ULONG)MIMETYPE_INTERNAL_ROOTVIEW},
				{AVIEW_Query_MimeType, (ULONG)MIMETYPE_INTERNAL_DIRECTORY},
				{AVIEW_Query_MimeType, (ULONG)MIMETYPE_INTERNAL_VFS},
				{AVIEW_Query_MimeExtension, (ULONG)"*"},
				{AVIEW_Query_Viewmode_Name, (ULONG)GSI(MSG_VIEW_ICONS)},
				{AVIEW_Query_Viewmode_RexxName, (ULONG)"ICONS"},
				{AVIEW_Query_Viewmode_Name, (ULONG)GSI(MSG_VIEW_ALLFILES)},
				{AVIEW_Query_Viewmode_RexxName, (ULONG)"ALL"},
				#if USE_THUMBS
				{AVIEW_Query_Viewmode_Name, (ULONG)GSI(MSG_VIEW_THUMB)},
				{AVIEW_Query_Viewmode_RexxName, (ULONG)"THUMBS"},
				#endif
				{AVIEW_Query_Flags, VF_SCROLLWIN | VF_STRING | VF_TOOLBAR | VF_SEARCHABLE | VF_FILEVIEW},
				{TAG_DONE, 0}
			};
			strcpy(vn->name, "Icon");
			vn->libbase = NULL; /* prevents its usage */
			vn->cl = geticonviewclass();
			vn->querytagarray = tags_clone( iconviewtagarray );
			vn->internal = TRUE;
			vn->namehash = hash_nocase(vn->name);
			vn->label = GSI(MSG_VIEW_ICON);
			vn->id = id++;

			/* XXX: we need to GetClass or so.. and a pointer for that. no createclass needed */

			ADDTAIL(&viewapi_list, vn);
		}
		else
		{
			rc = FALSE;
		}

		/*
		 * Add iconview for devices view.
		 */
		if ( (vn = malloc(sizeof(*vn) + sizeof("Icons"))) )
		{
			struct TagItem iconviewtagarray[] = {
				{AVIEW_Query_Version, 1},
				{AVIEW_Query_Revision, 0},
				{AVIEW_Query_Copyright, (ULONG)"Copyright 2003-2004 by David Gerber <zapek@morphos.net>, Copyright 2006 by Ambient Open Source Team, All Rights Reserved"},
				{AVIEW_Query_Info, (ULONG)"Displays icons, newicons, glowicons, pngicons and picture thumbnails"},
				{AVIEW_Query_MimeType, (ULONG)MIMETYPE_INTERNAL_VOLUMES},
				{AVIEW_Query_MimeExtension, (ULONG)"*"},
				{AVIEW_Query_Viewmode_Name, (ULONG)GSI(MSG_VIEW_ICONS)},
				{AVIEW_Query_Viewmode_RexxName, (ULONG)"ICONS"},
				{AVIEW_Query_Flags, VF_SCROLLWIN | VF_STRING | VF_TOOLBAR | VF_SEARCHABLE | VF_DEVICEVIEW },
				{TAG_DONE, 0}
			};
			strcpy(vn->name, "Icons");
			vn->libbase = NULL; /* prevents its usage */
			vn->cl = geticonviewclass();
			vn->querytagarray = tags_clone( iconviewtagarray );
			vn->internal = TRUE;
			vn->namehash = hash_nocase(vn->name);
			vn->label = GSI(MSG_VIEW_ICONS);
			vn->id = id++;

			/* XXX: we need to GetClass or so.. and a pointer for that. no createclass needed */

			ADDTAIL(&viewapi_list, vn);
		}
		else
		{
			rc = FALSE;
		}

		/*
		 * Add listview.
		 */
		if (rc && (vn = malloc(sizeof(*vn) + sizeof("List"))))
		{
			struct TagItem listviewtagarray[] = {
				{AVIEW_Query_Version, 1},
				{AVIEW_Query_Revision, 0},
				{AVIEW_Query_Copyright, (ULONG)"Copyright 2004 by David Gerber <zapek@morphos.net>, Copyright 2006 by Ambient Open Source Team, All Rights Reserved"},
				{AVIEW_Query_Info, (ULONG)"Displays files and directories"},
				{AVIEW_Query_MimeType, (ULONG)MIMETYPE_INTERNAL_DIRECTORY},
				//{AVIEW_Query_MimeType, (ULONG)MIMETYPE_INTERNAL_VOLUMES},
				{AVIEW_Query_MimeType, (ULONG)MIMETYPE_INTERNAL_VFS},
				{AVIEW_Query_MimeExtension, (ULONG)"*"},
				{AVIEW_Query_Viewmode_Name, (ULONG)GSI(MSG_VIEW_ICONS)},
				{AVIEW_Query_Viewmode_RexxName, (ULONG)"ICONS"},
				{AVIEW_Query_Viewmode_Name, (ULONG)GSI(MSG_VIEW_ALLFILES)},
				{AVIEW_Query_Viewmode_RexxName, (ULONG)"ALL"},
				{AVIEW_Query_Viewmode_Name, (ULONG)GSI(MSG_VIEW_THUMB)},
				{AVIEW_Query_Viewmode_RexxName, (ULONG)"THUMBS"},

				{AVIEW_Query_Flags, VF_STRING | VF_TOOLBAR | VF_SEARCHABLE | VF_FILEVIEW },
				{TAG_DONE, 0}
			};
			strcpy(vn->name, "List");
			vn->libbase = NULL; /* prevents its usage */
			vn->cl = getlistviewclass();
			vn->querytagarray = tags_clone( listviewtagarray );
			vn->internal = TRUE;
			vn->namehash = hash_nocase(vn->name);
			vn->label = GSI(MSG_VIEW_LIST);
			vn->id = id++;

			/* XXX: ditto.. */

			ADDTAIL(&viewapi_list, vn);
		}
		else
		{
			rc = FALSE;
		}

		/*
		 * Add listview for devices.
		 */
		if (rc && (vn = malloc(sizeof(*vn) + sizeof("DList"))))
		{
			struct TagItem listviewtagarray[] = {
				{AVIEW_Query_Version, 1},
				{AVIEW_Query_Revision, 0},
				{AVIEW_Query_Copyright, (ULONG)"Copyright 2004 by David Gerber <zapek@morphos.net>, Copyright 2006 by Ambient Open Source Team, All Rights Reserved"},
				{AVIEW_Query_Info, (ULONG)"Displays volumes"},
				{AVIEW_Query_MimeType, (ULONG)MIMETYPE_INTERNAL_VOLUMES},
				{AVIEW_Query_MimeExtension, (ULONG)"*"},
				{AVIEW_Query_Viewmode_Name, (ULONG)GSI(MSG_VIEW_ICONS)},
				{AVIEW_Query_Viewmode_RexxName, (ULONG)"ICONS"},
				{AVIEW_Query_Flags, VF_STRING | VF_TOOLBAR | VF_SEARCHABLE | VF_DEVICEVIEW },
				{TAG_DONE, 0}
			};
			strcpy(vn->name, "DList");
			vn->libbase = NULL; /* prevents its usage */
			vn->cl = getlistviewclass();
			vn->querytagarray = tags_clone( listviewtagarray );
			vn->internal = TRUE;
			vn->namehash = hash_nocase(vn->name);
			vn->label = GSI(MSG_VIEW_LIST);
			vn->id = id++;

			/* XXX: ditto.. */

			ADDTAIL(&viewapi_list, vn);
		}
		else
		{
			rc = FALSE;
		}

		#if USE_VIEW_IMAGE
		/*
		 * Add imageview.
		 */
		if ( (vn = malloc(sizeof(*vn) + sizeof("image"))) )
		{
			struct TagItem imageviewtagarray[] = {
				{AVIEW_Query_Version, 1},
				{AVIEW_Query_Revision, 0},
				{AVIEW_Query_Copyright, (ULONG)"Copyright 2004 by David Gerber <zapek@morphos.net>, Copyright 2006 by Ambient Open Source Team, All Rights Reserved"},
				{AVIEW_Query_Info, (ULONG)"Displays pictures using datatypes"},
				{AVIEW_Query_MimeType, (ULONG)"image/*"},
				{AVIEW_Query_MimeExtension, (ULONG)"*"},
				{AVIEW_Query_Viewmode_Name, (ULONG)GSI(MSG_VIEW_SCALED)},
				{AVIEW_Query_Viewmode_RexxName, (ULONG)"SCALED"},
				{AVIEW_Query_Viewmode_Name, (ULONG)GSI(MSG_VIEW_NORMAL)},
				{AVIEW_Query_Viewmode_RexxName, (ULONG)"NORMAL"},
				{AVIEW_Query_Flags, VF_SCROLLWIN | VF_STRING},
				{TAG_DONE, 0}
			};
			strcpy(vn->name, "Image");
			vn->libbase = NULL; /* prevents its usage */
			vn->cl = getimageviewclass();
			vn->querytagarray = tags_clone( imageviewtagarray );
			vn->internal = TRUE;
			vn->namehash = hash_nocase(vn->name);
			vn->label = "Image"; /* no need to localize for now */
			vn->id = id++;

			/* XXX: ditto.. */

			ADDTAIL(&viewapi_list, vn);
		}
		else
		{
			rc = FALSE;
		}
		#endif

		/*
		 * Add Boopsiview.
		 */
		if ( (vn = malloc(sizeof(*vn) + sizeof("boopsi"))) )
		{
			struct TagItem boopsiviewtagarray[] = {
				{AVIEW_Query_Version, 1},
				{AVIEW_Query_Revision, 0},
				{AVIEW_Query_Copyright, (ULONG)"Copyright 2004 by David Gerber <zapek@morphos.net>, Copyright 2006 by Ambient Open Source Team, All Rights Reserved"},
				{AVIEW_Query_Info, (ULONG)"Displays boopsi objects"},
				{AVIEW_Query_MimeType, (ULONG)"boopsi/*"}, /* XXX */
				{AVIEW_Query_MimeExtension, (ULONG)"*"},
				{AVIEW_Query_Flags, VF_SCROLLWIN | VF_STRING},
				{TAG_DONE, 0}
			};
			strcpy(vn->name, "Boopsi");
			vn->libbase = NULL; /* prevents its usage */
			vn->cl = getboopsiviewclass();
			vn->querytagarray = tags_clone( boopsiviewtagarray );
			vn->internal = TRUE;
			vn->namehash = hash_nocase(vn->name);
			vn->label = "Boopsi"; /* no need to localize for now */
			vn->id = id++;

			/* XXX: ditto.. */

			ADDTAIL(&viewapi_list, vn);
		}
		else
		{
			rc = FALSE;
		}


		#if USE_VIEW_TEXT
		/*
		 * Add textview.
		 */
		if ( (vn = malloc(sizeof(*vn) + sizeof("text"))) )
		{
			struct TagItem textviewtagarray[] = {
				{AVIEW_Query_Version, 1},
				{AVIEW_Query_Revision, 0},
				{AVIEW_Query_Copyright, (ULONG)"Copyright 2004 by David Gerber <zapek@morphos.net>, Copyright 2006 by Ambient Open Source Team, All Rights Reserved"},
				{AVIEW_Query_Info, (ULONG)"Displays text"},
				{AVIEW_Query_MimeType, (ULONG)"text/*"},
				{AVIEW_Query_MimeExtension, (ULONG)"*"},
				//{AVIEW_Query_Flags, VF_SCROLLWIN | VF_STRING | VF_TOOLBAR},
				{AVIEW_Query_Flags, VF_SCROLLERS | VF_STRING},
				{TAG_DONE, 0}
			};
			strcpy(vn->name, "Text");
			vn->libbase = NULL; /* prevents its usage */
			vn->cl = gettextviewclass();
			vn->querytagarray = tags_clone( textviewtagarray );
			vn->internal = TRUE;
			vn->namehash = hash_nocase(vn->name);
			vn->label = "Text"; /* no need to localize for now */
			vn->id = id++;

			/* XXX: ditto.. */

			ADDTAIL(&viewapi_list, vn);
		}
		else
		{
			rc = FALSE;
		}
		#endif

		#if USE_VIEW_HEX
		/*
		 * Add textview.
		 */
		if ( (vn = malloc(sizeof(*vn) + sizeof("internal/x-morphos-hex-file"))) )
		{
			struct TagItem hexviewtagarray[] = {
				{AVIEW_Query_Version, 1},
				{AVIEW_Query_Revision, 0},
				{AVIEW_Query_Copyright, (ULONG)"Copyright 2004 by David Gerber <zapek@morphos.net>, Copyright 2006 by Ambient Open Source Team, All Rights Reserved"},
				{AVIEW_Query_Info, (ULONG)"Displays binary data as hex"},
				{AVIEW_Query_MimeType, (ULONG)"internal/x-morphos-hex-file"},
				{AVIEW_Query_MimeExtension, (ULONG)"*"},
				//{AVIEW_Query_Flags, VF_SCROLLWIN | VF_STRING | VF_TOOLBAR},
				{AVIEW_Query_Flags, VF_SCROLLERS | VF_STRING},
				{TAG_DONE, 0}
			};
			strcpy(vn->name, "Hex");
			vn->libbase = NULL; /* prevents its usage */
			vn->cl = gethexviewclass();
			vn->querytagarray = tags_clone( hexviewtagarray );
			vn->internal = TRUE;
			vn->namehash = hash_nocase(vn->name);
			vn->label = "Hex"; /* no need to localize for now */
			vn->id = id++;

			/* XXX: ditto.. */

			ADDTAIL(&viewapi_list, vn);
		}
		else
		{
			rc = FALSE;
		}
		#endif

		internal_scanned = TRUE;
	}
	return (rc);
}


/*
 * Scans the disk for views. Can be called to
 * refresh the list as well. No multithreading
 * support. The list can only be checked from
 * the main thread.
 */
ULONG viewapi_scan(void)
{
	if (viewapi_scan_internal())
	{
		/* XXX: needs a real file scanning then */
		return (TRUE);
	}
	return (FALSE);
}


struct viewnode * viewapi_findbyclass(struct IClass *cl)
{
	struct viewnode *vn;

	ITERATELIST(vn, &viewapi_list)
	{
		if (cl == vn->cl)
		{
			return (vn);
		}
	}
	return (NULL);
}


struct viewnode * viewapi_findbyname(CONST_STRPTR name)
{
	struct viewnode *vn;
	ULONG h = hash_nocase(name);

	ITERATELIST(vn, &viewapi_list)
	{
		if (vn->namehash == h)
		{
			if (!stricmp(name, vn->name))
			{
				return (vn);
			}
		}
	}
	return (NULL);
}


struct viewnode * viewapi_findbyid(ULONG id)
{
	struct viewnode *vn;

	ITERATELIST(vn, &viewapi_list)
	{
		if (vn->id == id)
		{
			return (vn);
		}
	}
	return (NULL);
}


static struct viewnode * findbymime(struct viewnode *ivn, ULONG viewid, CONST_STRPTR mimename)
{
	ULONG len = 0;
	ULONG i = 0;
	ULONG partial = FALSE;
	CONST_STRPTR p = mimename;
	struct viewnode *vn;

	while (*p)
	{
		if (*p == '/')
		{
			if (*(p + 1) == '*')
			{
				partial = TRUE;
			}
			len = i;
			break;
		}
		p++;
		i++;
	}

	for (vn = (ivn ? ivn : FIRSTNODE(&viewapi_list)); NEXTNODE(vn); vn = NEXTNODE(vn))
	{
		if (!viewid || (viewid == vn->id))
		{
			FORTAG(vn->querytagarray)
			{
				case AVIEW_Query_MimeType:
					{
						/* exact match */
						if (!strcmp(mimename, (STRPTR)tag->ti_Data))
						{
							//DB(("Got a match for view <%s> and type <%s>\n", vn->name, tag->ti_Data));
							return (vn);
						}

						/* partial match */
						if (partial && !strncmp(mimename, (STRPTR)tag->ti_Data, len))
						{
							return (vn);
						}
					}
					break;
			}
			NEXTTAG
		}
	}
	return (NULL);
}


struct viewnode * viewapi_findbymime(CONST_STRPTR mimename)
{
	return (findbymime(NULL, 0, mimename));
}


ULONG viewapi_checkmime(ULONG viewid, CONST_STRPTR mimename)
{
	if (viewid)
	{
		if (findbymime(NULL, viewid, mimename))
		{
			return (TRUE);
		}
	}
	return (FALSE);
}


struct viewnode * viewapi_nextviewmime(struct viewnode *vn, CONST_STRPTR mimename)
{
	if (vn)
	{
		vn = NEXTNODE(vn);
	}
	return (findbymime(vn, 0, mimename));
}


ULONG viewapi_getmodeindex(struct viewnode *vn, CONST_STRPTR modename)
{
	ULONG i = 0;

	FORTAG(vn->querytagarray)
	{
		case AVIEW_Query_Viewmode_RexxName:
			if (!stricmp(modename, (STRPTR)tag->ti_Data))
			{
				return (i); /* found! */
			}
			i++;
			break;
	}
	NEXTTAG

	return (0); /* 0 is default */
}

STRPTR viewapi_getmodename( struct viewnode *vn, ULONG modeindex )
{
	ULONG i = 0;

	FORTAG(vn->querytagarray)
	{
		case AVIEW_Query_Viewmode_RexxName:
			if ( i == modeindex )
			{
				return (STRPTR)tag->ti_Data; /* found! */
			}
			i++;
			break;
	}
	NEXTTAG

	return ""; /* Maybe we should return NULL? */
}

ULONG viewapi_getflags( struct viewnode *vn )
{
	FORTAG(vn->querytagarray)
	{
		case AVIEW_Query_Flags:
			return tag->ti_Data; /* found! */
			break;
	}
	NEXTTAG

	return (VF_SCROLLWIN | VF_STRING | VF_TOOLBAR); /* Return default */
}

ULONG viewapi_compare_idx2name(ULONG viewindex, CONST_STRPTR name)
{
	struct viewnode *vn;
	ULONG rc = FALSE;

	vn = viewapi_findbyid(viewindex);

	if (vn)
	{
		rc = !stricmp(vn->name, name);
	}

	return rc;
}
