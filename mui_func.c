/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2016 Ambient Open Source Team
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
 * $Id: mui_func.c,v 1.18 2026/01/25 17:36:24 kronos Exp $
*/


#ifndef PANEL_APP
#include "ambient.h"
#else
#include <stddef.h>
#include <string.h>
#include <exec/nodes.h>
#include "debug.h"

extern const char * const __stringtable[];
#define GSI(x) ( STRPTR )__stringtable[x]
#endif


/* public */
#include <mui/NumericString_mcc.h>
#include <proto/intuition.h>

/* private */
#include "mui_func.h"


ULONG mui_getv(APTR obj, ULONG attr)
{
	ULONG v;

	GetAttr(attr, obj, &v);
	return (v);
}


/*
** ParseHotkey
*/
TEXT ParseHotKey(CONST_STRPTR string_num)
{
	STRPTR Button;
	TEXT Key = '\0';

	if(string_num)
	{
		Button = strchr(string_num, '_');

		if(Button)
			Key = ToLower(Button[1]);
	}

	return(Key);
}

/*
** SLabel
*/
APTR SLabel(CONST_STRPTR label)
{
	APTR obj = MUI_MakeObject(MUIO_Label, (ULONG)label, MUIO_Label_DontCopy);
	return obj;
}

/*
** SLabel1
*/
APTR SLabel1(CONST_STRPTR label)
{
	APTR obj = MUI_MakeObject(MUIO_Label, (ULONG)label, MUIO_Label_DontCopy|MUIO_Label_SingleFrame);
	return obj;
}

/*
** SLabe2
*/
APTR SLabel2(CONST_STRPTR label)
{
	APTR obj = MUI_MakeObject(MUIO_Label, (ULONG)label, MUIO_Label_DontCopy|MUIO_Label_DoubleFrame);
	return obj;
}

/*
** text
*/
APTR text(ULONG label)
{
	return TextObject, MUIA_Text_Contents, GSI(label), End;
}

/*
** text2
*/
APTR text2(CONST_STRPTR label)
{
	return TextObject, MUIA_Text_Contents, label, End;
}

/*
** button
*/
APTR button(ULONG label, ULONG helpid)
{
	APTR obj = MUI_MakeObject(MUIO_Button, (ULONG)GSI(label));
	SetAttrs(obj, MUIA_CycleChain, 1, helpid ? MUIA_ShortHelp : TAG_IGNORE, (ULONG)GSI(helpid), TAG_DONE);
	return (obj);
}

/*
** findview
**
** Finds a viewobject from a deeper object.
*/
APTR findview(APTR obj)
{
	ULONG isview;

	while (obj)
	{
		if (get(obj, MA_Viewgroup_IsView, &isview))
		{
			if (isview)
			{
				return (obj);
			}
		}
		obj = _parent(obj);
	}
	return (NULL);
}

/*
** filter_escapecodes
**
** function to filter out MUI escape codes except bold
*/

#define FILTERBUFSIZE 1024

STRPTR filter_escapecodes(CONST_STRPTR src)
{
	static TEXT tmp[ FILTERBUFSIZE ];
	STRPTR dst = tmp;
	int x = 0;

	ASSERT(src);

	while( *src && x++< (FILTERBUFSIZE-1) )
	{
		if( *src == 27 )
		{
			if( src[1] != 'b' )
				*dst++ = '*';
			else
				*dst++ = *src;
		}
		else
			*dst++ = *src;

		src++;
	}
	*dst = 0;

	return( tmp );
}


/*
** These functions are for simple and easy gadget creation (geit)
*/

/*
** MUIGetUnderScore - Returns the char followed by "_"
*/

TEXT MUIGetUnderScore( ULONG text)
{
	if (text)
	{
		char *c;
		if( (c = strchr( GSI( text),'_')) )
		{
			return(ToLower(*(c+1)));
		}
	}
	return(0);
}

/*
** MUIInitStringArray
**
** Fills the specified array with the given range of strings. This function
** is usefull when initing tabs or cycles.
**
** It's save to specify 0,0. Also last may be lower than first. So you can
** use catalog strings in reverse order.
**
*/

void MUIInitStringArray( APTR array[], ULONG first, ULONG last )
{

	ULONG i;

	if( array && first && last )
	{
		if( last > first )
		{
			for( i = first ; i <= last ; i++)
			{
				array[i-first] = GSI( i );
			}
			array[i-first] = NULL;
		}
		else
		{
			for( i = last ; i <= first ; i++)
			{
				array[i-last] = GSI( ((first-i)+last) );
			}
			array[i-last] = NULL;
		}
	}
}


/*
** MUICreateLabel
**
** Just like MUI label(), but with locale id as label
** text, MUIA_DoubleBuffer set and bubble help support.
**
** NOTE: Label must be followed by bubble help!
*/

APTR MUICreateLabel( ULONG text, ULONG flags)
{
	APTR obj;

	obj = TextObject,
			MUIA_Text_Contents    , GSI( text ),
			MUIA_Weight           , 0,
			MUIA_InnerLeft        , 0,
			MUIA_InnerRight       , 0,
			MUIA_FramePhantomHoriz, TRUE,
			text                             ? MUIA_ShortHelp   : TAG_IGNORE        , GSI( text+1), /* HELP is always behind label in catalog */
			flags & 0xff                     ? TAG_IGNORE       : MUIA_Text_HiIndex , '_',
			flags & 0xff                     ? MUIA_Text_HiChar : TAG_IGNORE        , flags & 0xff,
			flags & MUIO_Label_SingleFrame   ? MUIA_Frame       : TAG_IGNORE        , MUIV_Frame_Button,
			flags & MUIO_Label_DoubleFrame   ? MUIA_Frame       : TAG_IGNORE        , MUIV_Frame_String,
			flags & MUIO_Label_LeftAligned   ? TAG_IGNORE       : MUIA_Text_PreParse, "\33r",
			flags & MUIO_Label_Centered      ? MUIA_Text_PreParse : TAG_IGNORE      , "\33c",
			flags & MUIO_Label_FreeVert      ? MUIA_Text_SetVMax  : TAG_IGNORE      , FALSE,
                End;

		return(obj);
}



/*
** MUICreateString
**
** NOTE: Label must be followed by bubble help!
*/
APTR MUICreateString( ULONG text, ULONG maxchars, STRPTR help)
{
	APTR string;

	string = MUI_NewObject( MUIC_String,
							MUIA_Frame                                     , MUIV_Frame_String,
							((text) ? MUIA_ControlChar : TAG_IGNORE)       , MUIGetUnderScore( text ),
							MUIA_CycleChain                                , 1,
							((help) ? MUIA_HelpNode : TAG_IGNORE)          , help,
							((text) ? MUIA_ShortHelp : TAG_IGNORE)         , GSI( text+1), /* HELP is always behind label in catalog */
							((maxchars) ? MUIA_String_MaxLen : TAG_IGNORE) , maxchars,
	                        TAG_DONE);

	return( string );
}

/*
** MUICreateInteger
**
** NOTE: Label must be followed by bubble help!
*/
APTR MUICreateInteger(ULONG text, ULONG maxchars, STRPTR help, LONG min, LONG max, LONG factor)
{
	APTR integer;

	integer = MUI_NewObject( MUIC_NumericString,
							((text) ? MUIA_ControlChar : TAG_IGNORE)       , MUIGetUnderScore( text ),
							MUIA_CycleChain                                , 1,
							((help) ? MUIA_HelpNode : TAG_IGNORE)          , help,
							((text) ? MUIA_ShortHelp : TAG_IGNORE)         , GSI( text+1), /* HELP is always behind label in catalog */
							((maxchars) ? MUIA_String_MaxLen : TAG_IGNORE) , maxchars,
							MUIA_Numeric_Min, min,
							MUIA_Numeric_Max, max,
							MUIA_Numeric_FormatFactor, factor,
		TAG_DONE);

	return( integer );
}

/*
** MUICreateText
**
** NOTE: Label must be followed by bubble help!
*/

APTR MUICreateTextNoFrame( ULONG text, STRPTR contents)
{
	return( MUI_NewObject(MUIC_Text,
						MUIA_Text_Contents, contents,
						MUIA_ShortHelp, GSI( text+1),
						TAG_DONE ) );
}

/*
** MUICreateMarkableText
**
** NOTE: Label must be followed by bubble help!
*/

APTR MUICreateMarkableTextNoFrame( ULONG text, STRPTR contents)
{
	return( MUI_NewObject(MUIC_Text,
						MUIA_Text_Contents, contents,
						MUIA_Text_Marking, TRUE,
						MUIA_ShortHelp, GSI( text+1),
						TAG_DONE ) );
}

/*
** MUICreateButton
**
** NOTE: Label must be followed by bubble help!
*/
APTR MUICreateButton( ULONG text, STRPTR help)
{
	APTR button;
 
	if( (button = MUI_NewObject(MUIC_Text,
										MUIA_Frame                              , MUIV_Frame_Button,
										MUIA_Font                               , MUIV_Font_Button,
										MUIA_Background                         , MUII_ButtonBack,
										MUIA_ControlChar                        , MUIGetUnderScore( text ),
										MUIA_CycleChain                         , 1,
										((help) ? MUIA_HelpNode : TAG_IGNORE)   , help,
										MUIA_ShortHelp                          , GSI( text+1),
										MUIA_Text_Contents                      , GSI(text),
										MUIA_Text_PreParse                      , "\33c", /* center gadget text */
										MUIA_Text_HiChar                        , MUIGetUnderScore( text ),
										MUIA_Text_HiIndex						, '_',
										MUIA_InputMode                          , MUIV_InputMode_RelVerify,
										TAG_DONE) ))
	{
		SetAttrs( button, MUIA_HelpNode, help, TAG_DONE);
	}
	return( button );
}

/*
** MUICreateImageButton
**
** NOTE: Label must be followed by bubble help!
**
*/

APTR MUICreateImageButton( ULONG text, ULONG imagespec, STRPTR help )
{

	return( MUI_NewObject(MUIC_Image, MUIA_Background, MUII_ButtonBack,
						ButtonFrame,
						MUIA_Image_FontMatchHeight              , TRUE,
						MUIA_Image_Spec                         , imagespec,
						MUIA_InputMode                          , MUIV_InputMode_RelVerify,
						MUIA_Image_FreeHoriz                    , TRUE,
						MUIA_CycleChain                         , 1,
						MUIA_ShortHelp                          , GSI( text+1),
						((help) ? MUIA_HelpNode : TAG_IGNORE)   , help,
						TAG_DONE));
 
}

/*
** MUICreateCheckbox
**
** NOTE: Label must be followed by bubble help!
*/
APTR MUICreateCheckbox( ULONG text, ULONG defstate, STRPTR help )
{
	APTR checkbox;

	checkbox = MUI_NewObject( MUIC_Image,   MUIA_Frame                                  , MUIV_Frame_ImageButton,
	                                        MUIA_Background                             , MUII_ButtonBack,
	                                        ((text) ? MUIA_ControlChar : TAG_IGNORE)    , MUIGetUnderScore( text ),
	                                        MUIA_CycleChain                             , 1,
	                                        ((help) ? MUIA_HelpNode : TAG_IGNORE)       , help,
	                                        ((text) ? MUIA_ShortHelp : TAG_IGNORE)      , GSI( text + 1 ), /* HELP is always behind label in catalog */
	                                        MUIA_Image_FreeVert                         , TRUE,
	                                        MUIA_InputMode                              , MUIV_InputMode_Toggle,
	                                        MUIA_Image_Spec                             , MUII_CheckMark,
	                                        MUIA_Selected                               , defstate,
	                                        MUIA_ShowSelState                           , FALSE,
	                                        TAG_DONE);

	return( checkbox );
}


/*
** MUICreatePoppen
**
** NOTE: Label must be followed by bubble help and by popup window title!
*/

APTR MUICreatePoppen( ULONG text, STRPTR help )
{
	APTR poppen;

	poppen = MUI_NewObject(MUIC_Poppen,
	                                    ((text) ? MUIA_ControlChar : TAG_IGNORE)    , MUIGetUnderScore( text ),
	                                    MUIA_CycleChain                             , 1,
	                                    ((help) ? MUIA_HelpNode : TAG_IGNORE)       , help,
	                                    ((text) ? MUIA_ShortHelp : TAG_IGNORE)      , GSI( text + 1 ), /* HELP is always behind label in catalog */
	                                    InnerSpacing(0, 0),
	                                    ((text) ? MUIA_Window_Title : TAG_IGNORE)   , GSI( text + 2 ),
	                                    TAG_DONE);

	return( poppen );
}

/*
** MUICreatePopButton
**
** NOTE: Label must be followed by bubble help and by popup window title!
*/

APTR MUICreatePopButton( ULONG text, ULONG img, STRPTR help)
{
	APTR obj;

	if( (obj = MUI_MakeObject(MUIO_PopButton,img)))
	{
		TEXT key = 0;
		if( text ) {
			if( ( key = MUIGetUnderScore( text ) ) ) {
				key = ToUpper( key );
			}
		}
		
		SetAttrs( obj, ((help) ? MUIA_HelpNode  : TAG_IGNORE) , help,
						((text) ? MUIA_ShortHelp : TAG_IGNORE) , GSI( text + 1 ),
						((key) ? MUIA_ControlChar : TAG_IGNORE), key,
						MUIA_CycleChain               , 1,
		TAG_DONE);
	}

	return( obj );
}


/*
** MUICreateCycle
**
** NOTE: Label must be followed by bubble help!
*/
APTR MUICreateCycle( ULONG text, APTR cyclelabels, ULONG first, ULONG last, STRPTR help )
{
	APTR cycle;

	MUIInitStringArray( cyclelabels, first, last );

	cycle = MUI_NewObject( MUIC_Cycle,  MUIA_Frame                                  , MUIV_Frame_Button,
	                                    MUIA_Font                                   , MUIV_Font_Button,
	                                    MUIA_Cycle_Entries                          , cyclelabels,
	                                    MUIA_Background                             , MUII_ButtonBack,
	                                    ((text) ? MUIA_ControlChar : TAG_IGNORE)    , MUIGetUnderScore( text ),
	                                    MUIA_CycleChain                             , 1,
	                                    ((help) ? MUIA_HelpNode : TAG_IGNORE)       , help,
	                                    ((text) ? MUIA_ShortHelp : TAG_IGNORE)      , GSI( text+1), /* HELP is always behind label in catalog */
	                                    TAG_DONE);

	return( cycle );
}

/*
** MUICreateSlider
**
** NOTE: Label must be followed by bubble help!
*/
APTR MUICreateSlider( ULONG text, ULONG min, ULONG max, ULONG val, STRPTR help)
{
	APTR slider;

	slider = SliderObject,
	                        MUIA_Numeric_Min                            , min,
	                        MUIA_Numeric_Max                            , max,
	                        MUIA_Numeric_Value                          , val,
	                        ((text) ? MUIA_ControlChar : TAG_IGNORE)    , MUIGetUnderScore( text ),
	                        MUIA_CycleChain                             , 1,
	                        ((help) ? MUIA_HelpNode : TAG_IGNORE)       , help,
	                        ((text) ? MUIA_ShortHelp : TAG_IGNORE)      , GSI( text+1), /* HELP is always behind label in catalog */
	                        End;

	return( slider );
}

/*
** MUICreateToggleImageButton using a ImageObject
**
*/
APTR MUICreateToggleImageButton( STRPTR ispec, ULONG defstate, ULONG shelp )
{
	APTR but;

	but = ImageObject,
		ButtonFrame,
		//MUIA_Weight, 0,
		MUIA_Font, MUIV_Font_Tiny,
		MUIA_Selected, defstate,
		((shelp) ? MUIA_ShortHelp : TAG_IGNORE), GSI ( shelp ),
		MUIA_InputMode, MUIV_InputMode_Toggle,
		MUIA_CycleChain, 1,
		MUIA_Background, MUII_ButtonBack,
		MUIA_Image_Spec, ispec,
		MUIA_Image_FreeVert, TRUE, // bitRocky: hmm, an ext MUI brush (4:<name>) won't be scaled! why?
		MUIA_Image_FreeHoriz, TRUE,
		MUIA_Image_FontMatch, TRUE,
	End;
 
	return( but );
}

/*
** MUICreateToggleButton using an TextObject, because of the different selected background compared to an ImageObject!
**
*/
APTR MUICreateToggleButton( STRPTR txt, ULONG defstate, ULONG shelp )
{
	APTR but;

	but = TextObject,
		ButtonFrame,
		//MUIA_Weight, 0,
		MUIA_Selected, defstate,
		((shelp) ? MUIA_ShortHelp : TAG_IGNORE), GSI ( shelp ),
		MUIA_InputMode, MUIV_InputMode_Toggle,
		MUIA_CycleChain, 1,
		MUIA_Background, MUII_ButtonBack,
		MUIA_Text_Contents, txt,
	End;
 
	return( but );
}


#if USE_MUICLASSES_NAMEHACK
#define DCN(x) char MUIC_##x[]= { #x ".mui" }

DCN(Application);
DCN(Register);
DCN(Cycle);
DCN(Image);
DCN(Popasl);
DCN(String);
DCN(Poppen);
DCN(Slider);
DCN(Scrollgroup);
DCN(Window);
DCN(Floattext);
DCN(Menu);
DCN(Bitmap);
DCN(Virtgroup);
DCN(Scrollbar);
DCN(Notify);
DCN(Area);
DCN(Group);
DCN(Rectangle);
DCN(Popscreen);
DCN(Popobject);
DCN(Balance);
DCN(Text);
DCN(List);
DCN(Gauge);
DCN(Listview);
DCN(Scale);
DCN(Dataspace);
DCN(Menuitem);
DCN(Menustrip);
DCN(Bodychunk);
DCN(Aboutmui);
DCN(Numericbutton);
DCN(Radio);
DCN(Popimage);
DCN(Popframe);
DCN(Poplist);
#if !USE_LEGACY
#ifndef MUIC_Popfrimage
DCN(Popfrimage);
#endif
#endif
#endif
