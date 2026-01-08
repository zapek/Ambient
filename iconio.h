#ifndef AMBIENT_ICONIO_H
#define AMBIENT_ICONIO_H
/*
 * $Id: iconio.h,v 1.13 2017/09/12 21:16:02 nadir Exp $
 */

/*
 * Icon file layout
 * ----------------
 *
 *	 struct DiskObject:
 *	  UWORD do_Magic;
 *	  UWORD do_Version;
 *	  struct Gadget do_Gadget:
 *	   struct Gadget *NextGadget;       0x4
 *	   WORD LeftEdge;                   0x8
 *	   WORD TopEdge;                    0xa
 *	   WORD Width;                      0xc
 *	   WORD Height;                     0xe
 *	   UWORD Flags;                     0x10
 *	   UWORD Activation;                0x12
 *	   UWORD GadgetType;                0x14
 *	   APTR GadgetRender;               0x16
 *	   APTR SelectRender;               0x1a
 *	   struct IntuiText *GadgetText;    0x1e
 *	   LONG MutualExclude;              0x22
 *	   APTR SpecialInfo;                0x26
 *	   UWORD GadgetID;                  0x2a
 *	   APTR UserData;                   0x2c
 *	  UBYTE do_Type;                    0x30
 *	                                    pad
 *	  char * do_DefaultTool;            0x32
 *	  char ** do_ToolTypes;             0x36
 *	  LONG do_CurrentX;                 0x3a
 *	  LONG do_CurrentY;                 0x3e
 *	  struct DrawerData *do_DrawerData; 0x42
 *	  char * do_ToolWindow;             0x46
 *	  LONG do_StackSize;                0x4a
 *
 *	  if do_DrawerData, we have a DrawerData
 *
 */

enum {
	ICONTAG_FreeList = TAG_USER + 1, /* (struct FreeList *fl) use a FreeList for memory operations (iconlib only) */
	ICONTAG_FileSize,                /* (ULONG) known filesize to avoid gathering it */
	ICONTAG_FileType,                /* (ULONG) known filetype to avoid gathering it */
	ICONTAG_Ancillary,               /* (default: FALSE) adds images as ancillary data */
	ICONTAG_Deficon,                 /* (default: FALSE) try to find out a deficon as fallback */
	ICONTAG_Position,                /* (default: TRUE) read in position values */
	ICONTAG_End,                     /* (default: TRUE) send a MM_Icon_End method when done */
	ICONTAG_Infowin,                 /* (default: FALSE) this is loaded from an infowin */
	ICONTAG_IsAssign,                /* (default: FALSE) */
	ICONTAG_GetImage,                /* (default: TRUE) load icon image */
};

ULONG v_icon_read(STRPTR filename, APTR obj, struct TagItem *tags);
ULONG icon_read(STRPTR filename, APTR obj, ...);
STRPTR icon_read_infostring_buf(APTR fh, STRPTR buf, ULONG buflen, ULONG *size);

#include "sxmlc.h"

#ifdef BUILD_ICONLIB
ULONG icon_gettype(CONST_STRPTR name);
STRPTR icon_read_infostring(APTR fh, ULONG *size, struct FreeList *fl);
BOOL SetSVGIconContents(APTR obj, XMLNode *morphosicon, struct FreeList *fl);
#else
BOOL SetSVGIconContents(APTR, XMLNode *morphosicon);
STRPTR icon_read_infostring(APTR fh, ULONG *size);

ULONG tr_icon_read(APTR obj, STRPTR filename, ULONG all);
ULONG tr_icon_write_dummy(APTR obj, CONST_STRPTR filename);
ULONG tr_icon_write(APTR obj, CONST_STRPTR filename);
ULONG iconio_get_default_icon(STRPTR filename, APTR obj);
ULONG tr_icon_update(APTR obj, CONST_STRPTR path, CONST_STRPTR comment, ULONG flags, ULONG mode);
#endif
ULONG icon_write_infostring(APTR fh, CONST_STRPTR s, ULONG len);

#endif /* AMBIENT_ICONIO_H */
