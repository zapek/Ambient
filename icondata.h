#ifndef AMBIENT_ICONDATA_H
#define AMBIENT_ICONDATA_H
/*
 * $Id: icondata.h,v 1.16 2017/09/13 18:59:40 nadir Exp $
 */

#include "prefs.h"

struct ancillary_image {
	ULONG size;
	struct Image img;
};

enum {
	BMEFFECT_SELECTED,
	BMEFFECT_SELECTED2,
	BMEFFECT_MOUSEOVER,
	BMEFFECT_MOUSEOVER2,
	BMEFFECT_SELECTEDMOUSEOVER,
	BMEFFECT_SELECTEDMOUSEOVER2,
	BMEFFECT_DRAGDROP,
	BMEFFECT_TOTAL, /* array size */
	BMEFFECT_ALL /* for delete */
};



/*
 * Those access the x/y of the icon image.
 * Some vocabulary for users of this code:
 * Object(OBJ) is whole icon object including icon + label
 * Icon(OBJ) is the rectangular area inside which the image can be places. This is the thing that we position in windows
 * Image(IMG) is the area inside icon that represents icon image.
 *
 * Hope it won't cause confusion later.
 */

#define IDATA(obj) (( struct IconData * )muiUserData(obj))

#define ICON_GRIDSIZE (0xfff8) /* Gridsize is maxval(UWORD) - value. */
#define ICON_SETICON_LEFT(posx) ({idata->x = (((posx) & ICON_GRIDSIZE));})
#define ICON_SETICON_LEFT_NOGRID(posx) ({idata->x = (posx);})
#define ICON_SETICON_TOP(posy) ({idata->y = (((posy) & ICON_GRIDSIZE));})
#define ICON_SETICON_TOP_NOGRID(posy) ({idata->y = (posy);})

/* object (image + label) position without using _left and _top (they are 16bit) */

//#define ICON_GETOBJ_LEFT(obj) (LONG)( { struct IconData *idata = muiUserData(obj); idata->x + idata->addx + idata->bm_max_xs / 2 - _minwidth(obj) / 2;})
#define ICON_GETOBJ_LEFT(obj) (LONG)( { IDATA(obj)->x + (_conf(icon_maxsize)+IDATA(obj)->sizeadjustment) / 2 - _minwidth(obj) / 2;})
#define ICON_GETOBJ_TOP(obj) (LONG)( { IDATA(obj)->y;})

#define ICON_GETOBJ_WIDTH(obj) _minwidth(obj)
#define ICON_GETOBJ_HEIGHT(obj) _minheight(obj)


/* icon image (used area inside maxiconsize rectangle) position */

#define ICON_GETIMG_LEFT(obj) (LONG)( { IDATA(obj)->x + (_conf(icon_maxsize)+IDATA(obj)->sizeadjustment) / 2 - IDATA(obj)->bm_max_xs / 2;})
#define ICON_GETIMG_TOP(obj) (LONG)( { IDATA(obj)->y + IDATA(obj)->addy;})

#define ICON_GETIMG_WIDTH(obj) (LONG)( { IDATA(obj)->bm_max_xs;})
#define ICON_GETIMG_HEIGHT(obj) (LONG)( { IDATA(obj)->bm_max_ys;})

/* icon icon (image only) position without using _left and _top (they are 16bit) */

#define ICON_GETICON_LEFT(obj) (LONG)( { IDATA(obj)->x;})
#define ICON_GETICON_TOP(obj) (LONG)( { IDATA(obj)->y;})

#define ICON_GETICON_WIDTH(obj)  (LONG)( { _conf(icon_maxsize)+IDATA(obj)->sizeadjustment;})
//#define ICON_GETICON_WIDTH(obj) (LONG)( { struct IconData *idata = muiUserData(obj); idata->addx * 2 + idata->bm_max_xs;})
#define ICON_GETICON_HEIGHT(obj) (LONG)( { _conf(icon_maxsize)+IDATA(obj)->sizeadjustment;})


struct IconData {
	LONG x;              /* left of the icon image area (relative to window). doesn't include label. only maxiconsize rectangle */
	LONG y;              /* same for vertical position */
	LONG addx;           /* size to add to x to get the left of the object */
	LONG addy;           /* size to add to y to get the top of the object */
	STRPTR pathinfo;     /* full pathname of the icon file (including bla.info) */
	STRPTR path;         /* path */
	STRPTR iconname;     /* name of the icon to be displayed */
	STRPTR additionaltext;/* additional informations displayed under icon name */
	STRPTR iconpath;     /* path to the icon the image was taken from */
	STRPTR defaulttool;
	ULONG has_pos;       /* to know if an icon has a position or is free to go anywhere */
	ULONG has_drawerdata;
	ULONG type;
	ULONG filetype;      /* filetype, see defines (MV_Icon_FileType_#?) */
	ULONG stacksize;
	LONG win_x;          /* left of the window (relative to screen) */
	LONG win_y;          /* top of the window (relative to screen) */
	ULONG win_xs;        /* width of the window */
	ULONG win_ys;        /* height of the window */
	ULONG offs_x;
	ULONG offs_y;
	ULONG viewmode;
	ULONG sortmode;
	ULONG imagetype;
	ULONG disktype;      /* for volumes, 'DOS0', 'DOS1', etc.. */
	ULONG devicetype;    /* MV_Icon_DeviceType_Removable etc.. */
	ULONG is_shortcut;
	ULONG is_link;       /* is it a link? */
	LONG gauge_percent;

	/* File information, not always up to date */
	UQUAD filesize;
	ULONG filedate;      /* time in seconds, extracted from datestamp */

	/* Ancillary data for storing the Gadget and both Images verbatim for icon writing */
	struct Gadget *gad;
	struct ancillary_image im1; /* Image struct first then the data */
	struct ancillary_image im2; /* ditto */
	struct MinList ni1_tt;
	struct MinList ni2_tt;
	struct MinList glow_chunk;
	struct MinList png_chunk;
	APTR png_ctx;
	
	APTR svgdoc;
	APTR vgobj;

	/* Appicons */
	struct MsgPort *msgport;
	APTR appaddress;
	ULONG appid;
	ULONG appuserdata;

	/* Mouse clicks */
	ULONG selected;      /* state of user click */
	ULONG highlighted;
	ULONG checkhighlight;
	ULONG seconds;
	ULONG micros;
	ULONG doubleclick;

	ULONG needsupdate;
	struct doublebuf *dbuf; /* double buffering */

	/* Master pointers */
	ULONG scaled;

	APTR bm1_real;			/* _real bitmaps are bitmaps before scaling happens */
	APTR bm1;				/* scaled image, or same as _real is scaling wasn't necessary */

	APTR bm2_real;
	APTR bm2;

	ULONG minsize;
	ULONG maxsize;

	APTR bm_effect[BMEFFECT_TOTAL];

	ULONG alphaval;
	ULONG prefadeticks;

	#if USE_THUMBS
	ULONG thumbit;
	#endif

	/* Bitmap size helper */
	LONG bm_max_xs;
	LONG bm_max_ys;
	LONG bm_max_xs_real;
	LONG bm_max_ys_real;

	struct MinList ttlist;
	STRPTR errorstr;    /* error string */

	ULONG numimages;    /* number of images (1 or 2) */

	/* Text stuff */
	APTR tbctx;
	APTR tbctx2;
	LONG text_width;
	LONG text_height;
	ULONG text_color;
	ULONG text_bgcolor;
	ULONG text_hasbg;
	struct atextfont *afont;
	struct atextfont *smallfont;
	LONG fontspace;

	/* Context menu */
	APTR cmenu;
	ULONG parentcm; /* needed to send the MUIM_ContextMenuChoice to the parent also */
	ULONG cmgrouped; /* TRUE if the context menu is in grouped mode */

	/* Default icons */
	ULONG isdefault;
	ULONG refine;

	/* Drag Drop */
	ULONG immediate_update;

	/* Inline editing */
	APTR editobj;
	ULONG editmode;
	STRPTR editname;

	/* Fade effect */
	struct MUI_InputHandlerNode ihnode;

	ULONG infowin;

	/* Automatic buffer update */
	LONG prev_x;
	LONG prev_y;

	/* mimetype for object this icon is associated to */
	APTR mimetype;

	/* mouse click position for drag'n drop operations */
	LONG click_pos_x, click_pos_y;
	ULONG droptarget;

	ULONG dirtyflags;

	/* shared images */

	APTR reference_icon;
	ULONG reference_count;
	ULONG viewid; /* view to which icon is assigned (affected deficonpool). special values are MV_ViewID_XXX */
	LONG sizeadjustment; /* adjustment for default icon size */

	/* icon object locking */

	LONG locked; /* set (with atomic op) from thread if you will access this object. will prevent view to dispose it */
};


#endif /* AMBIENT_ICONDATA_H */
