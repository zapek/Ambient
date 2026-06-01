#ifndef AMBIENT_ICON_INTERNAL_LIB_H
#define AMBIENT_ICON_INTERNAL_LIB_H
/*
 * $Id: icon_internal.h,v 1.8 2025/09/06 01:31:01 piru Exp $
 */

#include <workbench/workbench.h>

struct BitMap;

/*
 * This is the "official" DiskObject
 * structure. That private field is assumed
 * to be there by some apps.
 */
struct KnownDiskObject {
	struct DiskObject diskobj;
	struct FreeList *fl;       /* this is safe for WB 3.1 */
};

struct ToolTypeNode {
	struct MinNode n;
	TEXT tt[0];
};

/*
 * Extended DiskObject structure
 * for our own use.
 */
#define OWN_MAGIC 0x466f4164

struct OwnDiskObject {
	struct DiskObject diskobj;
	struct FreeList *fl;       /* this is safe for WB 3.1 */
	/* start of additions */
	ULONG ownmagic;
	APTR ownptr;
	APTR png_context;
	ULONG ttnum;
	ULONG ttcur;
	STRPTR path;
	APTR glowchunk;
	ULONG glowsize;
	struct BitMap *pngimage;
	ULONG pngimage_width;
	ULONG pngimage_height;
	struct BitMap *pngimage2;
	APTR svgdoc;
	struct MinList tooltypelist;
	//ULONG haspos;
	//ULONG hasdrawerdata;
	ULONG viewmode;
	ULONG origflags;
	UWORD origviewmodes;
};

#define ISOWN(x) (((struct OwnDiskObject *)x)->ownmagic == OWN_MAGIC && ((struct OwnDiskObject *)x)->ownptr == x)

/* Constants */
#define MAX_DEFICON_FILEPATH	256

/* Number of entries in the mementrys in the freelists */
#define FREELIST_MEMLISTENTRIES 20

#pragma pack(2)

/*
 * That can be found in the *first* RKRM. Editions
 * before that were stripped from that info.
 */

/*
 * That structure is old and weird.
 * Only sizeof(struct NewWindow) + 2 * sizeof(LONG)
 * was written to disk though.
 */
struct RealOldDrawerData
{
	struct NewWindow dd_NewWindow;   /* args to open window */
	LONG             dd_CurrentX;    /* current x coordinate of origin */
	LONG             dd_CurrentY;    /* current y coordinate of origin */
	LONG             dd_MinX;        /* smallest x coordinate in window */
	LONG             dd_MinY;        /* smallest y coordinate in window */
	LONG             dd_MaxX;        /* largest x coordinate in window */
	LONG             dd_MaxY;        /* largest y coordinate in window */
	struct Gadget    dd_HorizScroll;
	struct Gadget    dd_VertScroll;
	struct Gadget    dd_UpMove;
	struct Gadget    dd_DownMove;
	struct Gadget    dd_LeftMove;
	struct Gadget    dd_RightMove;
	struct Image     dd_HorizImage;
	struct Image     dd_VertImage;
	struct PropInfo  dd_HorizProp;
	struct PropInfo  dd_VertProp;
	struct Window    *dd_DrawerWin;  /* pointer to drawers window */
	struct WBObject  *dd_Object;     /* back pointer to drawer object */
	struct List      dd_Children;    /* where our children hang out */
	LONG             dd_Lock;
};

struct WBObject
{   
	struct Node     wo_MasterNode;  /* all objects are on this list */
	struct Node     wo_Sibling;     /* list of drawer members */
	struct Node     wo_SelectNode;  /* list of all selected objects */
	struct Node     wo_UtilityNode; /* function specific linkages */
	struct WBObject *wo_Parent;
	UBYTE           wo_Flags;       /* icon currently in a window | we're a drawer, and it's open | our icon is selected | set if icon is in the background */
	UBYTE           wo_Type;        /* what flavor object is this? */
	UWORD           wo_UseCount;    /* number of references to this object */
	char            *wo_Name;       /* this object's textual name */
	WORD            wo_NameXOffset;
	WORD            wo_NameYOffset;
	char            *wo_DefaultTool;
	struct DrawerData *wo_DrawerData; /* if this is a drawer or disk */
	struct Window   *wo_Window;       /* each object's icon lives here */
	LONG            wo_CurrentX;      /* virtual X in drawer */
	LONG            wo_CurrentY;      /* virtual Y in drawer */
	char            **wo_ToolTypes;   /* the types for this tool */
	struct Gadget   wo_Gadget;        /* NOT a pointer, but an instance of a gadget structure */
	struct FreeList wo_FreeList;      /* this objects free list */
	char            *wo_ToolWindow;   /* character string for tool's window */
	LONG            wo_StackSize;     /* how much stack to give to this */
	LONG            wo_Lock;          /* if this tool is in the backdrop */
};


#define	WBOFLAGF_ICONDISP	0
#define	WBOFLAGF_DRAWEROPEN	1
#define	WBOFLAGF_SELECTED	2
#define	WBOFLAGF_UTILITYNODE	3

#define TMAlloc(size,type) ((type)MAlloc(size))
#define ObjAlloc(obj,size,type) ((type)OAlloc(obj,size))
#define STREQ(a,b) (!strcmp(a,b))

/*
 * Each message that comes into the WorkbenchPort must have a type field
 * in the preceeding short. These are the defines for this type
 */
#define MTYPE_PSTD       1 /* a "standard Potion" message */
#define MTYPE_TOOLEXIT   2 /* exit message from our tools */
#define MTYPE_DISKCHANGE 3 /* dos telling us of a disk change */
#define MTYPE_TIMER      4 /* we got a timer tick */
#define MTYPE_CLOSEDOWN  5 /* <unimplemented> */
#define MTYPE_IOPROC     6 /* <unimplemented> */

/*
 * We use the gadget id field to encode some special information
 */
#define GID_WBOBJECT    0 /* a normal workbench object */
#define GID_HORIZSCROLL 1 /* the horizontal scroll gadget for a drawer */
#define GID_VERTSCROLL  2 /* the vertcal scroll gadget for a drawer */
#define GID_LEFTSCROLL  3 /* move one window left */
#define GID_RIGHTSCROLL 4 /* move one window right */
#define GID_UPSCROLL    5 /* move one window up */
#define GID_DOWNSCROLL  6 /* move one window down */
#define GID_NAME        7 /* the name field for an object */


#pragma pack()

#endif /* AMBIENT_ICON_INTERNAL_LIB_H */
