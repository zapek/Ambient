/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2006 Ambient Open Source Team
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
 * $Id: prefs_advanced_listclass.c,v 1.2 2006/12/20 13:34:15 fab Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "prefs_advanced.h"

#define SUPPORT_VALUE_SORTING 1

#define BUF_TITLE_SIZE  (64)
#define BUF_VALUE_SIZE  (32)
#define BUF_NAME_SIZE   (PA_MAXVARIABLENAMELENGTH + 16)  /* name + escaping */


#define intcmp(a,b) ({                          \
	typeof(a) _a = (a);                         \
	typeof(b) _b = (b);                         \
	((_a > _b) ? (1) : ((_a < _b) ? (-1) : 0));})


struct Data {
	ULONG sort_column;
	LONG  sort_direction;

	/*   some temp buffers used in List_Display
	 */
	TEXT buf_title[BUF_TITLE_SIZE]; /* localized columntitle + sort arrow  */
	TEXT buf_value[BUF_VALUE_SIZE]; /* used for numeric values only        */
	TEXT buf_dflt [BUF_VALUE_SIZE];
	TEXT buf_name [BUF_NAME_SIZE];  /* used for variable name + escaping   */
};


DEFNEW
{
	obj = DoSuperNew(cl, obj,
		InputListFrame,
		MUIA_List_Format, "BAR,BAR H,BAR H,BAR,BAR,H",
		MUIA_List_Title,  TRUE,
		TAG_MORE, INITTAGS,
	End;

	if (obj)
	{
		GETDATA;

		data->sort_column    = 0;
		data->sort_direction = 1;

		/*  XXX: move to refresh method
		 */
		{
			struct MinList *pas;
			struct pa_node *t;
		
			pas = prefs_advanced_lockstorage();

			ASSERT(pas);

			ITERATELIST(t, (struct List *)pas)
			{
				DoMethod(obj, MUIM_List_InsertSingle, t, MUIV_List_Insert_Sorted);			
			}

			prefs_advanced_unlockstorage();
		}
	}

	return ((ULONG)obj);
}


DEFGET
{
	return (DOSUPER);
}


DEFSET
{
	GETDATA;

	FORTAG(INITTAGS)
	{
		case MUIA_List_TitleClick:
		{
			ULONG old_column = data->sort_column;
			ULONG new_column = tag->ti_Data;

#if SUPPORT_VALUE_SORTING
			if (new_column < 6)
#else
			if (new_column < 4) /* we don't support value sorting */
#endif
			{
				if (old_column == new_column)
				{
					data->sort_direction = -1 * data->sort_direction;
				}

				data->sort_column = new_column;

				DoMethod(obj, MUIM_List_Sort);
			}
		}
		break;
	}
	NEXTTAG

	return (DOSUPER);
}


DEFMMETHOD(List_Construct)
{
	return (ULONG)msg->entry;  /* just reference, the pa_storage list has fixed size */
}


DEFMMETHOD(List_Destruct)
{
	return (0);
}


DEFMMETHOD(List_Display)
{
	GETDATA;

	STRPTR buf_value = data->buf_value;
	STRPTR buf_dflt  = data->buf_dflt;
	STRPTR buf_name  = data->buf_name;
	STRPTR buf_title = data->buf_title;

	if (!msg->entry)
	{
		ULONG sc = data->sort_column;

		msg->array[0] = GSI(MSG_PREFSADVANCEDLISTCLASS_VARIABLE    );
		msg->array[1] = GSI(MSG_PREFSADVANCEDLISTCLASS_STATUS      );
		msg->array[2] = GSI(MSG_PREFSADVANCEDLISTCLASS_INIT        );
		msg->array[3] = GSI(MSG_PREFSADVANCEDLISTCLASS_TYPE        );
		msg->array[4] = GSI(MSG_PREFSADVANCEDLISTCLASS_VALUE       );
		msg->array[5] = GSI(MSG_PREFSADVANCEDLISTCLASS_DEFAULTVALUE);

		/*  add little arrow to currently sorted column
		 */
		snprintf(buf_title, BUF_TITLE_SIZE, "%s \033I[6:%s]", msg->array[sc], (data->sort_direction > 0) ? "38" : "39" );
		msg->array[sc] = buf_title;
	}
	else
	{
		struct pa_node *t  = msg->entry;

		STRPTR type        = NULL;
		STRPTR value       = buf_value;
		STRPTR defvalue    = buf_dflt;

		BOOL   userdefined = t->n_flags & PAF_USERDEFINED; 


		switch(t->n_type)
		{
			case PA_STRING:
				{
					static STRPTR null_string = "\033i<NULL>\033n";

					defvalue = t->n_default ? (STRPTR)t->n_default : null_string;
					value    = t->n_value   ? (STRPTR)t->n_value   : null_string;
					type     = "STRING";
				}
				break;

			case PA_BOOL:
				{
					snprintf(buf_value, BUF_VALUE_SIZE, "%s", t->n_value   ? "true" : "false");
					snprintf(buf_dflt,  BUF_VALUE_SIZE, "%s", t->n_default ? "true" : "false");
					type = "BOOL";
				}
				break;

			case PA_INT:
				{
					snprintf(buf_value, BUF_VALUE_SIZE, "%ld", t->n_value);
					snprintf(buf_dflt,  BUF_VALUE_SIZE, "%ld", t->n_default);
					type = "INT";
				}
				break;

			case PA_UINT:
				{
					snprintf(buf_value, BUF_VALUE_SIZE, "%lu", t->n_value);
					snprintf(buf_dflt,  BUF_VALUE_SIZE, "%lu", t->n_default);
					type = "UINT";
				}
				break;

			case PA_COLOUR:
				{
					snprintf(buf_value, BUF_VALUE_SIZE, "0x%08lx", t->n_value);
					snprintf(buf_dflt,  BUF_VALUE_SIZE, "0x%08lx", t->n_default);
					type = "COLOUR";
				}
				break;
		}

		if (userdefined)
		{
			snprintf(buf_name, BUF_NAME_SIZE, "\033b%s\033n", t->n_name); 
			msg->array[0] = buf_name;
		}
		else
		{
			msg->array[0] = t->n_name;
		}

		msg->array[1] = userdefined ? "\033buser set\033n" : "default";
		msg->array[2] = (t->n_flags & PAF_INITONLY)  ? "yes" : "no";
		msg->array[3] = type;
		msg->array[4] = value;
		msg->array[5] = defvalue;
	}

	return (0);
}


DEFMMETHOD(List_Compare)
{
	GETDATA;

	struct pa_node *n1 = msg->entry1;
	struct pa_node *n2 = msg->entry2;

	LONG result  = 0;
	LONG namefix = 1; 

	switch(data->sort_column)
	{
		case 0: /* name  */
			namefix = data->sort_direction; 
			/* comparing, see below 
			 */
			break;			

		case 1: /* status */
			result = intcmp(n1->n_flags & PAF_USERDEFINED, n2->n_flags & PAF_USERDEFINED);
			break;

		case 2: /* init  */
			result = intcmp(n1->n_flags & PAF_INITONLY, n2->n_flags & PAF_INITONLY);
			break;

		case 3: /* type  */
			result = intcmp(n1->n_type, n2->n_type);
			break;

#if SUPPORT_VALUE_SORTING
		/*
		 *  XXX: smart that way? I have no idea. Quite pointless after all anyway.
		 *       -- tokai
		 */
		case 4: /* value */
			if (!(result = intcmp(n1->n_value, n2->n_value)))
			{
				result = intcmp(n1->n_type, n2->n_type);
			}
			break;

		case 5: /* default value */
			if ((result = intcmp(n1->n_default, n2->n_default))) 
			{
				result = intcmp(n1->n_type, n2->n_type);
			}
			break;
#endif

		default:
			DB(("not supported!"));
			break;
	}

	result *= data->sort_direction;

	if (result == 0) /* always sub-sort for names alphabetically */
	{
		result = stricmp(n1->n_name, n2->n_name);
	}

	return result *= namefix;
}


BEGINMTABLE
DECNEW
DECGET
DECSET
DECMMETHOD(List_Construct)
DECMMETHOD(List_Destruct)
DECMMETHOD(List_Display)
DECMMETHOD(List_Compare)
ENDMTABLE

DECSUBCLASS_NC(MUIC_List, advancedprefslistclass)
