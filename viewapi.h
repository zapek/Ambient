#ifndef AMBIENT_VIEWAPI_H
#define AMBIENT_VIEWAPI_H
/*
 * $Id: viewapi.h,v 1.10 2017/08/21 06:23:02 cyfm Exp $
 */

struct IClass;

struct viewnode {
	struct MinNode n;
	struct Library *libbase;
	struct IClass *cl; /* speed up */
	struct TagItem *querytagarray;
	ULONG internal; /* ambient built-in */
	ULONG id;       /* unique view ID (for the duration of the session) */
	ULONG namehash;
	STRPTR label; /* XXX: Because we use matching by name in many places in code, it can't be localized. This one can */
	char name[0]; /* view's name (gathered from the filename) */
};

enum {
	AVIEW_Query_Version = TAG_USER + 0x512,
	AVIEW_Query_Revision,
	AVIEW_Query_Copyright,
	AVIEW_Query_Info,
	AVIEW_Query_APIVersion,
	AVIEW_Query_HasPrefs,
	AVIEW_Query_MimeType,
	AVIEW_Query_MimeExtension,
	AVIEW_Query_Viewmode_Name,
	AVIEW_Query_Viewmode_RexxName,
	AVIEW_Query_Flags,            /* mask constructed from VF_ flags */
};

#define VF_SCROLLWIN   (1 << 0UL) /* wants a scrollgroup with scrollbars in window */
#define VF_SCROLLGROUP (1 << 1UL) /* wants a scrollgroup with scrollbars within the group */
#define VF_SPACE       (1 << 2UL) /* wants space around the objects (probably never the case) */
#define VF_STRING      (1 << 3UL) /* wants a string object */
#define VF_TOOLBAR     (1 << 4UL) /* wants a toolbar */
#define VF_SCROLLERS   (1 << 5UL) /* wants scrollers only (mutually exclusive with VF_SCROLLWIN/VF_SCROLLGROUP) */
#define VF_SEARCHABLE  (1 << 6UL) /* supports fast searching functionality */
#define VF_FILEVIEW    (1 << 7UL) /* view displays files/directories */
#define VF_DEVICEVIEW  (1 << 8UL) /* view displays devices */

ULONG viewapi_init(void);
void viewapi_cleanup(void);
ULONG viewapi_scan(void);
struct viewnode *viewapi_findbyclass(struct IClass *cl);
struct viewnode *viewapi_findbyname(CONST_STRPTR name);
struct viewnode *viewapi_findbyid(ULONG id);
struct viewnode *viewapi_findbymime(CONST_STRPTR name);
struct viewnode *viewapi_nextviewmime(struct viewnode *vn, CONST_STRPTR mimename);
ULONG viewapi_checkmime(ULONG viewid, CONST_STRPTR mimename);
ULONG viewapi_getmodeindex(struct viewnode *vn, CONST_STRPTR modename);
STRPTR viewapi_getmodename( struct viewnode *vn, ULONG modeindex );
ULONG viewapi_getflags( struct viewnode *vn );
ULONG viewapi_compare_idx2name(ULONG viewindex, CONST_STRPTR name);

#define VIEW_TEMPLATE "L=LEFT/N,T=TOP/N,W=WIDTH/N,H=HEIGHT/N,M=MODE/K,V=VIEW/K,TYPE/K,SB=SORTBY/K,SO=SORTORDER/K"

struct v_args {
	LONG *left;
	LONG *top;
	LONG *width;
	LONG *height;
	STRPTR mode;
	STRPTR view;
	STRPTR type;
	STRPTR sortby;
	STRPTR sortorder;
};

#endif /* AMBIENT_VIEWAPI_H */
