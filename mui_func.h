#ifndef AMBIENT_MUI_FUNC_H
#define AMBIENT_MUI_FUNC_H
/*
 * Custom MUI functions and macros
 * -------------------------------
 * - This header should be included for any file using MUI related thing
 *
 * - Please don't add useless stuff here. Use direct MUI macros/functions
 * whenever possible because they're the same for all apps.
 *
 * $Id: mui_func.h,v 1.12 2017/08/09 23:41:19 cyfm Exp $
 *
*/

extern APTR app;
extern APTR aboutwin;
extern APTR aboutmoswin;
extern APTR executewin;
extern APTR cxwin;
extern APTR systeminfowin;
extern STRPTR screentitle;

#include <proto/intuition.h>

#include <exec/libraries.h>

/* _DCC -- Hack to get MUIC_xxx defines as extern char[] */
#if USE_MUICLASSES_NAMEHACK
#define _DCC
#endif
extern struct Library *MUIMasterBase;
#include <intuition/intuition.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>
#include <clib/alib_protos.h>
#if USE_INTERNAL_DOMETHOD
#undef DoMethodA
#endif
#include <proto/utility.h>
#if USE_MUICLASSES_NAMEHACK
#undef _DCC
#endif

#include "classes.h"

#include "mui_internal.h"

STRPTR getpubname(APTR winobj);
APTR hbar(void);
APTR makebutton(ULONG stringid);
int getmenucheck(ULONG menuid);
APTR findview(APTR obj);

ULONG mui_getv(APTR o, ULONG a);
#define getv(_o,_a) mui_getv(_o,_a)
void mui_askminmax(APTR obj);
ULONG get_control_char(STRPTR s);

#define _isinobject(_x,_y) (_between(_left(obj),(_x),_right(obj)) && _between(_top(obj),(_y),_bottom(obj)))
#define _isinobject2(_o,_x,_y) (_between(_left(_o),(_x),_right(_o)) && _between(_top(_o),(_y),_bottom(_o)))
#define _between(_a,_x,_b) ((_x)>=(_a) && (_x)<=(_b))
#define _isinwinborder(_x,_y) \
	((_between(0, (_x), _window(obj)->Width) && _between(0, (_y), _window(obj)->BorderTop)) || \
	 (_between(_window(obj)->Width - _window(obj)->BorderRight, (_x), _window(obj)->Width) && _between(0, (_y), _window(obj)->Height)) || \
	 (_between(0, (_x), _window(obj)->Width) && _between(_window(obj)->Height - _window(obj)->BorderBottom, (_y), _window(obj)->Height)) || \
	 (_between(0, (_x), _window(obj)->BorderLeft) && _between(0, (_y), _window(obj)->Height)))

#ifndef _parent
#define _parent(_o) ((Object *)getv(_o, MUIA_Parent))
#endif

/*
 * The following are stuntzi ideas mostly.
 */
#if USE_INLINE_NEXTOBJECT
#undef NextObject
#define NextObject(_cstate) \
	({ \
		struct Node **_nptr = (struct Node **)(APTR)_cstate; \
		struct Node *_this = *_nptr; \
		APTR _next = _this->ln_Succ; \
		if (_next) \
		{ \
			*_nptr = _next; \
			_next = BASEOBJECT(_this); \
		} \
		_next; \
	})
#endif


#define FORCHILD(_o, _a) \
	{ \
		APTR child, _cstate = (APTR)((struct MinList *)getv(_o, _a))->mlh_Head; \
		while ((child = NextObject(&_cstate)))

#define NEXTCHILD }


#if USE_INLINE_NEXTTAGITEM
#define FORTAG(_tagp) \
	{ \
		struct TagItem *tag = (struct TagItem *)(_tagp); \
		if (tag) \
		{ \
			for (;;tag++) \
			{ \
				if (tag->ti_Tag & ~0x3) \
				{ \
					switch ((int)tag->ti_Tag)
#define NEXTTAG \
				} \
				else if (tag->ti_Tag == TAG_DONE) break; \
				else if (tag->ti_Tag == TAG_MORE) { if (!tag->ti_Data) break; tag = ((struct TagItem *)tag->ti_Data) - 1; } \
				else if (tag->ti_Tag == TAG_SKIP) tag += (int)tag->ti_Data; \
			} \
		} \
	}

#else

#define FORTAG(_tagp) \
	{ \
		struct TagItem *tag, *_tags = (struct TagItem *)(_tagp); \
		while ((tag = NextTagItem(&_tags))) switch ((int)tag->ti_Tag)
#define NEXTTAG }
#endif

#define _view(_o) findview(_o)

/*
 * Static labels. They don't copy the
 * string.
 */
#if 0
#define SLabel(_label)   MUI_MakeObject(MUIO_Label,(ULONG)_label,MUIO_Label_DontCopy)
#define SLabel1(_label)  MUI_MakeObject(MUIO_Label,(ULONG)_label,MUIO_Label_DontCopy|MUIO_Label_SingleFrame)
#define SLabel2(_label)  MUI_MakeObject(MUIO_Label,(ULONG)_label,MUIO_Label_DontCopy|MUIO_Label_DoubleFrame)
#endif

/*
 * Static labels with full locale support.
 *
 * These functions should replace the SLabel(), SLabel1 and SLabel2() functions
 * step by step.
 *
 */

#define NSLabel(t)  MUICreateLabel( t, 0)
#define NSLabel1(t) MUICreateLabel( t, MUIO_Label_SingleFrame)
#define NSLabel2(t) MUICreateLabel( t, MUIO_Label_DoubleFrame)



#define TinyButton(name)\
	TextObject,\
		ButtonFrame,\
		MUIA_Font, MUIV_Font_Tiny,\
		MUIA_Text_Contents, name,\
		MUIA_InputMode    , MUIV_InputMode_RelVerify,\
		MUIA_Background   , MUII_ButtonBack,\
		MUIA_Weight		  , 0,\
		End

/*
 * Clipping stuff.
 */
#define ADD_CLIPPING cliphandle = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj))
#define REMOVE_CLIPPING MUI_RemoveClipping(muiRenderInfo(obj), cliphandle)

/*
 * Avoids typos.. should be merged with vapor.h somehow
 * or put into os-include
 */
#define INITTAGS (((struct opSet *)msg)->ops_AttrList)

#define IEQUALIFIER_SHIFTS   (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)
#define IEQUALIFIER_ALTS     (IEQUALIFIER_LALT | IEQUALIFIER_RALT)
#define IEQUALIFIER_COMMANDS (IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND)
#define IEQUALIFIER_CONTROLS (IEQUALIFIER_CONTROL)

TEXT ParseHotKey(CONST_STRPTR string_num);

APTR SLabel(CONST_STRPTR label);
APTR SLabel1(CONST_STRPTR label);
APTR SLabel2(CONST_STRPTR label);
APTR text(ULONG label);
APTR text2(CONST_STRPTR label); // non-lozalized version
APTR button( ULONG label, ULONG helpid );
APTR ebutton( ULONG label, ULONG helpid );
APTR string( CONST_STRPTR def, int maxlen, int sh );
void tickapp( void );

STRPTR filter_escapecodes(CONST_STRPTR src);


/*
** These functions are for simple and easy gadget creation (geit)
*/

TEXT MUIGetUnderScore   (ULONG text);
void MUIInitStringArray (APTR array[], ULONG first, ULONG last );

APTR MUICreateLabel               ( ULONG text, ULONG flags);
APTR MUICreateString              ( ULONG text, ULONG maxchars, STRPTR help );
APTR MUICreateInteger             ( ULONG text, ULONG maxchars, STRPTR help, LONG min, LONG max, LONG factor );
APTR MUICreateTextNoFrame         ( ULONG text, STRPTR contents);
APTR MUICreateButton              ( ULONG text, STRPTR help );
APTR MUICreateImageButton         ( ULONG text, ULONG imagespec, STRPTR help );
APTR MUICreateCheckbox            ( ULONG text, ULONG defstate, STRPTR help );
APTR MUICreatePoppen              ( ULONG text, STRPTR help );
APTR MUICreatePopButton           ( ULONG text, ULONG img, STRPTR help);
APTR MUICreateCycle               ( ULONG text, APTR labels, ULONG first, ULONG last, STRPTR help );
APTR MUICreateSlider              ( ULONG text, ULONG min, ULONG max, ULONG val, STRPTR help);
APTR MUICreateMarkableTextNoFrame ( ULONG text, STRPTR contents);

APTR MUICreateToggleImageButton   ( STRPTR ispec, ULONG defstate, ULONG shelp );
APTR MUICreateToggleButton        ( STRPTR txt, ULONG defstate, ULONG shelp );

#endif /* AMBIENT_MUI_FUNC_H */
