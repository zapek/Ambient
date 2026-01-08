/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
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
 * $Id: mimelisttreeclass.c,v 1.10 2017/08/21 06:17:44 cyfm Exp $
 */

#include "ambient.h"

/* public */
#include <mui/Listtree_mcc.h>

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "mimetype.h"
#include "name.h"
#include "mimegroupclass.h"

struct Data {
	APTR activetype;
	ULONG markdefined;
	ULONG showinternal;
};

#define _treenode(_o) ((struct MUIS_Listtree_TreeNode *)(_o))

/*  basically same as struct treedata (mimegroupclass.h), just stripped down
 *  because we put it on the stack
 */
struct treenode_temp {
	ULONG  flags;
	STRPTR longname;
	LONG   priority;
};

static const CONST_STRPTR mimenames[] = {
	"application",
	"audio",
	"image",
	"message",
	"model",
	"multipart",
	"text",
	"video",
	"internal"
};

static void doset(APTR obj UNUSED, struct Data *data, struct TagItem *tags)
{
	FORTAG(tags)
	{
		case MA_Mimelisttree_ActiveType:
			data->activetype = (APTR)tag->ti_Data;
			break;
	}
	NEXTTAG
}

MUI_HOOK(mimelisttree_constructfunc, APTR pool, struct treenode_temp *tempnode)
{
	struct treedata * td;
	if ((td = AllocPooled(pool, sizeof(struct treedata))))
	{
		td->flags    = tempnode->flags;
		td->priority = tempnode->priority;
		td->longname = NULL;

		if (tempnode->longname)
		{
			td->longname = malloc(strlen(tempnode->longname)+1);
			if (td->longname)
			{
				strcpy(td->longname, tempnode->longname);
			}
			/* XXX: handle failure case ? */
		}
	}

	return (ULONG) (td);
}

MUI_HOOK(mimelisttree_destructfunc, APTR pool, struct treedata * td)
{
	if (td->longname)
	{
		free(td->longname);
	}

	FreePooled(pool, td, sizeof(struct treedata));
	return (0);
}

MUI_HOOK(mimelisttree_displayfunc, STRPTR *array, struct MUIS_Listtree_TreeNode * tn )
{
	if (tn)
	{
		struct treedata * td = ((struct treedata *)tn->tn_User);
		STRPTR bold = ( td->flags & NODEFLAG_OVERLOADED) ? "\033b" : "";

		snprintf(td->buffer_name, 256, "%s%s", bold, tn->tn_Name);
		array[ 0 ] = td->buffer_name;
		array[ 1 ] = (!td->longname) ? (STRPTR)"" : td->longname;  /* XXX: if NULL is save here then this could be simplified. */

		#ifdef DEBUG
		snprintf(td->buffer_pri, 32, "%ld", td->priority);
		array[ 2 ] = (td->flags & NODEFLAG_MIMETYPE) ? (STRPTR)td->buffer_pri : (STRPTR)"";
		#endif

		if( (ULONG)array[ -1 ] % 2 )
		{
			array[ -9 ] = (STRPTR) 10;
		}
	}
	else
	{
		array[ 0 ] = GSI(MSG_MIMELISTTREECLASS_TYPE);
		array[ 1 ] = GSI(MSG_MIMELISTTREECLASS_NAME);

		#ifdef DEBUG
		array[ 2 ] = "Priority";
		#endif
	}

	return (0);
}

DEFNEW
{
	obj = DoSuperNew(cl, obj,
		InputListFrame,
		MUIA_Listtree_DoubleClick,   MUIV_Listtree_DoubleClick_Off,
		MUIA_Listtree_DisplayHook,   &mimelisttree_displayfunc_hook,
		MUIA_Listtree_ConstructHook, &mimelisttree_constructfunc_hook,
		MUIA_Listtree_DestructHook,  &mimelisttree_destructfunc_hook,
		#ifdef DEBUG
		MUIA_Listtree_Format,        "BAR,BAR,P=\033r",
		#else
		MUIA_Listtree_Format,        "BAR,",
		#endif
		MUIA_Listtree_Title,         TRUE, /* XXX: has no effect -> report to stuntzi (MUI4 bug) */
		MUIA_List_Title,             TRUE, /*      this worksaround the above problem ;)         */
	End;

	if ( obj != NULL )
	{
		struct Data *data;

		data = INST_DATA(cl, obj);

		data->markdefined = GetTagData( MA_Mimelisttree_MarkDefined, TRUE, INITTAGS );
		data->showinternal = GetTagData( MA_Mimelisttree_ShowInternal, TRUE, INITTAGS );

		DoMethod(obj, MM_Mimelisttree_Refresh);

		DoMethod(obj, MUIM_Notify, MUIA_Listtree_Active, MUIV_EveryTime,
			obj, 2, MM_Mimelisttree_ChangeEntry, MUIV_TriggerValue
		);
	}

	return ((ULONG)obj);
}


DEFGET
{
	switch (msg->opg_AttrID)
	{
		case MA_Mimelisttree_ActiveType:
		{
			GETDATA;
			*msg->opg_Storage = (ULONG)data->activetype;
			return (TRUE);
		}
	}
	return (DOSUPER);
}


DEFSET
{
	GETDATA;

	doset(obj, data, INITTAGS);

	return (DOSUPER);
}


DEFSMETHOD(Mimelisttree_ChangeEntry)
{
	switch (msg->entry)
	{
		case MUIV_Listtree_Active_Off:
			set(obj, MA_Mimelisttree_ActiveType, MV_Mimelisttree_ActiveType_None);
			break;

		default:
		{
			struct treedata * td = ((struct treedata *) _treenode(msg->entry)->tn_User);

			if (td->flags & NODEFLAG_MEDIATYPE)
			{
				if(td->flags & NODEFLAG_OVERLOADED)
				{
					set(obj, MA_Mimelisttree_ActiveType, MV_Mimelisttree_ActiveType_MediaOverloaded);
				}
				else
				{
					set(obj, MA_Mimelisttree_ActiveType, MV_Mimelisttree_ActiveType_Media);
				}
			}
			else if(td->flags & NODEFLAG_MIMETYPE)
			{
				if(td->flags & NODEFLAG_OVERLOADED)
				{
					set(obj, MA_Mimelisttree_ActiveType, MV_Mimelisttree_ActiveType_MimeOverloaded);
				}
				else
				{
					set(obj, MA_Mimelisttree_ActiveType, MV_Mimelisttree_ActiveType_Mime);
				}
			}
			else
			{
				set(obj, MA_Mimelisttree_ActiveType, MV_Mimelisttree_ActiveType_None);
			}
			break;
		}
	}
	return (0);
}

DEFTMETHOD(Mimelisttree_Refresh)
{
	GETDATA;
	ULONG i;
    struct MUIS_Listtree_TreeNode * tn;
	TEXT mimetype[256]; // enough (tm)
	STRPTR * mimetypes;
	STRPTR * ptr;

	/*
	 * Clear first
	 */

	set(obj, MUIA_Listtree_Quiet, TRUE);

	DoMethod(obj, MUIM_Listtree_Remove, MUIV_Listtree_Remove_ListNode_Root, MUIV_Listtree_Remove_TreeNode_All, 0);

	/*
	 * Fill in mime mediatypes.
	 */
	for (i = 0; i < sizeof(mimenames) / sizeof(mimenames[0]); i++)
	{
		struct internal_mimetype_node * imn;
		ULONG overloaded = 0;
		struct treenode_temp tempnode;

		stccpy(mimetype, mimenames[i], strlen(mimenames[i])+1);
		strncat(mimetype, "/*", sizeof(mimetype));


		imn = (struct internal_mimetype_node *) mimetype_find_generic_by_mimetype(mimetype);

		if(imn && imn->descriptor != NULL && data->markdefined)
		{
			overloaded = NODEFLAG_OVERLOADED;
		}

		tempnode.flags    = NODEFLAG_MEDIATYPE | overloaded;
		tempnode.longname = NULL;

		if (data->showinternal == TRUE || strcmp(mimenames[ i ], "internal"))
		{
			DoMethod(obj, MUIM_Listtree_Insert,
				mimenames[i],
				&tempnode,
				MUIV_Listtree_Insert_ListNode_Root,
				MUIV_Listtree_Insert_PrevNode_Tail,
				TNF_LIST
			);
		}
	}

	/*
	 * Fill in mime mimetypes.
	 */

	mimetypes = mimetype_gettypesarray( FALSE, TRUE );
	ptr = mimetypes;

	while(ptr && *ptr)
	{
		STRPTR tmp;
		ULONG overloaded = 0;
		struct treenode_temp tempnode;

		/* first we get mime family and check if it's already inserted */

		struct internal_mimetype_node * imn = (struct internal_mimetype_node *) mimetype_find_by_mimetype(*ptr);
		if(imn && imn->descriptor != NULL && data->markdefined)
		{
			overloaded = NODEFLAG_OVERLOADED;
		}

		stccpy(mimetype, *ptr, sizeof(mimetype));
		tmp = strchr(mimetype, '/');

		if(tmp && *(tmp+1))
		{
			*tmp = '\0';

			/* find family */
			tn = _treenode( DoMethod(obj, MUIM_Listtree_FindName,
				MUIV_Listtree_FindName_ListNode_Root,
				mimetype, MUIV_Listtree_FindName_Flags_SameLevel
			));

			/* mimetype inserted in its family (it has to exist. we don't allow nonstandard families) */

			if(tn != NULL)
			{
				tempnode.flags    = NODEFLAG_MIMETYPE | overloaded;
				tempnode.longname = imn->description;
				tempnode.priority = imn->priority;

				DoMethod(obj, MUIM_Listtree_Insert,
					tmp+1,
					&tempnode,
					tn,
					MUIV_Listtree_Insert_PrevNode_Tail,
					0
				);
			}
		}

		ptr++;
	}

	mimetype_freetypesarray(mimetypes);

	set(obj, MUIA_Listtree_Quiet, FALSE);

	return (0);
}

/*
 * Iterate through tree. dosearch informs if we should start matching entries. Used when starting search
 * from nofe different than root. sucky? oh well:)
 */

static APTR tree_iteratefind(Object *lt, struct MUIS_Listtree_TreeNode *list, STRPTR pattern, APTR startnode, ULONG *dosearch)
{
	struct MUIS_Listtree_TreeNode *tn;
	UWORD pos=0;

	for(pos=0; ; pos++)
	{
		tn = (struct MUIS_Listtree_TreeNode *)DoMethod(lt, MUIM_Listtree_GetEntry, list, pos, MUIV_Listtree_GetEntry_Flags_SameLevel);

		if ( tn != NULL )
		{
			if ( tn->tn_Flags & TNF_LIST )
			{
				APTR ftn = tree_iteratefind(lt, tn, pattern, startnode, dosearch);

				if ( ftn != NULL )
					return ftn;
			}
			else
			{
				if ( *dosearch )
				{
					struct treedata *td = ((struct treedata *)tn->tn_User);
					if ( name_match( tn->tn_Name, pattern ) || ( td != NULL && td->longname != NULL && name_match( td->longname, pattern ) ) )
						return tn;
				}
			}

			if ( tn == startnode )
				*dosearch = TRUE;

		}
		else
		{
			break;
		}
	}

	return NULL;
}

static APTR tree_find(Object *lt, STRPTR pattern, APTR startnode)
{
	ULONG dofind = startnode != NULL ? FALSE : TRUE;
	return tree_iteratefind(lt, MUIV_Listtree_GetEntry_ListNode_Root, pattern, startnode, &dofind);
}

DEFSMETHOD(Search)
{
	struct MUIS_Listtree_TreeNode *tn = NULL;
	STRPTR pattern;

	if ( msg->string == NULL || *msg->string == 0 )
	{
		return FALSE;
	}

	/*
	 * Search for pattern in mimetype name and description.
	 */

	pattern = malloc( strlen( msg->string ) + 5 );
	if ( pattern != NULL )
	{
		/*
		 * When searching for next one, fetch selected one first.
		 */

		if ( msg->direction == MV_Search_SearchNext )
		{
			tn = (struct MUIS_Listtree_TreeNode *)DoMethod(obj, MUIM_Listtree_GetEntry, MUIV_Listtree_GetEntry_ListNode_Active, MUIV_Listtree_GetEntry_Position_Active, 0);
		}

		sprintf( pattern, "#?%s#?", msg->string );

		tn = _treenode( tree_find( obj, pattern, tn ) );

		/*
		 * Open node if needed and select.
		 */

		if ( tn != NULL )
		{
			DoMethod( obj, MUIM_Listtree_Open, MUIV_Listtree_Open_ListNode_Parent, tn, 0 );
			set( obj, MUIA_Listtree_Active, tn );
		}

		free( pattern );
	}

	return tn != NULL ? TRUE : FALSE;
}

BEGINMTABLE
DECNEW
DECGET
DECSET
DECSMETHOD(Mimelisttree_ChangeEntry)
DECTMETHOD(Mimelisttree_Refresh)
DECSMETHOD(Search)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Listtree, mimelisttreeclass)
